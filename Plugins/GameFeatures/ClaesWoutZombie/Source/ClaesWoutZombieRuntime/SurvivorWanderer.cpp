// Fill out your copyright notice in the Description page of Project Settings.

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

	if (!bIsEnabled)
		return;

	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (!OwnerPawn)
		return;

	AAIController* AIC = Cast<AAIController>(OwnerPawn->GetController());
	if (!AIC)
		return;

	FVector const PawnLocation = OwnerPawn->GetActorLocation();
	float const SpeedSq = FVector::DistSquared(PawnLocation, LastLocation) / (DeltaTime * DeltaTime);
	LastLocation = PawnLocation;

	if (SpeedSq < 100.f) 
	{
		TimeStuck += DeltaTime;
		if (TimeStuck > StuckThreshold)
		{
			TimeStuck = 0.f;
			PickNewWanderTarget();
			return;
		}
	}
	else
	{
		TimeStuck = 0.f;
	}

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

	FVector const PawnLocation = OwnerPawn->GetActorLocation();
	FVector const PawnForward = OwnerPawn->GetActorForwardVector();

	FVector const SearchOrigin = PawnLocation + (PawnForward * (WanderRadius * 0.65f));

	FNavLocation NavLoc;
	if (NavSys->GetRandomReachablePointInRadius(SearchOrigin, WanderRadius * 0.75f, NavLoc))
	{
		AIC->MoveToLocation(NavLoc.Location, AcceptanceRadius);
	}
	else
	{
		if (NavSys->GetRandomReachablePointInRadius(PawnLocation, WanderRadius, NavLoc))
		{
			AIC->MoveToLocation(NavLoc.Location, AcceptanceRadius);
		}
	}
}

void USurvivorWanderer::SetEnabled(bool bEnabled)
{
	bIsEnabled = bEnabled;

	if (bIsEnabled)
		PickNewWanderTarget();
}
