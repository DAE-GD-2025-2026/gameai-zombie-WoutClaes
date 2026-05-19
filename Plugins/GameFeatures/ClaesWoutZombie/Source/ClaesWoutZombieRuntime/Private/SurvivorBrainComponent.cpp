// Fill out your copyright notice in the Description page of Project Settings.

#include "SurvivorBrainComponent.h"
#include "SurvivorAIController.h"
#include "Survivor/SurvivorPawn.h"
#include "Common/HealthComponent.h"
#include "Common/StaminaComponent.h"
#include "Common/InventoryComponent.h"
#include "Items/BaseItem.h"
#include "Items/ItemType.h"
#include "Items/Weapon.h"
#include "GameFramework/FloatingPawnMovement.h"
#include "AIController.h"
#include "DrawDebugHelpers.h"


USurvivorBrainComponent::USurvivorBrainComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	UE_LOG(LogTemp, Log, TEXT("[Brain] Constructor called."));
}

void USurvivorBrainComponent::BeginPlay()
{
	Super::BeginPlay();

	UE_LOG(LogTemp, Warning, TEXT("[Brain] BeginPlay START. Owner: %s"),
		GetOwner() ? *GetOwner()->GetName() : TEXT("NULL"));

	AIController = Cast<ASurvivorAIController>(GetOwner());
	if (!AIController)
	{
		UE_LOG(LogTemp, Error, TEXT("[Brain] FAIL: Owner is not ASurvivorAIController. Tick disabled."));
		SetComponentTickEnabled(false);
		return;
	}
	UE_LOG(LogTemp, Warning, TEXT("[Brain] OK: AIController found."));

	SurvivorPawn = Cast<ASurvivorPawn>(AIController->GetPawn());
	if (!SurvivorPawn)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Brain] Pawn not possessed yet at BeginPlay - will retry in Tick."));
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[Brain] OK: SurvivorPawn found: %s"), *SurvivorPawn->GetName());

		// Verify FloatingPawnMovement
		if (SurvivorPawn->GetComponentByClass<UFloatingPawnMovement>())
		{
			UE_LOG(LogTemp, Warning, TEXT("[Brain] OK: FloatingPawnMovement found on pawn."));
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("[Brain] FAIL: No FloatingPawnMovement on pawn! Movement will not work."));
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("[Brain] BeginPlay END. Tick enabled: %s"),
		IsComponentTickEnabled() ? TEXT("YES") : TEXT("NO"));
}

// ---------------------------------------------------------------------------
// Main Tick
// ---------------------------------------------------------------------------

