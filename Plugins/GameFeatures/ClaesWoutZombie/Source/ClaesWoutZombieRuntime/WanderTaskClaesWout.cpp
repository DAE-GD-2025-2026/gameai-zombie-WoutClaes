#include "WanderTaskClaesWout.h"
#include "AIController.h"
#include "NavigationSystem.h"
#include "StudentPerceptorClaesWout.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Navigation/PathFollowingComponent.h"

UWanderTaskClaesWout::UWanderTaskClaesWout()
{
	NodeName = "Wander";
	bNotifyTick = false;
	
	DestinationKey.AddVectorFilter(this, GET_MEMBER_NAME_CHECKED(UWanderTaskClaesWout, DestinationKey));
}

EBTNodeResult::Type UWanderTaskClaesWout::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* AICon = OwnerComp.GetAIOwner();
	if (!AICon || !AICon->GetPawn()) return EBTNodeResult::Failed;

	UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
	UStudentPerceptorClaesWout* Perceptor = AICon->GetPawn()->FindComponentByClass<UStudentPerceptorClaesWout>();
	if (!NavSys || !Perceptor) return EBTNodeResult::Failed;

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
		OwnerComp.GetBlackboardComponent()->SetValueAsVector(DestinationKey.SelectedKeyName, NavLoc.Location);
		return EBTNodeResult::Succeeded;
	}
	
	return EBTNodeResult::Failed;
}
