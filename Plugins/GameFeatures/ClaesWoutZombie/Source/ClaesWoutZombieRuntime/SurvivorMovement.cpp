#include "SurvivorMovement.h"
#include "AIController.h"
#include "NavigationSystem.h"
#include "Navigation/PathFollowingComponent.h"

USurvivorMovement::USurvivorMovement()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void USurvivorMovement::BeginPlay()
{
	Super::BeginPlay();
}

void USurvivorMovement::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	switch (State)
	{
	case ESurvivorState::Wander:
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
