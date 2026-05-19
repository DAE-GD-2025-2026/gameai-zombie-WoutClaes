// Fill out your copyright notice in the Description page of Project Settings.

#include "SurvivorBrainComponent.h"
#include "SurvivorAIController.h"
#include "Survivor/SurvivorPawn.h"
#include "Common/HealthComponent.h"
#include "Common/StaminaComponent.h"
#include "Common/InventoryComponent.h"
#include "Items/BaseItem.h"
#include "Items/ItemType.h"
#include "Items/ItemSpawnZone.h"
#include "GameFramework/FloatingPawnMovement.h"
#include "DrawDebugHelpers.h"
#include "Kismet/GameplayStatics.h"


USurvivorBrainComponent::USurvivorBrainComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void USurvivorBrainComponent::BeginPlay()
{
	Super::BeginPlay();

	AIController = Cast<ASurvivorAIController>(GetOwner());
	if (!AIController)
	{
		UE_LOG(LogTemp, Error, TEXT("[Brain] Owner is not ASurvivorAIController. Tick disabled."));
		SetComponentTickEnabled(false);
		return;
	}

	SurvivorPawn = Cast<ASurvivorPawn>(AIController->GetPawn());
	if (!SurvivorPawn)
		UE_LOG(LogTemp, Warning, TEXT("[Brain] Pawn not possessed yet at BeginPlay - will retry in Tick."));

	// Cache all item spawn zone locations — used for directed wandering so the pawn
	// walks toward houses (where items are) instead of random open-world points.
	TArray<AActor*> Found;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AItemSpawnZone::StaticClass(), Found);
	for (AActor* A : Found)
		SpawnZoneLocations.Add(A->GetActorLocation());

	UE_LOG(LogTemp, Warning, TEXT("[Brain] Found %d spawn zones."), SpawnZoneLocations.Num());
}

// ---------------------------------------------------------------------------
// Main Tick
// ---------------------------------------------------------------------------

void USurvivorBrainComponent::TickComponent(float DeltaTime, ELevelTick TickType,
                                             FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!SurvivorPawn && AIController)
		SurvivorPawn = Cast<ASurvivorPawn>(AIController->GetPawn());

	if (!AIController || !SurvivorPawn || !Blackboard)
	{
		DebugNoInitTimer += DeltaTime;
		if (DebugNoInitTimer >= 1.f)
		{
			DebugNoInitTimer = 0.f;
			if (GEngine) GEngine->AddOnScreenDebugMessage(1, 1.f, FColor::Red,
				FString::Printf(TEXT("[Brain] NOT READY  Ctrl:%s  Pawn:%s  BB:%s"),
					AIController ? TEXT("OK") : TEXT("NULL"),
					SurvivorPawn ? TEXT("OK") : TEXT("NULL"),
					Blackboard   ? TEXT("OK") : TEXT("NULL")));
		}
		return;
	}

	LastDeltaTime = DeltaTime;

	RefreshSelfState();
	Blackboard->ClearStaleEntries();
	RefreshNearestZombie();
	RefreshClosestItems();
	PickupNearbyItems();
	TickFleeTimer(DeltaTime);

	ESurvivorState const Desired = EvaluateTransitions();
	if (Desired != CurrentState)
		TransitionTo(Desired);

	switch (CurrentState)
	{
	case ESurvivorState::Explore:   TickExplore(DeltaTime);   break;
	case ESurvivorState::SeekItem:  TickSeekItem(DeltaTime);  break;
	case ESurvivorState::Fight:     TickFight(DeltaTime);     break;
	case ESurvivorState::Flee:      TickFlee(DeltaTime);      break;
	case ESurvivorState::UseMedkit: TickUseMedkit(DeltaTime); break;
	}

	DrawDebugInfo();
}

// ---------------------------------------------------------------------------
// FSM
// ---------------------------------------------------------------------------