void USurvivorBrainComponent::TickComponent(float DeltaTime, ELevelTick TickType,
                                             FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// --- Guard: retry pawn if not possessed yet ---
	if (!SurvivorPawn)
	{
		if (AIController)
		{
			SurvivorPawn = Cast<ASurvivorPawn>(AIController->GetPawn());
			if (SurvivorPawn)
				UE_LOG(LogTemp, Warning, TEXT("[Brain] Pawn acquired in Tick: %s"), *SurvivorPawn->GetName());
		}
	}

	if (!AIController || !SurvivorPawn || !Blackboard)
	{
		// Print once per second so we don't spam
		DebugNoInitTimer += DeltaTime;
		if (DebugNoInitTimer >= 1.0f)
		{
			DebugNoInitTimer = 0.f;
			UE_LOG(LogTemp, Error, TEXT("[Brain] NOT READY - AIController:%s SurvivorPawn:%s Blackboard:%s"),
				AIController  ? TEXT("OK") : TEXT("NULL"),
				SurvivorPawn  ? TEXT("OK") : TEXT("NULL"),
				Blackboard    ? TEXT("OK") : TEXT("NULL"));

			if (GEngine)
				GEngine->AddOnScreenDebugMessage(1, 1.f, FColor::Red,
					FString::Printf(TEXT("[Brain] NOT READY - Ctrl:%s Pawn:%s BB:%s"),
						AIController ? TEXT("OK") : TEXT("NULL"),
						SurvivorPawn ? TEXT("OK") : TEXT("NULL"),
						Blackboard   ? TEXT("OK") : TEXT("NULL")));
		}
		return;
	}

	// --- Normal tick ---
	LastDeltaTime = DeltaTime; // stored for use in MoveToward
	RefreshSelfState();
	Blackboard->ClearStaleEntries();
	RefreshNearestZombie();
	RefreshClosestItems();
	PickupNearbyItems();

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
// Debug HUD + visuals
// ---------------------------------------------------------------------------

void USurvivorBrainComponent::DrawDebugInfo()
{
	if (!GEngine || !SurvivorPawn) return;

	FVector const MyLoc = SurvivorPawn->GetActorLocation();
	UWorld* World = GetWorld();

	// Line 1: State + vitals
	GEngine->AddOnScreenDebugMessage(1, 0.f, FColor::Cyan,
		FString::Printf(TEXT("State: %s | HP: %.0f%% | Stamina: %.0f%%"),
			*SurvivorStateToString(CurrentState),
			Blackboard->HealthRatio  * 100.f,
			Blackboard->StaminaRatio * 100.f));

	// Line 2: Perceived counts
	GEngine->AddOnScreenDebugMessage(2, 0.f, FColor::Yellow,
		FString::Printf(TEXT("Known: Zombies=%d  Items=%d  Purge=%d | HasWeapon=%s"),
			Blackboard->KnownZombies.Num(),
			Blackboard->KnownItems.Num(),
			Blackboard->KnownPurgeZones.Num(),
			Blackboard->bHasWeapon ? TEXT("YES") : TEXT("NO")));

	// Line 3: Movement info
	FVector Vel = FVector::ZeroVector;
	if (auto* FPM = SurvivorPawn->GetComponentByClass<UFloatingPawnMovement>())
		Vel = FPM->Velocity;

	GEngine->AddOnScreenDebugMessage(3, 0.f, FColor::Green,
		FString::Printf(TEXT("Velocity: X=%.1f Y=%.1f Z=%.1f (speed=%.1f)"),
			Vel.X, Vel.Y, Vel.Z, Vel.Size()));

	// Line 4: Current wander/seek target
	if (CurrentState == ESurvivorState::Explore && Blackboard->bWanderTargetValid)
	{
		GEngine->AddOnScreenDebugMessage(4, 0.f, FColor::White,
			FString::Printf(TEXT("WanderTarget: X=%.0f Y=%.0f (dist=%.0f)"),
				Blackboard->WanderTarget.X, Blackboard->WanderTarget.Y,
				FVector::Dist(MyLoc, Blackboard->WanderTarget)));

		// Draw line to wander target
		DrawDebugLine(World, MyLoc, Blackboard->WanderTarget, FColor::White, false, -1.f, 0, 3.f);
		DrawDebugSphere(World, Blackboard->WanderTarget, 50.f, 8, FColor::White, false, -1.f);
	}

	if (CurrentState == ESurvivorState::SeekItem && IsValid(SeekTarget))
	{
		GEngine->AddOnScreenDebugMessage(4, 0.f, FColor::Orange,
			FString::Printf(TEXT("SeekTarget: %s (dist=%.0f)"),
				*SeekTarget->GetName(),
				FVector::Dist(MyLoc, SeekTarget->GetActorLocation())));

		DrawDebugLine(World, MyLoc, SeekTarget->GetActorLocation(), FColor::Orange, false, -1.f, 0, 3.f);
	}

	// Line 5: Nearest zombie
	if (Blackboard->NearestZombieDistance < 1e6f)
	{
		GEngine->AddOnScreenDebugMessage(5, 0.f, FColor::Red,
			FString::Printf(TEXT("Nearest zombie: %.0f units"), Blackboard->NearestZombieDistance));

		FVector const ZombiePos = MyLoc + Blackboard->NearestZombieDirection * Blackboard->NearestZombieDistance;
		DrawDebugLine(World, MyLoc, ZombiePos, FColor::Red, false, -1.f, 0, 2.f);
	}

	// Draw pawn's sight radius
	DrawDebugCone(World, MyLoc,
		SurvivorPawn->GetActorForwardVector(),
		1000.f,                     // sight radius from SurvivorPawn constructor
		FMath::DegreesToRadians(70.f),
		FMath::DegreesToRadians(0.f),
		12, FColor::Cyan, false, -1.f, 0, 1.f);
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
		FleeTimeWithoutThreat = 0.f;
		break;
	case ESurvivorState::SeekItem:
		SeekTarget = SelectBestItem();
		UE_LOG(LogTemp, Log, TEXT("[Brain] SeekItem target: %s"),
			SeekTarget ? *SeekTarget->GetName() : TEXT("NONE"));
		break;
	default:
		break;
	}
}

