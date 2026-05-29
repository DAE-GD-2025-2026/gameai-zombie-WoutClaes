#include "SurvivorMovementClaesWout.h"
#include "AIController.h"
#include "NavigationSystem.h"
#include "Survivor/SurvivorPawn.h"
#include "Items/Weapon.h"
#include "Items/Food.h"
#include "Items/Medkit.h"
#include "Navigation/PathFollowingComponent.h"

USurvivorMovementClaesWout::USurvivorMovementClaesWout()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void USurvivorMovementClaesWout::BeginPlay()
{
	Super::BeginPlay();
	
	Inventory = GetOwner()->FindComponentByClass<UInventoryComponent>();
	Health = GetOwner()->FindComponentByClass<UHealthComponent>();
	Stamina = GetOwner()->FindComponentByClass<UStaminaComponent>();
	
	MyPawn = Cast<APawn>(GetOwner());
	if (MyPawn)
	{
		MyAIController = Cast<AAIController>(MyPawn->GetController());
	}
}

void USurvivorMovementClaesWout::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	
	TryUseInventory(DeltaTime);

	switch (State)
	{
	case ESurvivorState::Wander:
		GEngine->AddOnScreenDebugMessage(-1, 0.f, FColor::Red, TEXT("STATE: Wander"));
		TickWander(DeltaTime);
		break;

	case ESurvivorState::ExploreHouse:
		GEngine->AddOnScreenDebugMessage(-1, 0.f, FColor::Yellow, TEXT("STATE: Explore House"));
		TickExploreHouse(DeltaTime);
		break;
	case ESurvivorState::ExitHouse:
		GEngine->AddOnScreenDebugMessage(-1, 0.f, FColor::Blue, TEXT("STATE: Exit House"));
		TickExitHouse(DeltaTime);
		break;
	case ESurvivorState::PickupItem:
		GEngine->AddOnScreenDebugMessage(-1, 0.f, FColor::Green, TEXT("STATE: Pickup Item"));
		TickPickupItem(DeltaTime);
		break;
	case ESurvivorState::Flee:
		GEngine->AddOnScreenDebugMessage(-1, 0.f, FColor::Orange, TEXT("STATE: Fleeing Zombie!"));
		TickFlee(DeltaTime);
		break;
	case ESurvivorState::Combat:
		GEngine->AddOnScreenDebugMessage(-1, 0.f, FColor::Purple, TEXT("STATE: Combat Shooting!"));
		TickCombat(DeltaTime);
		break;
	}
}

//========================
// Wander
//========================
void USurvivorMovementClaesWout::TickWander(float DeltaTime)
{
	if (!MyPawn || !MyAIController)
		return;

	SpinAngle += SpinSpeed * DeltaTime;
	if (SpinAngle > 360.f) SpinAngle -= 360.f;
	
	FVector SpinDirection = FRotator(0.f, SpinAngle, 0.f).Vector();
	FVector FocalPoint = MyPawn->GetActorLocation() + (SpinDirection * 1000.f);
	
	MyAIController->SetFocalPoint(FocalPoint);
	
	if (MyAIController->GetMoveStatus() == EPathFollowingStatus::Idle)
	{
		PickNewWanderTarget();
	}
}

void USurvivorMovementClaesWout::PickNewWanderTarget()
{
	if (!MyPawn || !MyAIController)
		return;
 
	UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
	if (!NavSys)
		return;
 
	// Project a point forward
	FVector ForwardPoint = MyPawn->GetActorLocation() + (MyPawn->GetActorForwardVector() * WanderForwardDistance);
	
	FNavLocation NavLoc;
	// Try to find a point around the forward projection
	if (NavSys->GetRandomReachablePointInRadius(ForwardPoint, WanderForwardRadius, NavLoc))
	{
		MyAIController->MoveToLocation(NavLoc.Location, AcceptanceRadius);
	}
	else // Fallback: if we hit a wall, wander around current location
	{
		if (NavSys->GetRandomReachablePointInRadius(MyPawn->GetActorLocation(), WanderRadius, NavLoc))
		{
			MyAIController->MoveToLocation(NavLoc.Location, AcceptanceRadius);
		}
	}
}