void USurvivorBrainComponent::TransitionTo(ESurvivorState NewState)
{
	UE_LOG(LogTemp, Warning, TEXT("[Brain] FSM: %s --> %s"),
		*SurvivorStateToString(CurrentState), *SurvivorStateToString(NewState));

	PreviousState = CurrentState;
	CurrentState  = NewState;

	switch (NewState)
	{
	case ESurvivorState::UseMedkit:
		StopMovement();
		MedkitUseTimer = 0.f;
		break;

	case ESurvivorState::Flee:
		// Lock flee in for a minimum duration so the pawn doesn't jitter
		FleeLockedTimer = FleeMinDuration;
		FleeTimeWithoutThreat = 0.f;
		// Pick an escape direction immediately on entry
		UpdateFleeTarget();
		break;

	case ESurvivorState::SeekItem:
		SeekTarget = SelectBestItem();
		UE_LOG(LogTemp, Log, TEXT("[Brain] SeekItem target: %s"),
			SeekTarget ? *SeekTarget->GetName() : TEXT("NONE"));
		break;

	case ESurvivorState::Explore:
		// Invalidate wander target so we pick a fresh zone-directed one
		Blackboard->bWanderTargetValid = false;
		StuckTimer = 0.f;
		LastDistToWanderTarget = TNumericLimits<float>::Max();
		break;

	default:
		break;
	}
}

void USurvivorBrainComponent::TickFleeTimer(float DeltaTime)
{
	if (FleeLockedTimer > 0.f)
		FleeLockedTimer -= DeltaTime;
}

ESurvivorState USurvivorBrainComponent::EvaluateTransitions() const
{
	const float ZombieDist = Blackboard->NearestZombieDistance;
	const bool  bHasWeapon = Blackboard->bHasWeapon;
	const float Health     = Blackboard->HealthRatio;
	const bool  bHealthOk  = Health >= SafeHealthThreshold;

	// --- Priority 1: use medkit ---
	if (Health < LowHealthThreshold && Blackboard->MedkitSlotIdx >= 0)
		return ESurvivorState::UseMedkit;

	// Stay in UseMedkit until that state self-transitions
	if (CurrentState == ESurvivorState::UseMedkit)
		return ESurvivorState::UseMedkit;

	// --- Priority 2: flee lock-in (prevents per-frame jitter) ---
	// Once we enter Flee we stay in it until FleeLockedTimer expires,
	// even if the zombie briefly leaves the radius.
	if (CurrentState == ESurvivorState::Flee && FleeLockedTimer > 0.f)
		return ESurvivorState::Flee;

	// --- Priority 3: zombie threat ---
	const bool bZombieDangerous = ZombieDist < DangerRadius;
	const bool bZombieClose     = ZombieDist < FleeRadius;
	const bool bZombieInRange   = ZombieDist < FightRadius;

	if (bZombieDangerous || bZombieClose)
	{
		if (bHasWeapon && bHealthOk)
			return ESurvivorState::Fight;
		return ESurvivorState::Flee;
	}

	if (bZombieInRange && bHasWeapon && bHealthOk)
		return ESurvivorState::Fight;

	// Flee: stay until threat is gone
	if (CurrentState == ESurvivorState::Flee)
	{
		if (!Blackboard->KnownZombies.IsEmpty() && bZombieClose)
			return ESurvivorState::Flee;
		// Threat cleared — go seek items or explore
		return SelectBestItem() ? ESurvivorState::SeekItem : ESurvivorState::Explore;
	}

	// --- Priority 4: seek a known item ---
	if (SelectBestItem() != nullptr)
		return ESurvivorState::SeekItem;

	// --- Default ---
	return ESurvivorState::Explore;
}

// ---------------------------------------------------------------------------
// State behaviours
// ---------------------------------------------------------------------------

void USurvivorBrainComponent::TickExplore(float DeltaTime)
{
	FVector const MyLoc = SurvivorPawn->GetActorLocation();

	// Pick a new target if we don't have one or have reached it
	if (!Blackboard->bWanderTargetValid ||
		FVector::Dist(MyLoc, Blackboard->WanderTarget) < ReachedTargetRadius)
	{
		PickNewWanderTarget();
	}

	// Stuck detection: if we haven't closed distance in StuckTimeout seconds, pick a new target
	float const DistNow = FVector::Dist(MyLoc, Blackboard->WanderTarget);
	if (DistNow < LastDistToWanderTarget - 10.f)
	{
		// Making progress
		LastDistToWanderTarget = DistNow;
		StuckTimer = 0.f;
	}
	else
	{
		StuckTimer += DeltaTime;
		if (StuckTimer >= StuckTimeout)
		{
			UE_LOG(LogTemp, Log, TEXT("[Brain] Stuck detected — picking new wander target"));
			PickNewWanderTarget();
			StuckTimer = 0.f;
		}
	}

	SurvivorPawn->StopRunning();
	MoveToward(Blackboard->WanderTarget, false);
}

