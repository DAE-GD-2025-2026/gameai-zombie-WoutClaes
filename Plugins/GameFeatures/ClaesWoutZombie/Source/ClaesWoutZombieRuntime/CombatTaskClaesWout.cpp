#include "CombatTaskClaesWout.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Navigation/PathFollowingComponent.h"
#include "Common/InventoryComponent.h"
#include "Items/BaseItem.h"
#include "Items/Weapon.h"

UCombatTaskClaesWout::UCombatTaskClaesWout()
{
	NodeName = "Combat";
	bNotifyTick = true;

	TargetActorKey.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(UCombatTaskClaesWout, TargetActorKey), AActor::StaticClass());
}

EBTNodeResult::Type UCombatTaskClaesWout::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* AICon = OwnerComp.GetAIOwner();
	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	if (!AICon || !BB) return EBTNodeResult::Failed;

	AActor* Target = Cast<AActor>(BB->GetValueAsObject(TargetActorKey.SelectedKeyName));
	if (!IsValid(Target)) return EBTNodeResult::Failed;

	FCombatMemory* Memory = reinterpret_cast<FCombatMemory*>(NodeMemory);
	Memory->WeaponFireTimer = 0.f;
	Memory->TimeSinceZombieSeen = 0.f;

	AICon->SetFocus(Target);

	return EBTNodeResult::InProgress;
}

void UCombatTaskClaesWout::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	AAIController* AICon = OwnerComp.GetAIOwner();
	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();

	if (!AICon || !AICon->GetPawn() || !BB)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	FCombatMemory* Memory = reinterpret_cast<FCombatMemory*>(NodeMemory);
	APawn* Pawn = AICon->GetPawn();

	bool bIsZombieVisible = BB->GetValueAsBool(FName("IsZombieVisible"));
	AActor* TargetZombie = Cast<AActor>(BB->GetValueAsObject(TargetActorKey.SelectedKeyName));

	if (!bIsZombieVisible)
	{
		Memory->TimeSinceZombieSeen += DeltaSeconds;
		if (Memory->TimeSinceZombieSeen >= ZombieMemoryDuration)
		{
			AICon->ClearFocus(EAIFocusPriority::Gameplay);
			FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
			return;
		}
	}

	if (!IsValid(TargetZombie))
	{
		AICon->ClearFocus(EAIFocusPriority::Gameplay);
		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
		return;
	}

	UInventoryComponent* Inventory = Pawn->FindComponentByClass<UInventoryComponent>();
	int32 WeaponSlot = -1;
	if (Inventory)
	{
		const TArray<ABaseItem*>& Items = Inventory->GetInventory();
		for (int32 i = 0; i < Items.Num(); ++i)
		{
			if (Items[i] && Items[i]->IsA(AWeapon::StaticClass()) && Items[i]->GetValue() > 0)
			{
				WeaponSlot = i;
				break;
			}
		}
	}

	if (WeaponSlot == -1)
	{
		AICon->ClearFocus(EAIFocusPriority::Gameplay);
		BB->SetValueAsBool(FName("ShouldFight"), false);
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	float DistToZombie = FVector::Dist(Pawn->GetActorLocation(), TargetZombie->GetActorLocation());

	if (DistToZombie > CombatEngageRange)
	{
		if (AICon->GetMoveStatus() == EPathFollowingStatus::Idle)
			AICon->MoveToActor(TargetZombie, CombatEngageRange * 0.8f);
		return;
	}

	if (AICon->GetMoveStatus() != EPathFollowingStatus::Idle)
		AICon->StopMovement();

	if (Memory->WeaponFireTimer > 0.f)
	{
		Memory->WeaponFireTimer -= DeltaSeconds;
	}
	else
	{
		bool bFired = Inventory->UseItem(WeaponSlot);
		if (bFired)
		{
			Memory->WeaponFireTimer = WeaponFireCooldownDuration;

			ABaseItem* WeaponItem = Inventory->GetInventory()[WeaponSlot];
			if (WeaponItem && WeaponItem->GetValue() <= 0)
				Inventory->RemoveItem(WeaponSlot);
		}
	}
}

EBTNodeResult::Type UCombatTaskClaesWout::AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	if (AAIController* AICon = OwnerComp.GetAIOwner())
	{
		AICon->StopMovement();
		AICon->ClearFocus(EAIFocusPriority::Gameplay);
	}
	return EBTNodeResult::Aborted;
}
