// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SurvivorBlackboard.h"
#include "SurvivorFSM.h"
#include "SurvivorBrainComponent.generated.h"

class ASurvivorPawn;
class ASurvivorAIController;

/**
 * USurvivorBrainComponent
 *
 * The "brain" of the AI survivor.  It is added to ASurvivorAIController by
 * UStudentPerceptor during BeginPlay (via the GameFramework component system).
 *
 * Each tick it:
 *   1. Refreshes self-state in the blackboard.
 *   2. Evaluates FSM transitions.
 *   3. Executes the behaviour for the current state.
 *
 * UStudentPerceptor writes perceived actors into the blackboard; this
 * component only reads from it (single writer / single reader pattern).
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class CLAESWOUTZOMBIERUNTIME_API USurvivorBrainComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	USurvivorBrainComponent();

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
	                           FActorComponentTickFunction* ThisTickFunction) override;

	// Called by UStudentPerceptor to hand in the shared blackboard pointer
	void SetBlackboard(FSurvivorBlackboard* InBoard) { Blackboard = InBoard; }

	FSurvivorBlackboard* GetBlackboard() const { return Blackboard; }

	// ---- Tuning ----
	UPROPERTY(EditDefaultsOnly, Category="Brain|Combat")
	float FleeRadius { 600.f };       // Start fleeing when zombie is closer than this

	UPROPERTY(EditDefaultsOnly, Category="Brain|Combat")
	float FightRadius { 900.f };      // Engage when zombie is closer than this

	UPROPERTY(EditDefaultsOnly, Category="Brain|Combat")
	float ShootRadius { 850.f };      // Actually fire when within this range

	UPROPERTY(EditDefaultsOnly, Category="Brain|Health")
	float LowHealthThreshold { 0.3f };// Use medkit / flee below this ratio

	UPROPERTY(EditDefaultsOnly, Category="Brain|Health")
	float SafeHealthThreshold { 0.5f };// Resume fighting above this ratio

	UPROPERTY(EditDefaultsOnly, Category="Brain|Stamina")
	float LowStaminaThreshold { 0.25f };

	UPROPERTY(EditDefaultsOnly, Category="Brain|Navigation")
	float WanderRadius { 1500.f };    // Random wander target radius

	UPROPERTY(EditDefaultsOnly, Category="Brain|Navigation")
	float ItemPickupRadius { 110.f }; // Distance to trigger GrabItem

	UPROPERTY(EditDefaultsOnly, Category="Brain|Navigation")
	float ReachedTargetRadius { 150.f };

private:
	// ---- FSM ----
	ESurvivorState CurrentState { ESurvivorState::Explore };
	ESurvivorState PreviousState { ESurvivorState::Explore };

	void TransitionTo(ESurvivorState NewState);
	ESurvivorState EvaluateTransitions() const;

	// ---- Per-state tick functions ----
	void TickExplore(float DeltaTime);
	void TickSeekItem(float DeltaTime);
	void TickFight(float DeltaTime);
	void TickFlee(float DeltaTime);
	void TickUseMedkit(float DeltaTime);

	// ---- Helpers ----
	void RefreshSelfState();
	void RefreshNearestZombie();
	void RefreshClosestItems();
	void PickupNearbyItems();

	/** Move toward a world location using the AIController. */
	void MoveToward(FVector const& Target, bool bRun = false);

	/** Stop any active movement request. */
	void StopMovement();

	/** Try to use the weapon in WeaponSlotIdx facing Direction. */
	void TryShootToward(FVector const& Direction);

	/** Pick a new random wander point and store it in the blackboard. */
	void PickNewWanderTarget();

	/** Find the best item to seek right now (medkit > weapon low ammo > food > weapon > other). */
	ABaseItem* SelectBestItem() const;

	// ---- Cached refs ----
	UPROPERTY()
	TObjectPtr<ASurvivorPawn> SurvivorPawn { nullptr };

	UPROPERTY()
	TObjectPtr<ASurvivorAIController> AIController { nullptr };

	FSurvivorBlackboard* Blackboard { nullptr };

	// Target being sought in SeekItem state
	UPROPERTY()
	TObjectPtr<ABaseItem> SeekTarget { nullptr };

	// Timer for UseMedkit – small delay to feel natural
	float MedkitUseTimer { 0.f };
	static constexpr float MedkitUseDelay { 0.4f };

	// How long we have been fleeing without seeing a zombie (to return to explore)
	float FleeTimeWithoutThreat { 0.f };
	static constexpr float FleeTimeoutDuration { 3.f };

	// ---- Rotation ----
	UPROPERTY(EditDefaultsOnly, Category="Brain|Navigation")
	float TurnInterpSpeed { 8.f }; // Higher = snappier turning

	float LastDeltaTime { 0.f };   // Cached for MoveToward rotation

	// ---- Debug ----
	void DrawDebugInfo();
	float DebugNoInitTimer { 0.f };
};