void USurvivorBrainComponent::TickSeekItem(float DeltaTime)
{
	// Re-validate target every tick
	if (!IsValid(SeekTarget) || SeekTarget->GetValue() <= 0)
	{
		SeekTarget = SelectBestItem();
		if (!SeekTarget)
		{
			TransitionTo(ESurvivorState::Explore);
			return;
		}
	}

	FVector const MyLoc   = SurvivorPawn->GetActorLocation();
	FVector const ItemLoc = SeekTarget->GetActorLocation();
	float   const Dist    = FVector::Dist(MyLoc, ItemLoc);

	if (Dist <= ItemPickupRadius)
	{
		StopMovement();
		auto* Inventory = SurvivorPawn->GetComponentByClass<UInventoryComponent>();
		if (Inventory)
		{
			for (int i = 0; i < Inventory->GetInventoryCapacity(); ++i)
			{
				if (!Inventory->GetInventory()[i])
				{
					Inventory->GrabItem(i, SeekTarget);
					UE_LOG(LogTemp, Warning, TEXT("[Brain] Grabbed item into slot %d"), i);
					break;
				}
			}
		}
		SeekTarget = nullptr;
		TransitionTo(ESurvivorState::Explore);
		return;
	}

	// Drive all the way to ItemPickupRadius -- temporarily shrink ReachedTargetRadius
	// so MoveToward doesn't stop short of the item
	float const SavedReached = ReachedTargetRadius;
	const_cast<USurvivorBrainComponent*>(this)->ReachedTargetRadius = ItemPickupRadius * 0.8f;
	bool const bUrgent = Blackboard->NearestZombieDistance < FightRadius * 1.5f;
	MoveToward(ItemLoc, bUrgent);
	const_cast<USurvivorBrainComponent*>(this)->ReachedTargetRadius = SavedReached;
}

void USurvivorBrainComponent::TickFight(float DeltaTime)
{
	if (!Blackboard->bHasWeapon)
	{
		TransitionTo(ESurvivorState::Flee);
		return;
	}

	APawn* Target = nullptr;
	float  Best   = TNumericLimits<float>::Max();
	FVector const MyLoc = SurvivorPawn->GetActorLocation();

	for (auto const& W : Blackboard->KnownZombies)
	{
		if (!W.IsValid()) continue;
		float D = FVector::Dist(MyLoc, W->GetActorLocation());
		if (D < Best) { Best = D; Target = W.Get(); }
	}

	if (!Target) { TransitionTo(ESurvivorState::Explore); return; }

	FVector const ToTarget = (Target->GetActorLocation() - MyLoc).GetSafeNormal();
	SurvivorPawn->SetActorRotation(FMath::RInterpTo(
		SurvivorPawn->GetActorRotation(), ToTarget.Rotation(), DeltaTime, TurnInterpSpeed));

	if (Best > ShootRadius)
		MoveToward(Target->GetActorLocation(), false);
	else
	{
		StopMovement();
		TryShootToward(ToTarget);
	}
}

void USurvivorBrainComponent::TickFlee(float DeltaTime)
{
	// Recalculate flee direction on a timer, but always project the target
	// ahead of the pawn's CURRENT position so it never "arrives" and stops.
	FleeTargetRefreshTimer -= DeltaTime;
	if (FleeTargetRefreshTimer <= 0.f)
	{
		UpdateFleeTarget();
		FleeTargetRefreshTimer = FleeTargetRefreshInterval;
	}

	// Re-project ahead of current position every frame
	FVector const MyLoc = SurvivorPawn->GetActorLocation();
	CurrentFleeTarget = MyLoc + FleeDirection * FleeDistance;

	bool const bShouldRun = Blackboard->StaminaRatio > LowStaminaThreshold;
	MoveToward(CurrentFleeTarget, bShouldRun);
}

