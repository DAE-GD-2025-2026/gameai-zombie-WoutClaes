#include "SurvivorMovementClaesWout.h"
#include "AIController.h"
#include "NavigationSystem.h"
#include "Items/Food.h"
#include "Items/Medkit.h"
#include "Items/Pistol.h"
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
	
	TryUseInventory();

	switch (State)
	{
	case ESurvivorState::Wander:
		GEngine->AddOnScreenDebugMessage(-1, 0.f, FColor::Red, TEXT("STATE: Wander"));
		TickWander(DeltaTime);
		break;

	case ESurvivorState::ExploreHouse:
		GEngine->AddOnScreenDebugMessage(-1, 0.f, FColor::Yellow, TEXT("STATE: ExploreHouse"));
		TickExploreHouse(DeltaTime);
		break;
	case ESurvivorState::ExitHouse:
		GEngine->AddOnScreenDebugMessage(-1, 0.f, FColor::Blue, TEXT("STATE: ExitHouse"));
		TickExitHouse(DeltaTime);
		break;
	case ESurvivorState::PickupItem:
		GEngine->AddOnScreenDebugMessage(-1, 0.f, FColor::Green, TEXT("STATE: PickupItem"));
		TickPickupItem(DeltaTime);
		break;

	}
}

//========================
// Wander
//========================
void USurvivorMovementClaesWout::TickWander(float DeltaTime)
{
	if (!MyPawn)
		return;

	if (!MyAIController) 
		return;

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
	if (!House)
		return;
	
	if (VisitedHouses.Contains(House))
		return;
	
	VisitedHouses.Add(House);
	CurrentHouse = House;
	State = ESurvivorState::ExploreHouse;
	ExploreHouseTimer = 0.f;
	
	EntranceLocation = GetOwner()->GetActorLocation();
	HouseExitLocation = EntranceLocation;
	
	MoveToHouseCenter();
}

void USurvivorMovementClaesWout::MoveToHouseCenter()
{
	if (!MyPawn)
		return;

	if (!MyAIController) return;

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
		
		if (InvItem->IsA(APistol::StaticClass())) WeaponCount++;
		else if (InvItem->IsA(AFood::StaticClass())) FoodCount++;
		else if (InvItem->IsA(AMedkit::StaticClass())) MedkitCount++;
	}

	if (EmptySlots == 0) return false; 

	if (Item->IsA(APistol::StaticClass()) && WeaponCount >= 1) return false;
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
void USurvivorMovementClaesWout::TryUseInventory()
{
	if (!Inventory)
		return;

	const TArray<ABaseItem*>& Items = Inventory->GetInventory();

	for (int32 i = 0; i < Items.Num(); ++i)
	{
		ABaseItem* Item = Items[i];
		if (!Item || Item->GetValue() <= 0) 
			continue;

		if (Health && Item->IsA(AMedkit::StaticClass()))
		{
			if (Health->GetHealth() <= Health->GetMaxHealth() * 0.5f)
			{
				if (Inventory->UseItem(i))
				{
					Inventory->RemoveItem(i);
					return;
				}
			}
		}
		
		if (Stamina && Item->IsA(AFood::StaticClass()))
		{
			if (Stamina->GetCurrentStamina() <= Stamina->GetMaxStamina() * 0.3f)
			{
				if (Inventory->UseItem(i))
				{
					Inventory->RemoveItem(i);
					return;
				}
			}
		}
	}
}
