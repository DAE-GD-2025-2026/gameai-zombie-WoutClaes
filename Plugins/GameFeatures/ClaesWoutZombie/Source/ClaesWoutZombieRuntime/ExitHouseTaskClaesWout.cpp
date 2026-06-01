#include "ExitHouseTaskClaesWout.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Navigation/PathFollowingComponent.h"

UExitHouseTaskClaesWout::UExitHouseTaskClaesWout()
{
	NodeName = "Exit House";
	bNotifyTick = true;

	ExitLocationKey.AddVectorFilter(this, GET_MEMBER_NAME_CHECKED(UExitHouseTaskClaesWout, ExitLocationKey));
}

EBTNodeResult::Type UExitHouseTaskClaesWout::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* AICon = OwnerComp.GetAIOwner();
	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	if (!AICon || !BB) return EBTNodeResult::Failed;

	FExitHouseMemory* Memory = reinterpret_cast<FExitHouseMemory*>(NodeMemory);
	Memory->ExitTimer = 0.f;
	Memory->ExitLocation = BB->GetValueAsVector(ExitLocationKey.SelectedKeyName);

	AICon->MoveToLocation(Memory->ExitLocation, 150.f);

	return EBTNodeResult::InProgress;
}

void UExitHouseTaskClaesWout::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	AAIController* AICon = OwnerComp.GetAIOwner();
	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	if (!AICon || !AICon->GetPawn() || !BB)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	FExitHouseMemory* Memory = reinterpret_cast<FExitHouseMemory*>(NodeMemory);
	Memory->ExitTimer += DeltaSeconds;

	if (Memory->ExitTimer >= MaxExitHouseTime)
	{
		BB->SetValueAsObject(FName("TargetHouse"), nullptr);
		BB->SetValueAsBool(FName("DesireHouse"), false);
		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
		return;
	}

	float DistSq = FVector::DistSquared(AICon->GetPawn()->GetActorLocation(), Memory->ExitLocation);
	if (DistSq <= ExitAcceptanceRadius * ExitAcceptanceRadius)
	{
		BB->SetValueAsObject(FName("TargetHouse"), nullptr);
		BB->SetValueAsBool(FName("DesireHouse"), false);
		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
	}
}

EBTNodeResult::Type UExitHouseTaskClaesWout::AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	if (AAIController* AICon = OwnerComp.GetAIOwner())
		AICon->StopMovement();
	return EBTNodeResult::Aborted;
}