void USurvivorBrainComponent::UpdateFleeTarget()
{
	FVector const MyLoc = SurvivorPawn->GetActorLocation();
	FVector Dir = -Blackboard->NearestZombieDirection;

	// Steer away from purge zones too
	for (auto const& W : Blackboard->KnownPurgeZones)
	{
		if (!W.IsValid()) continue;
		FVector const ToPurge = W->GetActorLocation() - MyLoc;
		float   const Dist    = ToPurge.Size();
		if (Dist < 600.f && Dist > 1.f)
			Dir -= ToPurge.GetSafeNormal() * (600.f - Dist) / 600.f;
	}

	if (Dir.IsNearlyZero())
		Dir = SurvivorPawn->GetActorForwardVector() * -1.f;
	Dir.Z = 0.f;
	Dir.Normalize();

	// Store direction separately — TickFlee projects ahead from current pos each frame
	FleeDirection = Dir;
	CurrentFleeTarget = MyLoc + FleeDirection * FleeDistance;
}

void USurvivorBrainComponent::TickUseMedkit(float DeltaTime)
{
	MedkitUseTimer += DeltaTime;
	if (MedkitUseTimer < MedkitUseDelay) return;

	auto* Inventory = SurvivorPawn->GetComponentByClass<UInventoryComponent>();
	if (Inventory && Blackboard->MedkitSlotIdx >= 0)
	{
		Inventory->UseItem(Blackboard->MedkitSlotIdx);
		auto const& Inv = Inventory->GetInventory();
		if (Inv[Blackboard->MedkitSlotIdx] && Inv[Blackboard->MedkitSlotIdx]->GetValue() <= 0)
			Inventory->RemoveItem(Blackboard->MedkitSlotIdx);
	}

	if (!Blackboard->KnownZombies.IsEmpty() && Blackboard->bHasWeapon)
		TransitionTo(ESurvivorState::Fight);
	else if (!Blackboard->KnownZombies.IsEmpty())
		TransitionTo(ESurvivorState::Flee);
	else
		TransitionTo(PreviousState == ESurvivorState::UseMedkit ? ESurvivorState::Explore : PreviousState);
}

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

void USurvivorBrainComponent::RefreshSelfState()
{
	auto* Health  = SurvivorPawn->GetComponentByClass<UHealthComponent>();
	auto* Stamina = SurvivorPawn->GetComponentByClass<UStaminaComponent>();
	auto* Inv     = SurvivorPawn->GetComponentByClass<UInventoryComponent>();

	if (Health)
		Blackboard->HealthRatio = (float)Health->GetHealth() / (float)Health->GetMaxHealth();
	if (Stamina)
		Blackboard->StaminaRatio = Stamina->GetCurrentStamina() / Stamina->GetMaxStamina();

	Blackboard->bHasWeapon    = false;
	Blackboard->WeaponSlotIdx = -1;
	Blackboard->MedkitSlotIdx = -1;
	Blackboard->FoodSlotIdx   = -1;

	if (Inv)
	{
		auto const& Items = Inv->GetInventory();
		for (int i = 0; i < Items.Num(); ++i)
		{
			ABaseItem* Item = Items[i];
			if (!Item || Item->GetValue() <= 0) continue;
			switch (Item->GetItemType())
			{
			case EItemType::Pistol:
			case EItemType::Shotgun:
				if (Blackboard->WeaponSlotIdx < 0) { Blackboard->bHasWeapon = true; Blackboard->WeaponSlotIdx = i; } break;
			case EItemType::Medkit:
				if (Blackboard->MedkitSlotIdx < 0) Blackboard->MedkitSlotIdx = i; break;
			case EItemType::Food:
				if (Blackboard->FoodSlotIdx < 0) Blackboard->FoodSlotIdx = i; break;
			default: break;
			}
		}
	}
}

