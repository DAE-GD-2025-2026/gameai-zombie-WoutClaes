#include "PickupItemTaskClaesWout.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Items/BaseItem.h"
#include "Common/InventoryComponent.h"

UPickupItemTaskClaesWout::UPickupItemTaskClaesWout()
{
	NodeName = "Pickup Item";
	bNotifyTick = true;

	TargetActorKey.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(UPickupItemTaskClaesWout, TargetActorKey), AActor::StaticClass());
}

EBTNodeResult::Type UPickupItemTaskClaesWout::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* AICon = OwnerComp.GetAIOwner();
	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	if (!AICon || !BB) return EBTNodeResult::Failed;

	ABaseItem* Item = Cast<ABaseItem>(BB->GetValueAsObject(TargetActorKey.SelectedKeyName));
	if (!Item) return EBTNodeResult::Failed;

	FPickupItemMemory* Memory = reinterpret_cast<FPickupItemMemory*>(NodeMemory);
	Memory->PickupTimer = 0.f;

	float EffectivePickupRadius = ItemPickupRadius;
	if (APawn* Pawn = AICon->GetPawn())
	{
		if (UInventoryComponent* Inventory = Pawn->FindComponentByClass<UInventoryComponent>())
			EffectivePickupRadius = Inventory->GetPickupRange();
	}

	AICon->ClearFocus(EAIFocusPriority::Gameplay);
	AICon->MoveToActor(Item);

	return EBTNodeResult::InProgress;
}

void UPickupItemTaskClaesWout::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	FPickupItemMemory* Memory = reinterpret_cast<FPickupItemMemory*>(NodeMemory);
	Memory->PickupTimer += DeltaSeconds;

	AAIController* AICon = OwnerComp.GetAIOwner();
	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();

	if (Memory->PickupTimer >= MaxPickupTime)
	{
		if (BB)
		{
			BB->SetValueAsObject(TargetActorKey.SelectedKeyName, nullptr);
			BB->SetValueAsBool(FName("DesireItem"), false);
		}
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	if (!AICon || !AICon->GetPawn() || !BB)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	ABaseItem* Item = Cast<ABaseItem>(BB->GetValueAsObject(TargetActorKey.SelectedKeyName));
	if (!Item)
	{
		if (BB) BB->SetValueAsBool(FName("DesireItem"), false);
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	APawn* Pawn = AICon->GetPawn();
	UInventoryComponent* Inventory = Pawn->FindComponentByClass<UInventoryComponent>();
	if (!Inventory)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	float EffectivePickupRadius = Inventory->GetPickupRange();
	float DistSq = FVector::DistSquared(Pawn->GetActorLocation(), Item->GetActorLocation());

	if (DistSq <= EffectivePickupRadius * EffectivePickupRadius)
	{
		int32 Capacity = Inventory->GetInventoryCapacity();
		int32 TargetSlot = -1;
		bool bIsReplacing = false;

		for (int32 Slot = 0; Slot < Capacity; ++Slot)
		{
			ABaseItem* InvItem = Inventory->GetInventory()[Slot];
			if (InvItem && InvItem->GetClass() == Item->GetClass() && InvItem->GetValue() < Item->GetValue())
			{
				TargetSlot = Slot;
				bIsReplacing = true;
				break;
			}
		}

		if (!bIsReplacing)
		{
			for (int32 Slot = 0; Slot < Capacity; ++Slot)
			{
				ABaseItem* InvItem = Inventory->GetInventory()[Slot];
				if (InvItem == nullptr || InvItem->GetValue() <= 0)
				{
					TargetSlot = Slot;
					break;
				}
			}
		}

		if (TargetSlot != -1)
		{
			if (bIsReplacing)
			{
				Inventory->RemoveItem(TargetSlot); 
			}
			
			Inventory->GrabItem(TargetSlot, Item);
		}

		BB->SetValueAsObject(TargetActorKey.SelectedKeyName, nullptr);
		BB->SetValueAsBool(FName("DesireItem"), false);
		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
	}
}

EBTNodeResult::Type UPickupItemTaskClaesWout::AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	if (AAIController* AICon = OwnerComp.GetAIOwner())
		AICon->StopMovement();
	return EBTNodeResult::Aborted;
}
