// Fill out your copyright notice in the Description page of Project Settings.

#include "SurvivorItemPursuer.h"
#include "AIController.h"
#include "Items/BaseItem.h"
#include "Survivor/SurvivorPawn.h"
#include "Common/InventoryComponent.h"
#include "Navigation/PathFollowingComponent.h"

USurvivorItemPursuer::USurvivorItemPursuer()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void USurvivorItemPursuer::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!bIsEnabled || !CurrentTarget)
		return;

	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (!OwnerPawn)
		return;

	FVector const PawnLocation = OwnerPawn->GetActorLocation();

	float const SpeedSq = FVector::DistSquared(PawnLocation, LastLocation) / (DeltaTime * DeltaTime);
	LastLocation = PawnLocation;

	if (SpeedSq < 100.f)
	{
		TimeStuck += DeltaTime;
		if (TimeStuck > StuckThreshold)
		{
			KnownItems.Remove(CurrentTarget);
			CurrentTarget = nullptr;
			TimeStuck = 0.f;
			
			if (KnownItems.IsEmpty())
			{
				OnItemListEmpty.ExecuteIfBound();
				return;
			}
			MoveToClosestItem();
			return;
		}
	}
	else
	{
		TimeStuck = 0.f;
	}

	if (TryPickup())
	{
		KnownItems.Remove(CurrentTarget);
		CurrentTarget = nullptr;
		TimeStuck = 0.f;

		if (KnownItems.IsEmpty())
		{
			OnItemListEmpty.ExecuteIfBound();
			return;
		}

		MoveToClosestItem();
		return;
	}

	if (AAIController* AIC = Cast<AAIController>(OwnerPawn->GetController()))
	{
		if (AIC->GetMoveStatus() == EPathFollowingStatus::Idle)
			MoveToClosestItem();
	}
}

void USurvivorItemPursuer::MoveToClosestItem()
{
	KnownItems.RemoveAll([](ABaseItem* Item) { return !IsValid(Item); });

	if (KnownItems.IsEmpty())
		return;

	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (!OwnerPawn)
		return;

	AAIController* AIC = Cast<AAIController>(OwnerPawn->GetController());
	if (!AIC)
		return;

	if (CurrentTarget && KnownItems.Contains(CurrentTarget))
	{
		AIC->MoveToActor(CurrentTarget, 50.f);
		return;
	}

	FVector const PawnLocation = OwnerPawn->GetActorLocation();
	ABaseItem* Closest = nullptr;
	float BestDistSq = MAX_FLT;

	for (ABaseItem* Item : KnownItems)
	{
		float const DistSq = FVector::DistSquared(PawnLocation, Item->GetActorLocation());
		if (DistSq < BestDistSq)
		{
			BestDistSq = DistSq;
			Closest = Item;
		}
	}

	if (Closest)
	{
		CurrentTarget = Closest;
		AIC->MoveToActor(CurrentTarget, 50.f);
	}
}

void USurvivorItemPursuer::SetEnabled(bool bEnabled)
{
	bIsEnabled = bEnabled;

	if (bIsEnabled)
	{
		MoveToClosestItem();
	}
	else
	{
		CurrentTarget = nullptr;

		APawn* OwnerPawn = Cast<APawn>(GetOwner());
		if (!OwnerPawn)
			return;

		if (UMovementComponent* Movement = OwnerPawn->GetComponentByClass<UMovementComponent>())
			Movement->StopMovementImmediately();
	}
}

void USurvivorItemPursuer::OnItemSpotted(ABaseItem* Item)
{
	if (!Item)
		return;

	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (OwnerPawn)
	{
		if (UInventoryComponent* Inventory = OwnerPawn->GetComponentByClass<UInventoryComponent>())
		{
			if (FindFreeSlot(Inventory) == -1)
			{
				return;
			}
		}
	}

	KnownItems.AddUnique(Item);

	if (bIsEnabled && !CurrentTarget)
		MoveToClosestItem();
}

void USurvivorItemPursuer::OnItemLost(ABaseItem* Item)
{
	if (!Item)
		return;

	KnownItems.Remove(Item);

	if (CurrentTarget == Item)
	{
		CurrentTarget = nullptr;

		if (KnownItems.IsEmpty())
			OnItemListEmpty.ExecuteIfBound();
		else
			MoveToClosestItem();
	}
}

bool USurvivorItemPursuer::TryPickup()
{
	if (!CurrentTarget)
		return false;

	ASurvivorPawn* OwnerPawn = Cast<ASurvivorPawn>(GetOwner());
	if (!OwnerPawn)
		return false;

	UInventoryComponent* Inventory = OwnerPawn->GetComponentByClass<UInventoryComponent>();
	if (!Inventory)
		return false;

	int32 const SlotIdx = FindFreeSlot(Inventory);
	if (SlotIdx == -1)
	{
		UE_LOG(LogTemp, Warning, TEXT("SurvivorItemPursuer: Inventory full - abandoning pursuit"));
		KnownItems.Empty();
		CurrentTarget = nullptr;
		OnItemListEmpty.ExecuteIfBound();
		return false;
	}

	float const PickupRange = Inventory->GetPickupRange();
	float const DistSq = FVector::DistSquared(OwnerPawn->GetActorLocation(), CurrentTarget->GetActorLocation());
	if (DistSq > PickupRange * PickupRange)
		return false;

	return Inventory->GrabItem(SlotIdx, CurrentTarget);
}

int32 USurvivorItemPursuer::FindFreeSlot(UInventoryComponent* Inventory) const
{
	TArray<ABaseItem*> const& Items = Inventory->GetInventory();
	for (int32 i = 0; i < Items.Num(); ++i)
	{
		if (Items[i] == nullptr)
			return i;
	}
	return -1;
}