void USurvivorBrainComponent::RefreshNearestZombie()
{
	FVector const MyLoc = SurvivorPawn->GetActorLocation();
	Blackboard->NearestZombieDistance  = TNumericLimits<float>::Max();
	Blackboard->NearestZombieDirection = FVector::ZeroVector;

	for (auto const& W : Blackboard->KnownZombies)
	{
		if (!W.IsValid()) continue;
		FVector const Delta = W->GetActorLocation() - MyLoc;
		float   const Dist  = Delta.Size();
		if (Dist < Blackboard->NearestZombieDistance)
		{
			Blackboard->NearestZombieDistance  = Dist;
			Blackboard->NearestZombieDirection = Delta.GetSafeNormal();
		}
	}
}

void USurvivorBrainComponent::RefreshClosestItems()
{
	FVector const MyLoc = SurvivorPawn->GetActorLocation();
	Blackboard->ClosestMedkit.Reset();
	Blackboard->ClosestFood.Reset();
	Blackboard->ClosestWeapon.Reset();
	float BestMedkit = TNumericLimits<float>::Max();
	float BestFood   = TNumericLimits<float>::Max();
	float BestWeapon = TNumericLimits<float>::Max();

	for (auto const& W : Blackboard->KnownItems)
	{
		if (!W.IsValid() || W->GetValue() <= 0) continue;
		float const D = FVector::Dist(MyLoc, W->GetActorLocation());
		switch (W->GetItemType())
		{
		case EItemType::Medkit:
			if (D < BestMedkit) { BestMedkit = D; Blackboard->ClosestMedkit = W; } break;
		case EItemType::Food:
			if (D < BestFood)   { BestFood   = D; Blackboard->ClosestFood   = W; } break;
		case EItemType::Pistol:
		case EItemType::Shotgun:
			if (D < BestWeapon) { BestWeapon = D; Blackboard->ClosestWeapon = W; } break;
		default: break;
		}
	}
}

void USurvivorBrainComponent::PickupNearbyItems()
{
	auto* Inventory = SurvivorPawn->GetComponentByClass<UInventoryComponent>();
	if (!Inventory) return;

	FVector const MyLoc = SurvivorPawn->GetActorLocation();
	int FreeSlot = -1;
	auto const& InvItems = Inventory->GetInventory();
	for (int i = 0; i < InvItems.Num(); ++i)
		if (!InvItems[i]) { FreeSlot = i; break; }
	if (FreeSlot < 0) return;

	for (auto const& W : Blackboard->KnownItems)
	{
		if (!W.IsValid() || W->GetValue() <= 0) continue;
		if (FVector::Dist(MyLoc, W->GetActorLocation()) <= ItemPickupRadius)
		{
			if (Inventory->GrabItem(FreeSlot, W.Get()))
			{
				FreeSlot = -1;
				auto const& R = Inventory->GetInventory();
				for (int i = 0; i < R.Num(); ++i)
					if (!R[i]) { FreeSlot = i; break; }
				if (FreeSlot < 0) break;
			}
		}
	}
}

FVector USurvivorBrainComponent::ComputeAvoidance(FVector const& DesiredDir) const
{
	FVector Origin = SurvivorPawn->GetActorLocation();
	FVector Forward = SurvivorPawn->GetActorForwardVector();
	UWorld* World = GetWorld();

	if (!World) return FVector::ZeroVector;

	FCollisionQueryParams Params;
	Params.AddIgnoredActor(SurvivorPawn);

	FHitResult CenterHit;
	
	if (!World->LineTraceSingleByChannel(CenterHit, Origin, Origin + Forward * ProbeLength, ECC_Visibility, Params))
	{
		return FVector::ZeroVector; 
	}

	float Angles[] = { 30.f, -30.f, 60.f, -60.f, 90.f, -90.f };

	for (float Angle : Angles)
	{
		FVector RayDir = Forward.RotateAngleAxis(Angle, FVector::UpVector);
		FHitResult Hit;

		if (!World->LineTraceSingleByChannel(Hit, Origin, Origin + RayDir * ProbeLength, ECC_Visibility, Params))
		{
			DrawDebugLine(World, Origin, Origin + RayDir * ProbeLength, FColor::Green, false, -1.f, 0, 2.f);
			
			return RayDir * SteerStrength * 2.0f; 
		}
	}
	return CenterHit.ImpactNormal * SteerStrength;
}

