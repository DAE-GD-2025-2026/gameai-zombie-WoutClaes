// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SurvivorBlackboard.h"
#include "SurvivorFSM.h"
#include "SurvivorBrainComponent.generated.h"

class ASurvivorPawn;
class ASurvivorAIController;
class ABaseItem;

/**
 * USurvivorBrainComponent
 *
 * Owns the FSM and drives the SurvivorPawn every tick.
 * Created on the AIController by UStudentPerceptor at runtime.
 *
 * Movement uses FloatingPawnMovement (AddMovementInput) with a
 * fan-raycast obstacle avoidance layer so the pawn steers around walls.
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

	void SetBlackboard(FSurvivorBlackboard* InBoard) { Blackboard = InBoard; }
	FSurvivorBlackboard* GetBlackboard() const { return Blackboard; }

	// ---- Tuning (editable in Blueprint defaults) ----

	UPROPERTY(EditDefaultsOnly, Category="Brain|Combat")
	float DangerRadius { 300.f };

	UPROPERTY(EditDefaultsOnly, Category="Brain|Combat")
	float FleeRadius { 600.f };

	UPROPERTY(EditDefaultsOnly, Category="Brain|Combat")
	float FightRadius { 900.f };

	UPROPERTY(EditDefaultsOnly, Category="Brain|Combat")
	float ShootRadius { 850.f };

	UPROPERTY(EditDefaultsOnly, Category="Brain|Health")
	float LowHealthThreshold { 0.3f };

	UPROPERTY(EditDefaultsOnly, Category="Brain|Health")
	float SafeHealthThreshold { 0.5f };

	UPROPERTY(EditDefaultsOnly, Category="Brain|Stamina")
	float LowStaminaThreshold { 0.25f };

	UPROPERTY(EditDefaultsOnly, Category="Brain|Navigation")
	float WanderRadius { 1500.f };

	UPROPERTY(EditDefaultsOnly, Category="Brain|Navigation")
	float ItemPickupRadius { 110.f };

	UPROPERTY(EditDefaultsOnly, Category="Brain|Navigation")
	float ReachedTargetRadius { 150.f };

	UPROPERTY(EditDefaultsOnly, Category="Brain|Navigation")
	float TurnInterpSpeed { 8.f };

	UPROPERTY(EditDefaultsOnly, Category="Brain|Navigation")
	float FleeMinDuration { 3.f };    // Seconds the pawn keeps fleeing before re-evaluating

	UPROPERTY(EditDefaultsOnly, Category="Brain|Navigation")
	float FleeDistance { 1200.f };    // How far ahead the flee target is projected

	UPROPERTY(EditDefaultsOnly, Category="Brain|Navigation")
	float FleeTargetRefreshInterval { 0.5f }; // How often the flee direction is recalculated

	UPROPERTY(EditDefaultsOnly, Category="Brain|Navigation")
	float StuckTimeout { 2.5f };      // Seconds without progress before picking a new wander target

	// ---- Obstacle avoidance ----
	UPROPERTY(EditDefaultsOnly, Category="Brain|Avoidance")
	float ProbeLength { 300.f };      // Ray length for wall detection

	UPROPERTY(EditDefaultsOnly, Category="Brain|Avoidance")
	float ProbeAngle { 40.f };        // Degrees between each probe ray (left/right of forward)

	UPROPERTY(EditDefaultsOnly, Category="Brain|Avoidance")
	float SteerStrength { 1.5f };     // How hard to steer away from a blocked ray

private:
	// ---- FSM ----
	ESurvivorState CurrentState { ESurvivorState::Explore };
	ESurvivorState PreviousState { ESurvivorState::Explore };

	void TransitionTo(ESurvivorState NewState);
	ESurvivorState EvaluateTransitions() const;

	// ---- Per-state ticks ----
	void TickExplore(float DeltaTime);
	void TickSeekItem(float DeltaTime);
	void TickFight(float DeltaTime);
	void TickFlee(float DeltaTime);
	void TickUseMedkit(float DeltaTime);

	// ---- Movement ----
	/**
	 * Drive the pawn toward Target using AddMovementInput.
	 * Applies fan-raycast obstacle avoidance to steer around walls.
	 */
	void MoveToward(FVector const& Target, bool bRun = false);
	void StopMovement();

	/**
	 * Cast a fan of rays ahead of the pawn and return a steering correction vector.
	 * Returns FVector::ZeroVector if the path is clear.
	 */
	FVector ComputeAvoidance(FVector const& DesiredDir) const;

	void UpdateFleeTarget();
	void PickNewWanderTarget();
	void TickFleeTimer(float DeltaTime);

	// ---- Item helpers ----
	void TryShootToward(FVector const& Direction);
	void PickupNearbyItems();
	ABaseItem* SelectBestItem() const;

	// ---- Blackboard refresh ----
	void RefreshSelfState();
	void RefreshNearestZombie();
	void RefreshClosestItems();

	// ---- Debug ----
	void DrawDebugInfo();

	// ---- Cached refs ----
	UPROPERTY()
	TObjectPtr<ASurvivorPawn> SurvivorPawn { nullptr };

	UPROPERTY()
	TObjectPtr<ASurvivorAIController> AIController { nullptr };

	FSurvivorBlackboard* Blackboard { nullptr };

	UPROPERTY()
	TObjectPtr<ABaseItem> SeekTarget { nullptr };

	// Spawn zone positions cached at BeginPlay for directed wandering
	TArray<FVector> SpawnZoneLocations;

	// ---- Flee state ----
	float FleeLockedTimer        { 0.f };
	float FleeTimeWithoutThreat  { 0.f };
	float FleeTargetRefreshTimer { 0.f };
	FVector CurrentFleeTarget    { FVector::ZeroVector };
	FVector FleeDirection        { FVector::ZeroVector }; // stored separately, re-projected each frame

	// ---- Explore state ----
	float StuckTimer              { 0.f };
	float LastDistToWanderTarget  { TNumericLimits<float>::Max() };

	// ---- UseMedkit state ----
	float MedkitUseTimer { 0.f };
	static constexpr float MedkitUseDelay { 0.4f };

	// ---- Avoidance ----
	FVector SteerDir { FVector::ZeroVector }; // Persistent steered direction, blends across frames

	// ---- Misc ----
	float LastDeltaTime    { 0.f };
	float DebugNoInitTimer { 0.f };
};