//========================
// House Exploring
//========================
void USurvivorMovementClaesWout::TickExploreHouse(float DeltaTime)
{
	if (!CurrentHouse || !MyPawn)
	{
		State = ESurvivorState::Wander;
		return;
	}

	ExploreHouseTimer += DeltaTime;
	if (ExploreHouseTimer >= MaxExploreHouseTime)
	{
		State = ESurvivorState::ExitHouse;
		ExitHouseTimer = 0.f;
		if (MyAIController) MyAIController->MoveToLocation(HouseExitLocation, 150.f); // Call once!
		return;
	}

	FVector HouseCenter;
	FVector Extents;
	CurrentHouse->GetActorBounds(true, HouseCenter, Extents);

	float DistSq = FVector::DistSquared(MyPawn->GetActorLocation(), HouseCenter);
	if (DistSq <= HouseAcceptanceRadius * HouseAcceptanceRadius)
	{
		State = ESurvivorState::ExitHouse;
		if (MyAIController) MyAIController->MoveToLocation(HouseExitLocation, 150.f); // Call once!
	}
}

void USurvivorMovementClaesWout::StartExploringHouse(AActor* House)
{
	if (!House || VisitedHouses.Contains(House))
		return;
	
	VisitedHouses.Add(House);
	CurrentHouse = House;
	State = ESurvivorState::ExploreHouse;
	ExploreHouseTimer = 0.f;
	
	EntranceLocation = GetOwner()->GetActorLocation();
	HouseExitLocation = EntranceLocation;
	
	if (MyAIController) MyAIController->ClearFocus(EAIFocusPriority::Gameplay);
	
	MoveToHouseCenter();
}

void USurvivorMovementClaesWout::MoveToHouseCenter()
{
	if (!MyPawn || !MyAIController)
		return;

	FVector HouseCenter;
	FVector Extents;
	CurrentHouse->GetActorBounds(true, HouseCenter, Extents);

	MyAIController->MoveToLocation(HouseCenter, HouseAcceptanceRadius);
}

//========================
// House Exiting
//========================
void USurvivorMovementClaesWout::TickExitHouse(float DeltaTime)
{
	ExitHouseTimer += DeltaTime;

	if (ExitHouseTimer >= MaxExitHouseTime)
	{
		State = ESurvivorState::Wander;
		CurrentHouse = nullptr;
		return;
	}
	
	if (!MyPawn)
		return;

	float DistSq = FVector::DistSquared(MyPawn->GetActorLocation(), HouseExitLocation);
	if (DistSq <= 150.f * 150.f)
	{
		State = ESurvivorState::Wander;
		CurrentHouse = nullptr;
	}
}

//========================
// Pickup Item
//========================
void USurvivorMovementClaesWout::StartPickingUpItem(ABaseItem* Item)
{
	if (!Item || PickedUpItems.Contains(Item))
		return;

	PreviousState = State;
	
	CurrentItem = Item;
	State = ESurvivorState::PickupItem;
	PickupTimer = 0.f;
	
	if (Inventory)
		ItemPickupRadius = Inventory->GetPickupRange();
	else
		ItemPickupRadius = 100.f;

	if (MyAIController)
	{
		MyAIController->ClearFocus(EAIFocusPriority::Gameplay);
		MyAIController->MoveToActor(Item);
	}
}

bool USurvivorMovementClaesWout::ShouldPickUpItem(ABaseItem* Item)
{
	if (!Item || !Inventory) return false;
	
	if (Item->GetValue() <= 0) return false; 

	int WeaponCount = 0;
	int FoodCount = 0;
	int MedkitCount = 0;
	int EmptySlots = 0;

	for (ABaseItem* InvItem : Inventory->GetInventory())
	{
		if (!InvItem)
		{
			EmptySlots++;
			continue;
		}
		
		if (InvItem->IsA(AWeapon::StaticClass())) WeaponCount++;
		else if (InvItem->IsA(AFood::StaticClass())) FoodCount++;
		else if (InvItem->IsA(AMedkit::StaticClass())) MedkitCount++;
	}

	if (EmptySlots == 0) return false; 

	if (Item->IsA(AWeapon::StaticClass()) && WeaponCount >= 1) return false;
	if (Item->IsA(AFood::StaticClass()) && FoodCount >= 2) return false;
	if (Item->IsA(AMedkit::StaticClass()) && MedkitCount >= 2) return false;

	return true;
}

