// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SurvivorHouseExplorer.generated.h"

class AHouse;

DECLARE_DELEGATE(FOnHouseListEmpty);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class CLAESWOUTZOMBIERUNTIME_API USurvivorHouseExplorer : public UActorComponent
{
	GENERATED_BODY()

public:
	USurvivorHouseExplorer();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	void SetEnabled(bool bEnabled);
	bool IsEnabled() const { return bIsEnabled; }

	void OnHouseSpotted(AHouse* House);

	bool HasUnvisitedHouses() const;
	
	FOnHouseListEmpty OnHouseListEmpty;

private:
	void MoveToClosestUnvisitedHouse();

	UPROPERTY()
	TArray<AHouse*> UnvisitedHouses;

	UPROPERTY()
	TArray<AHouse*> VisitedHouses;

	UPROPERTY()
	AHouse* CurrentTarget{nullptr};

	bool bIsEnabled{false};

	UPROPERTY(EditAnywhere, Category="AI|Exploration")
	float HouseVisitRadius{300.f};
	
	FVector LastLocation{FVector::ZeroVector};
	float TimeStuck{0.f};

	UPROPERTY(EditAnywhere, Category="AI|Exploration")
	float StuckThreshold{3.f};
};
