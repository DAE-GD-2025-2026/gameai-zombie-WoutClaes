// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Perception/AIPerceptionComponent.h"
#include "SurvivorBlackboard.h"
#include "StudentPerceptor.generated.h"

class USurvivorBrainComponent;
class ASurvivorPawn;
class ASurvivorAIController;

/**
 * UStudentPerceptor
 *
 * Added to ASurvivorPawn by the GameFeature extension system.
 * Binds to the pawn's AIPerceptionComponent and writes perceived
 * actors into a shared FSurvivorBlackboard.
 *
 * It also creates USurvivorBrainComponent on the AIController
 * once possession has happened (retries via timer if needed).
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class CLAESWOUTZOMBIERUNTIME_API UStudentPerceptor : public UActorComponent
{
	GENERATED_BODY()

public:
	UStudentPerceptor();
	virtual void BeginPlay() override;

	UFUNCTION()
	virtual void OnPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus);

private:
	// Retried until the AIController is possessed
	UFUNCTION()
	void TryRegisterBrain();

	// Owned blackboard shared with the brain
	TUniquePtr<FSurvivorBlackboard> Blackboard;

	UPROPERTY()
	TObjectPtr<ASurvivorPawn> SurvivorPawn { nullptr };

	UPROPERTY()
	TObjectPtr<USurvivorBrainComponent> BrainComponent { nullptr };
};