void USurvivorMovementClaesWout::TickPickupItem(float DeltaTime)
{
	PickupTimer += DeltaTime;
	if (PickupTimer >= MaxPickupTime)
	{
		CurrentItem = nullptr;
		State = PreviousState;
		return;
	}
	
	if (!CurrentItem || !MyPawn)
	{
		State = PreviousState;
		return;
	}

	GEngine->AddOnScreenDebugMessage(-1, 0.f, FColor::Yellow,
	FString::Printf(TEXT("Dist: %.1f / PickupRadius: %.1f"),
	FVector::Dist(MyPawn->GetActorLocation(), CurrentItem->GetActorLocation()),
	ItemPickupRadius));

	
	float DistSq = FVector::DistSquared(MyPawn->GetActorLocation(), CurrentItem->GetActorLocation());
	if (DistSq <= ItemPickupRadius * ItemPickupRadius)
	{
		if (Inventory)
		{
			int32 Capacity = Inventory->GetInventoryCapacity();
			for (int32 Slot = 0; Slot < Capacity; ++Slot)
			{
				if (Inventory->GetInventory()[Slot] == nullptr)
				{
					bool bSuccess = Inventory->GrabItem(Slot, CurrentItem);

					if (bSuccess)
					{
						PickedUpItems.Add(CurrentItem);
						
						GEngine->AddOnScreenDebugMessage(-1, 1.f, FColor::Green,
							FString::Printf(TEXT("Picked up item: %s"), *CurrentItem->GetName()));
					}
					else
					{
						GEngine->AddOnScreenDebugMessage(-1, 1.f, FColor::Red,
							TEXT("GrabItem failed"));
					}
					break;
				}
			}
		}

		CurrentItem = nullptr;
		State = PreviousState;
	}
}

//========================
// Inventory
//========================
void USurvivorMovementClaesWout::TryUseInventory(float DeltaTime)
{
	if (!Inventory) return;

	if (ItemCooldownTimer > 0.f)
	{
		ItemCooldownTimer -= DeltaTime;
		return;
	}

	const TArray<ABaseItem*>& Items = Inventory->GetInventory();

	for (int32 i = 0; i < Items.Num(); ++i)
	{
		ABaseItem* Item = Items[i];
		if (!Item || Item->GetValue() <= 0) 
			continue;

		bool bUsedItem = false;

		if (Health && Item->IsA(AMedkit::StaticClass()))
		{
			if (Health->GetHealth() <= Health->GetMaxHealth() * 0.5f)
			{
				bUsedItem = Inventory->UseItem(i);
			}
		}
		
		else if (Stamina && Item->IsA(AFood::StaticClass()))
		{
			if (Stamina->GetCurrentStamina() <= Stamina->GetMaxStamina() * 0.3f)
			{
				bUsedItem = Inventory->UseItem(i);
			}
		}

		if (bUsedItem)
		{
			ItemCooldownTimer = ItemUseCooldownDuration;

			if (Item->GetValue() <= 0)
			{
				Inventory->RemoveItem(i);
			}
			return;
		}
	}
}

//========================
// Zombie Engagement
//========================
bool USurvivorMovementClaesWout::HasWeapon() const
{
	if (!Inventory) return false;
	for (ABaseItem* InvItem : Inventory->GetInventory())
	{
		if (InvItem && InvItem->IsA(AWeapon::StaticClass()))
		{
			return true;
		}
	}
	return false;
}

int32 USurvivorMovementClaesWout::GetWeaponSlot() const
{
	if (!Inventory) return -1;
	const TArray<ABaseItem*>& Items = Inventory->GetInventory();

	for (int32 i = 0; i < Items.Num(); ++i)
	{
		ABaseItem* Item = Items[i];
		if (Item && Item->IsA(AWeapon::StaticClass()) && Item->GetValue() > 0)
		{
			return i;
		}
	}
	return -1;
}

void USurvivorMovementClaesWout::HandleZombieSpotted(AActor* Zombie)
{
	if (!Zombie) return;
	TargetZombie = Zombie;

	if (HasWeapon())
	{
		if (CanOverride(ESurvivorState::Combat))
		{
			State = ESurvivorState::Combat;
			if (MyAIController) MyAIController->SetFocus(TargetZombie);
			if (ASurvivorPawn* Survivor = Cast<ASurvivorPawn>(MyPawn)) Survivor->StartRunning();
		}
	}
	else
	{
		if (CanOverride(ESurvivorState::Flee))
		{
			State = ESurvivorState::Flee;
			FleeTimer = 4.0f;
			FleeDestination = FVector::ZeroVector;
			
			if (MyAIController) MyAIController->ClearFocus(EAIFocusPriority::Gameplay);
			if (ASurvivorPawn* Survivor = Cast<ASurvivorPawn>(MyPawn)) Survivor->StartRunning();
		}
	}
}

