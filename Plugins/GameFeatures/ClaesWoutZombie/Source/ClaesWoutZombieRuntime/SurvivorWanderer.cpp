#include "SurvivorWanderer.h"
#include "AIController.h"
#include "NavigationSystem.h"
#include "Navigation/PathFollowingComponent.h"

USurvivorWanderer::USurvivorWanderer()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void USurvivorWanderer::BeginPlay()
{
	Super::BeginPlay();
}

void USurvivorWanderer::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (!OwnerPawn)
		return;

	AAIController* AIC = Cast<AAIController>(OwnerPawn->GetController());
	if (!AIC)
		return;

	// Only pick a new target when idle
	if (AIC->GetMoveStatus() == EPathFollowingStatus::Idle)
	{
		PickNewWanderTarget();
	}
}

void USurvivorWanderer::PickNewWanderTarget()
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