ESurvivorState USurvivorBrainComponent::EvaluateTransitions() const
{
	if (Blackboard->HealthRatio < LowHealthThreshold && Blackboard->MedkitSlotIdx >= 0)
		return ESurvivorState::UseMedkit;

	const bool bZombieTooClose = Blackboard->NearestZombieDistance < FleeRadius;
	const bool bZombieNearby   = Blackboard->NearestZombieDistance < FightRadius;

	if (bZombieTooClose)
	{
		if (Blackboard->HealthRatio < SafeHealthThreshold || !Blackboard->bHasWeapon)
			return ESurvivorState::Flee;
		return ESurvivorState::Fight;
	}

	if (bZombieNearby && Blackboard->bHasWeapon && Blackboard->HealthRatio >= SafeHealthThreshold)
		return ESurvivorState::Fight;

	if (CurrentState == ESurvivorState::UseMedkit)
		return ESurvivorState::UseMedkit;

	if (CurrentState == ESurvivorState::Flee)
	{
		if (Blackboard->KnownZombies.IsEmpty())
			return PreviousState == ESurvivorState::Flee ? ESurvivorState::Explore : PreviousState;
		return ESurvivorState::Flee;
	}

	if (SelectBestItem() != nullptr)
		return ESurvivorState::SeekItem;

	return ESurvivorState::Explore;
}

// ---------------------------------------------------------------------------
// State behaviours
// ---------------------------------------------------------------------------

void USurvivorBrainComponent::TickExplore(float DeltaTime)
{
	FVector const MyLoc = SurvivorPawn->GetActorLocation();

	if (!Blackboard->bWanderTargetValid ||
		FVector::Dist(MyLoc, Blackboard->WanderTarget) < ReachedTargetRadius)
	{
		PickNewWanderTarget();
		UE_LOG(LogTemp, Log, TEXT("[Brain] New wander target: X=%.0f Y=%.0f"),
			Blackboard->WanderTarget.X, Blackboard->WanderTarget.Y);
	}

	SurvivorPawn->StopRunning();
	MoveToward(Blackboard->WanderTarget, false);
}

void USurvivorBrainComponent::TickSeekItem(float DeltaTime)
{
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
		auto* Inventory = SurvivorPawn->GetComponentByClass<UInventoryComponent>();
		if (Inventory)
		{
			for (int i = 0; i < Inventory->GetInventoryCapacity(); ++i)
			{
				if (Inventory->GetInventory()[i] == nullptr)
				{
					bool bGrabbed = Inventory->GrabItem(i, SeekTarget);
					UE_LOG(LogTemp, Log, TEXT("[Brain] GrabItem slot %d: %s"), i, bGrabbed ? TEXT("OK") : TEXT("FAIL"));
					break;
				}
			}
		}
		SeekTarget = nullptr;
		TransitionTo(ESurvivorState::Explore);
		return;
	}

	bool const bUrgent = Blackboard->NearestZombieDistance < FightRadius * 1.5f;
	MoveToward(ItemLoc, bUrgent);
}

void USurvivorBrainComponent::TickFight(float DeltaTime)
{
	if (!Blackboard->bHasWeapon)
	{
		TransitionTo(ESurvivorState::Flee);
		return;
	}

	APawn* Target = nullptr;
	float BestDist = TNumericLimits<float>::Max();
	FVector const MyLoc = SurvivorPawn->GetActorLocation();

	for (auto const& Weak : Blackboard->KnownZombies)
	{
		if (!Weak.IsValid()) continue;
		float const D = FVector::Dist(MyLoc, Weak->GetActorLocation());
		if (D < BestDist) { BestDist = D; Target = Weak.Get(); }
	}

	if (!Target)
	{
		TransitionTo(ESurvivorState::Explore);
		return;
	}

	FVector const ToTarget = (Target->GetActorLocation() - MyLoc).GetSafeNormal();
	SurvivorPawn->SetActorRotation(FMath::RInterpTo(
		SurvivorPawn->GetActorRotation(), ToTarget.Rotation(), DeltaTime, 8.f));

	if (BestDist > ShootRadius)
	{
		MoveToward(Target->GetActorLocation(), false);
	}
	else
	{
		StopMovement();
		TryShootToward(ToTarget);
	}
}

