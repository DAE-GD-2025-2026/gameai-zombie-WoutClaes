// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SurvivorItemPursuer.generated.h"

class ABaseItem;
class UInventoryComponent;

// Fired when the pursuer has no more items to chase
DECLARE_DELEGATE(FOnItemListEmpty);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class CLAESWOUTZOMBIERUNTIME_API USurvivorItemPursuer : public UActorComponent
{
	GENERATED_BODY()

public:
	USurvivorItemPursuer();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	void SetEnabled(bool bEnabled);
	bool IsEnabled() const { return bIsEnabled; }
	bool HasKnownItems() const { return KnownItems.Num() > 0; }

	void OnItemSpotted(ABaseItem* Item);
	void OnItemLost(ABaseItem* Item);

	FOnItemListEmpty OnItemListEmpty;

private:
	bool TryPickup();
	int32 FindFreeSlot(UInventoryComponent* Inventory) const;
	void MoveToClosestItem();

	UPROPERTY()
	TArray<ABaseItem*> KnownItems;

	UPROPERTY()
	ABaseItem* CurrentTarget{nullptr};

	bool bIsEnabled{false};
	
	FVector LastLocation{FVector::ZeroVector};
	float TimeStuck{0.f};

	UPROPERTY(EditAnywhere, Category="AI|Pursuit")
	float StuckThreshold{2.f};
};
