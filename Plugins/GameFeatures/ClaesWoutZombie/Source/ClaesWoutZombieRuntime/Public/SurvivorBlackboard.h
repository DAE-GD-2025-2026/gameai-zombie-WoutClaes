// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Items/BaseItem.h"
#include "Items/ItemType.h"

/**
 * Plain data struct shared between UStudentPerceptor (writer) and USurvivorBrainComponent (reader).
 * No UObject overhead - just a snapshot of perceived world state.
 */
struct FSurvivorBlackboard
{
	// --- Threats ---
	// Zombies currently perceived (in sight or recently damaged by)
	TArray<TWeakObjectPtr<APawn>> KnownZombies;

	// Purge zones we have seen (actors in world, located via overlap/sight)
	TArray<TWeakObjectPtr<AActor>> KnownPurgeZones;

	// --- Items ---
	// Items seen in the vision cone, sorted by priority score (descending) each update
	TArray<TWeakObjectPtr<ABaseItem>> KnownItems;

	// Closest item of each type (cached for fast decision making)
	TWeakObjectPtr<ABaseItem> ClosestMedkit;
	TWeakObjectPtr<ABaseItem> ClosestFood;
	TWeakObjectPtr<ABaseItem> ClosestWeapon;

	// --- Self state (mirrors from components, written by brain each tick) ---
	float HealthRatio    = 1.0f;   // 0..1
	float StaminaRatio   = 1.0f;   // 0..1
	bool  bHasWeapon     = false;
	int   WeaponSlotIdx  = -1;
	int   MedkitSlotIdx  = -1;
	int   FoodSlotIdx    = -1;

	// --- Navigation ---
	// A wander target when nothing better is known
	FVector WanderTarget = FVector::ZeroVector;
	bool    bWanderTargetValid = false;

	// Nearest zombie direction & distance (cached each tick)
	FVector NearestZombieDirection = FVector::ZeroVector;
	float   NearestZombieDistance  = TNumericLimits<float>::Max();

	void ClearStaleEntries()
	{
		KnownZombies.RemoveAll([](TWeakObjectPtr<APawn> const& P) { return !P.IsValid(); });
		KnownPurgeZones.RemoveAll([](TWeakObjectPtr<AActor> const& A) { return !A.IsValid(); });
		KnownItems.RemoveAll([](TWeakObjectPtr<ABaseItem> const& I) { return !I.IsValid() || I->GetValue() <= 0; });
	}
};
