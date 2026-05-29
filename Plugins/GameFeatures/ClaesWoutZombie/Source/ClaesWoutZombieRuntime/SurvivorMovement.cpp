#include "SurvivorMovement.h"
#include "AIController.h"
#include "NavigationSystem.h"
#include "Items/Food.h"
#include "Items/Medkit.h"
#include "Items/Pistol.h"
#include "Navigation/PathFollowingComponent.h"

USurvivorMovement::USurvivorMovement()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void USurvivorMovement::BeginPlay()
{
	Super::BeginPlay();
	
	Inventory = GetOwner()->FindComponentByClass<UInventoryComponent>();
	
	Health = GetOwner()->FindComponentByClass<UHealthComponent>();
}

void USurvivorMovement::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
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
void USurvivorMovement::TickWander(float DeltaTime)
{
	APawn* Pawn = Cast<APawn>(GetOwner());
	if (!Pawn)
		return;

	AAIController* AIC = Cast<AAIController>(Pawn->GetController());
	if (!AIC)
		return;

	if (AIC->GetMoveStatus() == EPathFollowingStatus::Idle)
	{
		PickNewWanderTarget();
	}
}

void USurvivorMovement::PickNewWanderTarget()
{
	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (!OwnerPawn)
		return;
 
	AAIController* AIC = Cast<AAIController>(OwnerPawn->GetController());
	if (!AIC)
		return;
 
	UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
	if (!NavSys)
		return;
 
	FNavLocation NavLoc;
	if (NavSys->GetRandomReachablePointInRadius(OwnerPawn->GetActorLocation(), WanderRadius, NavLoc))
	{
		AIC->MoveToLocation(NavLoc.Location, AcceptanceRadius);
	}
}

//========================
// House Exploring
//========================
void USurvivorMovement::TickExploreHouse(float DeltaTime)
{
	if (!CurrentHouse)
	{
		State = ESurvivorState::Wander;
		return;
	}

	ExploreHouseTimer += DeltaTime;
	if (ExploreHouseTimer >= MaxExploreHouseTime)
	{
		State = ESurvivorState::ExitHouse;
		ExitHouseTimer = 0.f;
		return;
	}
	
	APawn* Pawn = Cast<APawn>(GetOwner());
	if (!Pawn)
		return;

	AAIController* AIC = Cast<AAIController>(Pawn->GetController());
	if (!AIC)
		return;

	FVector HouseCenter;
	FVector Extents;
	CurrentHouse->GetActorBounds(true, HouseCenter, Extents);

	float DistSq = FVector::DistSquared(Pawn->GetActorLocation(), HouseCenter);
	if (DistSq <= HouseAcceptanceRadius * HouseAcceptanceRadius)
	{
		State = ESurvivorState::ExitHouse;
		AIC->MoveToLocation(HouseExitLocation, 150.f);
	}
}

void USurvivorMovement::StartExploringHouse(AActor* House)
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

void USurvivorMovement::MoveToHouseCenter()
{
	APawn* Pawn = Cast<APawn>(GetOwner());
	if (!Pawn)
		return;

	AAIController* AIC = Cast<AAIController>(Pawn->GetController());
	if (!AIC)
		return;

	FVector HouseCenter;
	FVector Extents;
	CurrentHouse->GetActorBounds(true, HouseCenter, Extents);

	AIC->MoveToLocation(HouseCenter, HouseAcceptanceRadius);
}

//========================
// House Exiting
//========================
void USurvivorMovement::TickExitHouse(float DeltaTime)
{
	ExitHouseTimer += DeltaTime;

	if (ExitHouseTimer >= MaxExitHouseTime)
	{
		State = ESurvivorState::Wander;
		CurrentHouse = nullptr;
		return;
	}
	
	APawn* Pawn = Cast<APawn>(GetOwner());
	if (!Pawn)
		return;

	AAIController* AIC = Cast<AAIController>(Pawn->GetController());
	if (!AIC)
		return;

	float DistSq = FVector::DistSquared(Pawn->GetActorLocation(), HouseExitLocation);
	if (DistSq <= 150.f * 150.f)
	{
		State = ESurvivorState::Wander;
		CurrentHouse = nullptr;
		return;
	}
	
	AIC->MoveToLocation(HouseExitLocation, 150.f);
}

//========================
// Pickup Item
//========================
void USurvivorMovement::StartPickingUpItem(ABaseItem* Item)
{
	if (!Item || PickedUpItems.Contains(Item))
		return;

	CurrentItem = Item;
	State = ESurvivorState::PickupItem;
	PickupTimer = 0.f;
	
	if (Inventory)
		ItemPickupRadius = Inventory->GetPickupRange();
	else
		ItemPickupRadius = 100.f;

	APawn* Pawn = Cast<APawn>(GetOwner());
	if (AAIController* AIC = Cast<AAIController>(Pawn->GetController()))
	{
		AIC->MoveToActor(Item, ItemPickupRadius);
	}
}

bool USurvivorMovement::ShouldPickUpItem(ABaseItem* Item)
{
	if (!Item || !Inventory)
		return false;

	return true;
}

void USurvivorMovement::TickPickupItem(float DeltaTime)
{
	PickupTimer += DeltaTime;
	if (PickupTimer >= MaxPickupTime)
	{
		CurrentItem = nullptr;
		State = ESurvivorState::Wander;
		return;
	}
	
	if (!CurrentItem)
	{
		State = ESurvivorState::Wander;
		return;
	}

	APawn* Pawn = Cast<APawn>(GetOwner());
	AAIController* AIC = Cast<AAIController>(Pawn->GetController());

	GEngine->AddOnScreenDebugMessage(-1, 0.f, FColor::Yellow,
	FString::Printf(TEXT("Dist: %.1f / PickupRadius: %.1f"),
	FVector::Dist(Pawn->GetActorLocation(), CurrentItem->GetActorLocation()),
	ItemPickupRadius));

	
	float DistSq = FVector::DistSquared(Pawn->GetActorLocation(), CurrentItem->GetActorLocation());
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

		PickedUpItems.Add(CurrentItem);
		CurrentItem = nullptr;
		State = ESurvivorState::Wander;
		return;
	}

	AIC->MoveToLocation(CurrentItem->GetActorLocation());
}

//========================
// Inventory
//========================
void USurvivorMovement::TryUseInventory()
{
	if (!Inventory || !Health)
		return;

	const TArray<ABaseItem*>& Items = Inventory->GetInventory();

	for (int32 i = 0; i < Items.Num(); ++i)
	{
		ABaseItem* Item = Items[i];
		if (!Item)
			continue;

		if (Item->IsA(AMedkit::StaticClass()))
		{
			if (Health->GetHealth() <= Health->GetMaxHealth() * 0.5f)
			{
				Inventory->UseItem(i);
				return;
			}
		}
	}
}