void USurvivorBrainComponent::MoveToward(FVector const& Target, bool bRun)
{
	if (bRun) SurvivorPawn->StartRunning();
	else      SurvivorPawn->StopRunning();

	FVector const MyLoc = SurvivorPawn->GetActorLocation();
	FVector Delta = Target - MyLoc;
	Delta.Z = 0.f;

	if (Delta.SizeSquared() < ReachedTargetRadius * ReachedTargetRadius) return;

	FVector SeekDirection = Delta.GetSafeNormal();
	FVector AvoidanceForce = ComputeAvoidance(SeekDirection);

	FVector DesiredFinalDir = (SeekDirection + AvoidanceForce).GetSafeNormal();
	if (DesiredFinalDir.IsNearlyZero()) DesiredFinalDir = SurvivorPawn->GetActorForwardVector();

	SteerDir = FMath::VInterpTo(SteerDir, DesiredFinalDir, LastDeltaTime, TurnInterpSpeed);
	SteerDir.Z = 0.f;
	
	FVector FinalInput = SteerDir.GetSafeNormal();

	SurvivorPawn->AddMovementInput(FinalInput, 1.0f);

	FRotator const TargetRot = FinalInput.Rotation();
	FRotator const NewRot = FMath::RInterpTo(SurvivorPawn->GetActorRotation(), TargetRot, LastDeltaTime, TurnInterpSpeed * 1.5f);
	SurvivorPawn->SetActorRotation(NewRot);
}

void USurvivorBrainComponent::StopMovement()
{
	if (auto* FPM = SurvivorPawn->GetComponentByClass<UFloatingPawnMovement>())
		FPM->StopMovementImmediately();
	SurvivorPawn->StopRunning();
}

void USurvivorBrainComponent::TryShootToward(FVector const& /*Direction*/)
{
	auto* Inventory = SurvivorPawn->GetComponentByClass<UInventoryComponent>();
	if (!Inventory || Blackboard->WeaponSlotIdx < 0) return;

	Inventory->UseItem(Blackboard->WeaponSlotIdx);

	auto const& Inv = Inventory->GetInventory();
	if (Inv[Blackboard->WeaponSlotIdx] && Inv[Blackboard->WeaponSlotIdx]->GetValue() <= 0)
		Inventory->RemoveItem(Blackboard->WeaponSlotIdx);
}

void USurvivorBrainComponent::PickNewWanderTarget()
{
	FVector const MyLoc = SurvivorPawn->GetActorLocation();

	if (SpawnZoneLocations.Num() > 0)
	{
		int32 BestIdx = -1;
		float BestScore = TNumericLimits<float>::Max();

		for (int i = 0; i < SpawnZoneLocations.Num(); ++i)
		{
			if (SpawnZoneLocations.Num() > 1 && i == LastSpawnZoneIdx)
			{
				continue;
			}

			float const Dist  = FVector::Dist(MyLoc, SpawnZoneLocations[i]);
			float const Score = FMath::Abs(Dist - WanderRadius) + FMath::FRandRange(0.f, WanderRadius * 0.4f);
			if (Score < BestScore) 
			{ 
				BestScore = Score; 
				BestIdx = i; 
			}
		}

		if (BestIdx == -1)
		{
			BestIdx = FMath::RandRange(0, SpawnZoneLocations.Num() - 1);
		}

		LastSpawnZoneIdx = BestIdx;

		FVector const Jitter = FVector(FMath::FRandRange(-150.f, 150.f), FMath::FRandRange(-150.f, 150.f), 0.f);
		Blackboard->WanderTarget       = SpawnZoneLocations[BestIdx] + Jitter;
		Blackboard->bWanderTargetValid = true;
		LastDistToWanderTarget         = FVector::Dist(MyLoc, Blackboard->WanderTarget);
		StuckTimer                     = 0.f;
		return;
	}
	else
	{
		// Fallback random wander
		FVector RandomDir = FMath::VRand();
		RandomDir.Z = 0.f;
		Blackboard->WanderTarget = SurvivorPawn->GetActorLocation() + RandomDir * 1000.f;
		Blackboard->bWanderTargetValid = true;
	}
}