void USurvivorMovementClaesWout::TickFlee(float DeltaTime)
{
	if (!MyPawn || !MyAIController) return;

	FleeTimer -= DeltaTime;
	if (FleeTimer <= 0.f)
	{
		if (ASurvivorPawn* Survivor = Cast<ASurvivorPawn>(MyPawn)) Survivor->StopRunning();
		TargetZombie = nullptr;
		State = ESurvivorState::Wander;
		return;
	}

	if (HasWeapon() && IsValid(TargetZombie))
	{
		State = ESurvivorState::Combat;
		MyAIController->SetFocus(TargetZombie);
		return;
	}

	if (MyAIController->GetMoveStatus() == EPathFollowingStatus::Idle || FleeDestination.IsZero())
	{
		FVector ZombieLoc = IsValid(TargetZombie) ? TargetZombie->GetActorLocation() : MyPawn->GetActorLocation() - MyPawn->GetActorForwardVector() * 500.f;
		FVector MyLoc = MyPawn->GetActorLocation();

		FVector FleeDirection = (MyLoc - ZombieLoc).GetSafeNormal2D();
		if (FleeDirection.IsNearlyZero()) 
		{
			FleeDirection = -MyPawn->GetActorForwardVector();
		}

		UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
		if (NavSys)
		{
			FNavLocation NavLoc;
			bool bFoundValidPath = false;

			FVector TargetPoint = MyLoc + (FleeDirection * 2500.f);
			if (NavSys->ProjectPointToNavigation(TargetPoint, NavLoc, FVector(300.f, 300.f, 300.f)))
			{
				FleeDestination = NavLoc.Location;
				bFoundValidPath = true;
			}

			if (!bFoundValidPath)
			{
				FVector LeftDir = FleeDirection.RotateAngleAxis(45.f, FVector::UpVector);
				TargetPoint = MyLoc + (LeftDir * 1500.f);
				if (NavSys->ProjectPointToNavigation(TargetPoint, NavLoc, FVector(300.f, 300.f, 300.f)))
				{
					FleeDestination = NavLoc.Location;
					bFoundValidPath = true;
				}
			}

			if (!bFoundValidPath)
			{
				FVector RightDir = FleeDirection.RotateAngleAxis(-45.f, FVector::UpVector);
				TargetPoint = MyLoc + (RightDir * 1500.f);
				if (NavSys->ProjectPointToNavigation(TargetPoint, NavLoc, FVector(300.f, 300.f, 300.f)))
				{
					FleeDestination = NavLoc.Location;
					bFoundValidPath = true;
				}
			}

			if (!bFoundValidPath)
			{
				TargetPoint = MyLoc + (FleeDirection * 600.f);
				if (NavSys->ProjectPointToNavigation(TargetPoint, NavLoc, FVector(500.f, 500.f, 500.f)))
				{
					FleeDestination = NavLoc.Location;
					bFoundValidPath = true;
				}
			}

			if (bFoundValidPath)
			{
				MyAIController->MoveToLocation(FleeDestination, 50.f);
			}
		}
	}
}

void USurvivorMovementClaesWout::TickCombat(float DeltaTime)
{
	if (!MyPawn || !MyAIController) return;

	if (!bIsZombieVisible)
	{
		TimeSinceZombieSeen += DeltaTime;
		if (TimeSinceZombieSeen >= ZombieMemoryDuration)
		{
			TargetZombie = nullptr;
			MyAIController->ClearFocus(EAIFocusPriority::Gameplay);
			if (ASurvivorPawn* Survivor = Cast<ASurvivorPawn>(MyPawn)) Survivor->StopRunning();
			State = ESurvivorState::Wander;
			return;
		}
	}

	if (!IsValid(TargetZombie))
	{
		MyAIController->ClearFocus(EAIFocusPriority::Gameplay);
		if (ASurvivorPawn* Survivor = Cast<ASurvivorPawn>(MyPawn)) Survivor->StopRunning();
		State = ESurvivorState::Wander;
		return;
	}

	int32 WeaponSlot = GetWeaponSlot();
	if (WeaponSlot == -1)
	{
		MyAIController->ClearFocus(EAIFocusPriority::Gameplay);
		State = ESurvivorState::Flee;
		FleeTimer = 0.f;
		return;
	}

	if (MyAIController->GetMoveStatus() != EPathFollowingStatus::Idle)
	{
		MyAIController->StopMovement();
	}

	if (WeaponFireTimer > 0.f)
	{
		WeaponFireTimer -= DeltaTime;
	}
	else
	{
		bool bFired = Inventory->UseItem(WeaponSlot);
		if (bFired)
		{
			WeaponFireTimer = WeaponFireCooldownDuration;

			ABaseItem* WeaponItem = Inventory->GetInventory()[WeaponSlot];
			if (WeaponItem && WeaponItem->GetValue() <= 0)
			{
				Inventory->RemoveItem(WeaponSlot);
			}
		}
	}
}
