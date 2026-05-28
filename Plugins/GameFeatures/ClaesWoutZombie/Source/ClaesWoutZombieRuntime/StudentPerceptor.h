// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Perception/AIPerceptionComponent.h"
#include "StudentPerceptor.generated.h"

class USurvivorWanderer;
class USurvivorItemPursuer;
class USurvivorHouseExplorer;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class CLAESWOUTZOMBIERUNTIME_API UStudentPerceptor : public UActorComponent
{
	GENERATED_BODY()

public:
	UStudentPerceptor();

protected:
	virtual void BeginPlay() override;

private:
	void SetupNavCollision();
	void SetupBehaviourComponents();

	UFUNCTION()
	void OnPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus);

	// Priority stack — each resumes the next lower priority when it finishes
	// Priority 1 (highest): Item pursuit
	void BeginItemPursuit();
	void EndItemPursuit(); // resumes house exploration or wandering

	// Priority 2: House exploration
	void BeginHouseExploration();
	void EndHouseExploration(); // resumes wandering

	// Priority 3 (lowest): Wandering

	UPROPERTY()
	USurvivorWanderer* Wanderer{nullptr};

	UPROPERTY()
	USurvivorItemPursuer* ItemPursuer{nullptr};

	UPROPERTY()
	USurvivorHouseExplorer* HouseExplorer{nullptr};
};
