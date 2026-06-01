#include "FleeTaskClaesWout.h"
#include "AIController.h"
#include "NavigationSystem.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Navigation/PathFollowingComponent.h"
#include "Common/InventoryComponent.h"
#include "Common/HealthComponent.h"
#include "Items/Weapon.h"

UFleeTaskClaesWout::UFleeTaskClaesWout()
{
	NodeName = "Flee";
	bNotifyTick = true;

	TargetActorKey.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(UFleeTaskClaesWout, TargetActorKey), AActor::StaticClass());
}

EBTNodeResult::Type UFleeTaskClaesWout::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* AICon = OwnerComp.GetAIOwner();
	if (!AICon) return EBTNodeResult::Failed;

	FFleeMemory* Memory = reinterpret_cast<FFleeMemory*>(NodeMemory);
	Memory->FleeTimer = FleeDuration;
	Memory->FleeDestination = FVector::ZeroVector;

	AICon->ClearFocus(EAIFocusPriority::Gameplay);

	return EBTNodeResult::InProgress;
}

void UFleeTaskClaesWout::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	AAIController* AICon = OwnerComp.GetAIOwner();
	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();

	if (!AICon || !AICon->GetPawn() || !BB)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	FFleeMemory* Memory = reinterpret_cast<FFleeMemory*>(NodeMemory);
	APawn* Pawn = AICon->GetPawn();

	Memory->FleeTimer -= DeltaSeconds;
	if (Memory->FleeTimer <= 0.f)
	{
		BB->SetValueAsObject(TargetActorKey.SelectedKeyName, nullptr);
		BB->SetValueAsBool(FName("IsZombieVisible"), false);
		BB->SetValueAsBool(FName("ShouldFlee"), false); // Escape Flee State
		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
		return;
	}

	AActor* TargetZombie = Cast<AActor>(BB->GetValueAsObject(TargetActorKey.SelectedKeyName));
	if (IsValid(TargetZombie))
	{
		UInventoryComponent* Inventory = Pawn->FindComponentByClass<UInventoryComponent>();
		UHealthComponent* Health = Pawn->FindComponentByClass<UHealthComponent>();

		bool bHasWeapon = false;
		if (Inventory)
		{
			for (ABaseItem* Item : Inventory->GetInventory()) // Or Inventory->GetInventory() matching your class layout
			{
				if (Item && Item->IsA(AWeapon::StaticClass()) && Item->GetValue() > 0)
				{
					bHasWeapon = true;
					break;
				}
			}
		}

		float CurrentHealth = Health ? Health->GetHealth() : 0.f;
		if (bHasWeapon && CurrentHealth > FleeReengageHealthThreshold)
		{
			BB->SetValueAsBool(FName("ShouldFight"), true);
			BB->SetValueAsBool(FName("ShouldFlee"), false); // Transition to Fight
			FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
			return;
		}
	}

	// Pick a new flee destination if idle or no destination yet
	if (AICon->GetMoveStatus() == EPathFollowingStatus::Idle || Memory->FleeDestination.IsZero())
	{
		FVector ZombieLoc = IsValid(TargetZombie)
			? TargetZombie->GetActorLocation()
			: Pawn->GetActorLocation() - Pawn->GetActorForwardVector() * 500.f;
		FVector MyLoc = Pawn->GetActorLocation();

		FVector FleeDirection = (MyLoc - ZombieLoc).GetSafeNormal2D();
		if (FleeDirection.IsNearlyZero())
			FleeDirection = -Pawn->GetActorForwardVector();

		UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
		if (NavSys)
		{
			FNavLocation NavLoc;
			bool bFoundValidPath = false;

			// Try straight back
			FVector TargetPoint = MyLoc + (FleeDirection * 2500.f);
			if (NavSys->ProjectPointToNavigation(TargetPoint, NavLoc, FVector(300.f, 300.f, 300.f)))
			{
				Memory->FleeDestination = NavLoc.Location;
				bFoundValidPath = true;
			}

			// Try left diagonal
			if (!bFoundValidPath)
			{
				FVector LeftDir = FleeDirection.RotateAngleAxis(45.f, FVector::UpVector);
				TargetPoint = MyLoc + (LeftDir * 1500.f);
				if (NavSys->ProjectPointToNavigation(TargetPoint, NavLoc, FVector(300.f, 300.f, 300.f)))
				{
					Memory->FleeDestination = NavLoc.Location;
					bFoundValidPath = true;
				}
			}

			// Try right diagonal
			if (!bFoundValidPath)
			{
				FVector RightDir = FleeDirection.RotateAngleAxis(-45.f, FVector::UpVector);
				TargetPoint = MyLoc + (RightDir * 1500.f);
				if (NavSys->ProjectPointToNavigation(TargetPoint, NavLoc, FVector(300.f, 300.f, 300.f)))
				{
					Memory->FleeDestination = NavLoc.Location;
					bFoundValidPath = true;
				}
			}

			// Fallback: short distance with larger tolerance
			if (!bFoundValidPath)
			{
				TargetPoint = MyLoc + (FleeDirection * 600.f);
				if (NavSys->ProjectPointToNavigation(TargetPoint, NavLoc, FVector(500.f, 500.f, 500.f)))
				{
					Memory->FleeDestination = NavLoc.Location;
					bFoundValidPath = true;
				}
			}

			if (bFoundValidPath)
				AICon->MoveToLocation(Memory->FleeDestination, 50.f);
		}
	}
}

EBTNodeResult::Type UFleeTaskClaesWout::AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	if (AAIController* AICon = OwnerComp.GetAIOwner())
		AICon->StopMovement();
	return EBTNodeResult::Aborted;
}