ABaseItem* USurvivorBrainComponent::SelectBestItem() const
{
	if (!Blackboard) return nullptr;

	if (Blackboard->HealthRatio < SafeHealthThreshold && Blackboard->ClosestMedkit.IsValid())
		return Blackboard->ClosestMedkit.Get();
	if (!Blackboard->bHasWeapon && Blackboard->ClosestWeapon.IsValid())
		return Blackboard->ClosestWeapon.Get();
	if (Blackboard->StaminaRatio < LowStaminaThreshold && Blackboard->ClosestFood.IsValid())
		return Blackboard->ClosestFood.Get();
	if (Blackboard->ClosestMedkit.IsValid())
		return Blackboard->ClosestMedkit.Get();
	if (Blackboard->ClosestWeapon.IsValid())
		return Blackboard->ClosestWeapon.Get();
	if (Blackboard->ClosestFood.IsValid())
		return Blackboard->ClosestFood.Get();

	return nullptr;
}

// ---------------------------------------------------------------------------
// Debug
// ---------------------------------------------------------------------------

void USurvivorBrainComponent::DrawDebugInfo()
{
	if (!GEngine || !SurvivorPawn) return;

	FVector const MyLoc = SurvivorPawn->GetActorLocation();
	UWorld* World = GetWorld();

	GEngine->AddOnScreenDebugMessage(1, 0.f, FColor::Cyan,
		FString::Printf(TEXT("State: %s | HP: %.0f%% | Stamina: %.0f%%"),
			*SurvivorStateToString(CurrentState),
			Blackboard->HealthRatio  * 100.f,
			Blackboard->StaminaRatio * 100.f));

	GEngine->AddOnScreenDebugMessage(2, 0.f, FColor::Yellow,
		FString::Printf(TEXT("Zombies=%d  Items=%d  Weapon=%s | FleeTimer=%.1f"),
			Blackboard->KnownZombies.Num(),
			Blackboard->KnownItems.Num(),
			Blackboard->bHasWeapon ? TEXT("YES") : TEXT("NO"),
			FMath::Max(0.f, FleeLockedTimer)));

	FVector Vel = FVector::ZeroVector;
	if (auto* FPM = SurvivorPawn->GetComponentByClass<UFloatingPawnMovement>())
		Vel = FPM->Velocity;
	GEngine->AddOnScreenDebugMessage(3, 0.f, FColor::Green,
		FString::Printf(TEXT("Speed=%.0f | Stuck=%.1fs"), Vel.Size(), StuckTimer));

	if (CurrentState == ESurvivorState::Explore && Blackboard->bWanderTargetValid)
	{
		DrawDebugLine(World, MyLoc, Blackboard->WanderTarget, FColor::White, false, -1.f, 0, 2.f);
		DrawDebugSphere(World, Blackboard->WanderTarget, 60.f, 8, FColor::White, false, -1.f);
	}
	if (CurrentState == ESurvivorState::SeekItem && IsValid(SeekTarget))
	{
		DrawDebugLine(World, MyLoc, SeekTarget->GetActorLocation(), FColor::Orange, false, -1.f, 0, 3.f);
		DrawDebugSphere(World, SeekTarget->GetActorLocation(), 60.f, 8, FColor::Orange, false, -1.f);
	}
	if (CurrentState == ESurvivorState::Flee)
	{
		DrawDebugLine(World, MyLoc, CurrentFleeTarget, FColor::Red, false, -1.f, 0, 3.f);
	}
	if (Blackboard->NearestZombieDistance < 1e6f)
	{
		GEngine->AddOnScreenDebugMessage(5, 0.f, FColor::Red,
			FString::Printf(TEXT("Nearest zombie: %.0f u"), Blackboard->NearestZombieDistance));
	}

	// Draw avoidance probe rays
	FVector const Forward = SurvivorPawn->GetActorForwardVector();
	float const Angles[] = { 0.f, -ProbeAngle, ProbeAngle, -ProbeAngle * 0.5f, ProbeAngle * 0.5f };
	for (float Angle : Angles)
	{
		FVector const ProbeDir = Forward.RotateAngleAxis(Angle, FVector::UpVector);
		DrawDebugLine(World, MyLoc, MyLoc + ProbeDir * ProbeLength,
			FColor::Magenta, false, -1.f, 0, 1.f);
	}
}
