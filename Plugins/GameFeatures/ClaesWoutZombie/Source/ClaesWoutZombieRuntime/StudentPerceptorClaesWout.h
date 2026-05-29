// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SurvivorMovementClaesWout.h"
#include "Components/ActorComponent.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"
#include "Perception/AISenseConfig_Damage.h"
#include "Perception/AISense_Damage.h"
#include "StudentPerceptorClaesWout.generated.h"

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class CLAESWOUTZOMBIERUNTIME_API UStudentPerceptorClaesWout : public UActorComponent
{
	GENERATED_BODY()

public:
	UStudentPerceptorClaesWout();
	
	virtual void BeginPlay() override;

	UFUNCTION()
	virtual void OnPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus);
	
	void SetupNavCollision();
	void SetupBehaviourComponents();

	UPROPERTY()
	USurvivorMovementClaesWout* Wanderer{nullptr};
};
