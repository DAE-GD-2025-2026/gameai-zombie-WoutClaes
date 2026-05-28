// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SurvivorWanderer.generated.h"

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class CLAESWOUTZOMBIERUNTIME_API USurvivorWanderer : public UActorComponent
{
	GENERATED_BODY()

public:
	USurvivorWanderer();

protected:
	virtual void BeginPlay() override;

public:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	void SetEnabled(bool bEnabled);
	bool IsEnabled() const { return bIsEnabled; }

private:
	void PickNewWanderTarget();

	UPROPERTY(EditAnywhere, Category="AI|Wander")
	float WanderRadius{2000.f};

	UPROPERTY(EditAnywhere, Category="AI|Wander")
	float AcceptanceRadius{200.f};

	bool bIsEnabled{true};
	
	FVector LastLocation{FVector::ZeroVector};
	float TimeStuck{0.f};

	UPROPERTY(EditAnywhere, Category="AI|Wander")
	float StuckThreshold{3.f};
};
