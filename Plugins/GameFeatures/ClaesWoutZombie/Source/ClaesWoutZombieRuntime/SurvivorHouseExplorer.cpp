// Fill out your copyright notice in the Description page of Project Settings.

#include "SurvivorHouseExplorer.h"
#include "AIController.h"
#include "Village/House/House.h"
#include "Navigation/PathFollowingComponent.h"
#include "NavigationSystem.h"

USurvivorHouseExplorer::USurvivorHouseExplorer()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void USurvivorHouseExplorer::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!bIsEnabled || !CurrentTarget)
		return;

	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (!OwnerPawn)
		return;

	FVector const PawnLocation = OwnerPawn->GetActorLocation();

	float const SpeedSq = FVector::DistSquared(PawnLocation, LastLocation) / (DeltaTime * DeltaTime);
	LastLocation = PawnLocation;

	if (SpeedSq < 100.f)
	{
		TimeStuck += DeltaTime;
		if (TimeStuck > StuckThreshold)
		{
			VisitedHouses.AddUnique(CurrentTarget);
			UnvisitedHouses.Remove(CurrentTarget);
			CurrentTarget = nullptr;
			TimeStuck = 0.f;
			
			if (UnvisitedHouses.IsEmpty())
			{
				OnHouseListEmpty.ExecuteIfBound();
				return;
			}
			MoveToClosestUnvisitedHouse();
			return;
		}
	}
	else
	{
		TimeStuck = 0.f;
	}

	float const DistSq = FVector::DistSquared(PawnLocation, CurrentTarget->GetActorLocation());
	if (DistSq <= HouseVisitRadius * HouseVisitRadius)
	{
		VisitedHouses.AddUnique(CurrentTarget);
		UnvisitedHouses.Remove(CurrentTarget);
		CurrentTarget = nullptr;
		TimeStuck = 0.f;

		if (UnvisitedHouses.IsEmpty())
		{
			OnHouseListEmpty.ExecuteIfBound();
			return;
		}

		MoveToClosestUnvisitedHouse();
		return;
	}

	if (AAIController* AIC = Cast<AAIController>(OwnerPawn->GetController()))
	{
		if (AIC->GetMoveStatus() == EPathFollowingStatus::Idle)
			MoveToClosestUnvisitedHouse();
	}
}

void USurvivorHouseExplorer::MoveToClosestUnvisitedHouse()
{
	if (UnvisitedHouses.IsEmpty())
		return;

	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (!OwnerPawn)
		return;

	AAIController* AIC = Cast<AAIController>(OwnerPawn->GetController());
	if (!AIC)
		return;

	if (CurrentTarget && UnvisitedHouses.Contains(CurrentTarget))
	{
		AIC->MoveToActor(CurrentTarget, HouseVisitRadius);
		return;
	}

	FVector const PawnLocation = OwnerPawn->GetActorLocation();
	AHouse* Closest = nullptr;
	float BestDistSq = MAX_FLT;

	for (AHouse* House : UnvisitedHouses)
	{
		if (!IsValid(House))
			continue;

		float const DistSq = FVector::DistSquared(PawnLocation, House->GetActorLocation());
		if (DistSq < BestDistSq)
		{
			BestDistSq = DistSq;
			Closest = House;
		}
	}

	if (Closest)
	{
		CurrentTarget = Closest;
		
		// FIX: Project the house location to a reachable spot on the NavMesh
		UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
		if (NavSys)
		{
			FNavLocation ProjectedLocation;
			// Find the nearest valid point within 500 units of the house origin
			if (NavSys->ProjectPointToNavigation(CurrentTarget->GetActorLocation(), ProjectedLocation, FVector(500.f, 500.f, 500.f)))
			{
				AIC->MoveToLocation(ProjectedLocation.Location, HouseVisitRadius);
			}
			else
			{
				// Fallback if projection fails
				AIC->MoveToActor(CurrentTarget, HouseVisitRadius);
			}
		}
		else
		{
			AIC->MoveToActor(CurrentTarget, HouseVisitRadius);
		}
	}
}

void USurvivorHouseExplorer::SetEnabled(bool bEnabled)
{
	bIsEnabled = bEnabled;

	if (bIsEnabled)
	{
		MoveToClosestUnvisitedHouse();
	}
	else
	{
		CurrentTarget = nullptr;

		APawn* OwnerPawn = Cast<APawn>(GetOwner());
		if (!OwnerPawn)
			return;

		if (UMovementComponent* Movement = OwnerPawn->GetComponentByClass<UMovementComponent>())
			Movement->StopMovementImmediately();
	}
}

void USurvivorHouseExplorer::OnHouseSpotted(AHouse* House)
{
	if (!House)
		return;
	
	if (VisitedHouses.Contains(House))
		return;

	UnvisitedHouses.AddUnique(House);

	if (bIsEnabled && !CurrentTarget)
		MoveToClosestUnvisitedHouse();
}

bool USurvivorHouseExplorer::HasUnvisitedHouses() const
{
	return UnvisitedHouses.Num() > 0;
}
