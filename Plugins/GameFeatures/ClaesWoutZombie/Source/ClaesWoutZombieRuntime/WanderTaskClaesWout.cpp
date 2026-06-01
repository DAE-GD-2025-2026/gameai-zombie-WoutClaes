#include "WanderTaskClaesWout.h"
#include "AIController.h"
#include "NavigationSystem.h"
#include "StudentPerceptorClaesWout.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Navigation/PathFollowingComponent.h"

UWanderTaskClaesWout::UWanderTaskClaesWout()
{
	NodeName = "Smooth Wander";
	bNotifyTick = true;
	
	DestinationKey.AddVectorFilter(this, GET_MEMBER_NAME_CHECKED(UWanderTaskClaesWout, DestinationKey));
}

EBTNodeResult::Type UWanderTaskClaesWout::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* AICon = OwnerComp.GetAIOwner();
	if (!AICon || !AICon->GetPawn()) return EBTNodeResult::Failed;

	FVector InitialDest;
	if (PickWanderLocation(AICon, InitialDest))
	{
		OwnerComp.GetBlackboardComponent()->SetValueAsVector(DestinationKey.SelectedKeyName, InitialDest);
		AICon->MoveToLocation(InitialDest, 100.f, false, true, true);
		return EBTNodeResult::InProgress;
	}
	
	return EBTNodeResult::Failed;
}

void UWanderTaskClaesWout::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	AAIController* AICon = OwnerComp.GetAIOwner();
	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	if (!AICon || !AICon->GetPawn() || !BB)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	FVector TargetDest = BB->GetValueAsVector(DestinationKey.SelectedKeyName);
	float DistToDest = FVector::Dist(AICon->GetPawn()->GetActorLocation(), TargetDest);

	if (DistToDest <= 250.f || AICon->GetMoveStatus() == EPathFollowingStatus::Idle)
	{
		FVector NextDest;
		if (PickWanderLocation(AICon, NextDest))
		{
			BB->SetValueAsVector(DestinationKey.SelectedKeyName, NextDest);
			AICon->MoveToLocation(NextDest, 100.f, false, true, true);
		}
	}
}

bool UWanderTaskClaesWout::PickWanderLocation(AAIController* AICon, FVector& OutLocation)
{
	UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
	UStudentPerceptorClaesWout* Perceptor = AICon->GetPawn()->FindComponentByClass<UStudentPerceptorClaesWout>();
	if (!NavSys || !Perceptor) return false;

	FVector Origin = AICon->GetPawn()->GetActorLocation();
	FVector BestDirection = FMath::VRand().GetSafeNormal2D();
	float BestScore = -1.f;
	
	const TArray<FVector>& Breadcrumbs = Perceptor->GetBreadcrumbs();

	for (int32 i = 0; i < 6; ++i)
	{
		FVector TestDir = FMath::VRand().GetSafeNormal2D();
		FVector TestLoc = Origin + (TestDir * ForwardDistance);
		
		float Score = 0.f;
		if (Breadcrumbs.Num() == 0)
		{
			BestDirection = TestDir;
			break;
		}

		for (const FVector& Crumb : Breadcrumbs)
		{
			Score += FVector::DistSquared(TestLoc, Crumb);
		}

		if (Score > BestScore)
		{
			BestScore = Score;
			BestDirection = TestDir;
		}
	}

	FNavLocation NavLoc;
	if (NavSys->GetRandomReachablePointInRadius(Origin + (BestDirection * ForwardDistance), ForwardRadius, NavLoc))
	{
		OutLocation = NavLoc.Location;
		return true;
	}
	return false;
}