void USurvivorBrainComponent::TickFlee(float DeltaTime)
{
	if (Blackboard->KnownZombies.IsEmpty())
	{
		FleeTimeWithoutThreat += DeltaTime;
		if (FleeTimeWithoutThreat >= FleeTimeoutDuration)
		{
			TransitionTo(ESurvivorState::Explore);
			return;
		}
	}
	else
	{
		FleeTimeWithoutThreat = 0.f;
	}

	FVector const MyLoc = SurvivorPawn->GetActorLocation();
	FVector FleeDir     = -Blackboard->NearestZombieDirection;

	for (auto const& Weak : Blackboard->KnownPurgeZones)
	{
		if (!Weak.IsValid()) continue;
		FVector const ToPurge = Weak->GetActorLocation() - MyLoc;
		float   const Dist    = ToPurge.Size();
		if (Dist < 600.f && Dist > 1.f)
			FleeDir -= ToPurge.GetSafeNormal() * (600.f - Dist) / 600.f;
	}

	if (FleeDir.IsNearlyZero())
		FleeDir = SurvivorPawn->GetActorForwardVector() * -1.f;
	FleeDir.Z = 0.f;
	FleeDir.Normalize();

	bool const bShouldRun = Blackboard->StaminaRatio > LowStaminaThreshold;
	MoveToward(MyLoc + FleeDir * 800.f, bShouldRun);
}

void USurvivorBrainComponent::TickUseMedkit(float DeltaTime)
{
	MedkitUseTimer += DeltaTime;
	if (MedkitUseTimer < MedkitUseDelay) return;

	auto* Inventory = SurvivorPawn->GetComponentByClass<UInventoryComponent>();
	if (Inventory && Blackboard->MedkitSlotIdx >= 0)
	{
		bool bUsed = Inventory->UseItem(Blackboard->MedkitSlotIdx);
		UE_LOG(LogTemp, Log, TEXT("[Brain] UseItem(medkit slot %d): %s"), Blackboard->MedkitSlotIdx, bUsed ? TEXT("OK") : TEXT("FAIL"));
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
				if (Blackboard->WeaponSlotIdx < 0) { Blackboard->bHasWeapon = true; Blackboard->WeaponSlotIdx = i; }
				break;
			case EItemType::Medkit:
				if (Blackboard->MedkitSlotIdx < 0) Blackboard->MedkitSlotIdx = i;
				break;
			case EItemType::Food:
				if (Blackboard->FoodSlotIdx < 0) Blackboard->FoodSlotIdx = i;
				break;
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

	for (auto const& Weak : Blackboard->KnownZombies)
	{
		if (!Weak.IsValid()) continue;
		FVector const Delta = Weak->GetActorLocation() - MyLoc;
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

	for (auto const& Weak : Blackboard->KnownItems)
	{
		if (!Weak.IsValid() || Weak->GetValue() <= 0) continue;
		float const D = FVector::Dist(MyLoc, Weak->GetActorLocation());
		switch (Weak->GetItemType())
		{
		case EItemType::Medkit:
			if (D < BestMedkit) { BestMedkit = D; Blackboard->ClosestMedkit = Weak; } break;
		case EItemType::Food:
			if (D < BestFood)   { BestFood = D;   Blackboard->ClosestFood   = Weak; } break;
		case EItemType::Pistol:
		case EItemType::Shotgun:
			if (D < BestWeapon) { BestWeapon = D; Blackboard->ClosestWeapon = Weak; } break;
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

	for (auto const& Weak : Blackboard->KnownItems)
	{
		if (!Weak.IsValid() || Weak->GetValue() <= 0) continue;
		if (FVector::Dist(MyLoc, Weak->GetActorLocation()) <= ItemPickupRadius)
		{
			if (Inventory->GrabItem(FreeSlot, Weak.Get()))
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

void USurvivorBrainComponent::MoveToward(FVector const& Target, bool bRun)
{
	if (bRun) SurvivorPawn->StartRunning();
	else      SurvivorPawn->StopRunning();

	FVector const MyLoc = SurvivorPawn->GetActorLocation();
	FVector const Delta = Target - MyLoc;

	if (Delta.SizeSquared() < ReachedTargetRadius * ReachedTargetRadius)
		return;

	// FloatingPawnMovement is driven by AddMovementInput
	FVector Dir = Delta;
	Dir.Z = 0.f;
	Dir.Normalize();
	SurvivorPawn->AddMovementInput(Dir, 1.0f);

	// FloatingPawnMovement does not auto-rotate toward movement — do it manually
	FRotator const TargetRot = Dir.Rotation();
	FRotator const NewRot    = FMath::RInterpTo(
		SurvivorPawn->GetActorRotation(), TargetRot, LastDeltaTime, TurnInterpSpeed);
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
	FVector const MyLoc   = SurvivorPawn->GetActorLocation();
	FVector const RandDir = FMath::VRand();
	FVector Dir = FVector(RandDir.X, RandDir.Y, 0.f);
	Dir.Normalize();
	Blackboard->WanderTarget       = MyLoc + Dir * WanderRadius;
	Blackboard->bWanderTargetValid = true;
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
