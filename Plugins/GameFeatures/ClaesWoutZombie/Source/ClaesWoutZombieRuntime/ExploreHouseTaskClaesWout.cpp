#include "ExploreHouseTaskClaesWout.h"
#include "AIController.h"
#include "StudentPerceptorClaesWout.h"
#include "BehaviorTree/BlackboardComponent.h"

UExploreHouseTaskClaesWout::UExploreHouseTaskClaesWout()
{
	NodeName = "Explore House";
	bNotifyTick = true;

	TargetActorKey.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(UExploreHouseTaskClaesWout, TargetActorKey), AActor::StaticClass());
}

EBTNodeResult::Type UExploreHouseTaskClaesWout::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* AICon = OwnerComp.GetAIOwner();
	if (!AICon || !AICon->GetPawn()) return EBTNodeResult::Failed;

	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	if (!BB) return EBTNodeResult::Failed;

	AActor* House = Cast<AActor>(BB->GetValueAsObject(TargetActorKey.SelectedKeyName));
	if (!House) return EBTNodeResult::Failed;

	FExploreHouseMemory* Memory = reinterpret_cast<FExploreHouseMemory*>(NodeMemory);
	Memory->ExploreTimer = 0.f;
	Memory->EntranceLocation = AICon->GetPawn()->GetActorLocation();

	FVector HouseCenter, Extents;
	House->GetActorBounds(true, HouseCenter, Extents);
	AICon->ClearFocus(EAIFocusPriority::Gameplay);
	AICon->MoveToLocation(HouseCenter, HouseAcceptanceRadius);

	return EBTNodeResult::InProgress;
}

void UExploreHouseTaskClaesWout::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	AAIController* AICon = OwnerComp.GetAIOwner();
	if (!AICon || !AICon->GetPawn())
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	AActor* House = BB ? Cast<AActor>(BB->GetValueAsObject(TargetActorKey.SelectedKeyName)) : nullptr;
	if (!House)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	FExploreHouseMemory* Memory = reinterpret_cast<FExploreHouseMemory*>(NodeMemory);
	Memory->ExploreTimer += DeltaSeconds;

	if (Memory->ExploreTimer >= MaxExploreHouseTime)
	{
		if (BB) BB->SetValueAsVector(FName("HouseExitLocation"), Memory->EntranceLocation);
		if (UStudentPerceptorClaesWout* Perceptor = AICon->GetPawn()->FindComponentByClass<UStudentPerceptorClaesWout>())
		{
			Perceptor->AddVisitedHouse(House);
		}
		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
		return;
	}

	FVector HouseCenter, Extents;
	House->GetActorBounds(true, HouseCenter, Extents);
	float DistSq = FVector::DistSquared(AICon->GetPawn()->GetActorLocation(), HouseCenter);
	if (DistSq <= HouseAcceptanceRadius * HouseAcceptanceRadius)
	{
		if (BB) BB->SetValueAsVector(FName("HouseExitLocation"), Memory->EntranceLocation);
	
		if (UStudentPerceptorClaesWout* Perceptor = AICon->GetPawn()->FindComponentByClass<UStudentPerceptorClaesWout>())
		{
			Perceptor->AddVisitedHouse(House);
		}
		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
	}
}

EBTNodeResult::Type UExploreHouseTaskClaesWout::AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	if (AAIController* AICon = OwnerComp.GetAIOwner())
		AICon->StopMovement();
	return EBTNodeResult::Aborted;
}
