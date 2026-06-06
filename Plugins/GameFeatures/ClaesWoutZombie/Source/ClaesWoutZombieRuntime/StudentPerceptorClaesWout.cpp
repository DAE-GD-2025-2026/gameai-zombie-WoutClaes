#include "StudentPerceptorClaesWout.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Common/InventoryComponent.h"
#include "Components/CapsuleComponent.h"
#include "Village/House/House.h"
#include "Items/BaseItem.h"
#include "Items/Food.h"
#include "Items/Medkit.h"
#include "Items/Pistol.h"
#include "Items/Shotgun.h"
#include "Items/Weapon.h"
#include "Zombies/BaseZombie.h"

UStudentPerceptorClaesWout::UStudentPerceptorClaesWout()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UStudentPerceptorClaesWout::BeginPlay()
{
	Super::BeginPlay();
	
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().SetTimer(
			BreadcrumbTimerHandle, 
			this, 
			&UStudentPerceptorClaesWout::DropBreadcrumb, 
			3.0f, 
			true
		);
	}
	
	if (auto PerceptionComp = GetOwner()->GetComponentByClass<UAIPerceptionComponent>())
	{
		PerceptionComp->OnTargetPerceptionUpdated.AddDynamic(this, &UStudentPerceptorClaesWout::OnPerceptionUpdated);
	}
	
	SetupNavCollision();
	
	if (BehaviorTreeAsset)
	{
		APawn* OwnerPawn = Cast<APawn>(GetOwner());
		if (OwnerPawn)
		{
			AAIController* AICon = Cast<AAIController>(OwnerPawn->GetController());
			if (AICon)
			{
				AICon->RunBehaviorTree(BehaviorTreeAsset);
			}
		}
	}
}

void UStudentPerceptorClaesWout::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (!OwnerPawn) return;

	if (UHealthComponent* HP = OwnerPawn->FindComponentByClass<UHealthComponent>())
	{
		float CurrentHealth = HP->GetHealth();

		// Initialize the health on the first tick so we don't trigger damage instantly
		if (!bHasInitializedHealth)
		{
			PreviousHealth = CurrentHealth;
			bHasInitializedHealth = true;
			return;
		}

		if (CurrentHealth < PreviousHealth)
		{
			FRotator NewRotation = OwnerPawn->GetActorRotation();
			NewRotation.Yaw += 180.0f;
			OwnerPawn->SetActorRotation(NewRotation);

			if (AAIController* AICon = Cast<AAIController>(OwnerPawn->GetController()))
			{
				AICon->StopMovement();
			}
		}

		PreviousHealth = CurrentHealth;
	}
}

UBlackboardComponent* UStudentPerceptorClaesWout::GetBlackboard() const
{
	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (!OwnerPawn) return nullptr;

	AAIController* AICon = Cast<AAIController>(OwnerPawn->GetController());
	if (!AICon) return nullptr;

	return AICon->GetBlackboardComponent();
}

void UStudentPerceptorClaesWout::AddVisitedHouse(AActor* House)
{
	if (House) VisitedHouses.Add(House);
}

void UStudentPerceptorClaesWout::DropBreadcrumb()
{
	if (APawn* OwnerPawn = Cast<APawn>(GetOwner()))
	{
		Breadcrumbs.Add(OwnerPawn->GetActorLocation());
		
		if (Breadcrumbs.Num() > 20)
		{
			Breadcrumbs.RemoveAt(0);
		}
	}
}

bool UStudentPerceptorClaesWout::ShouldPickUpItem(ABaseItem* Item) const
{
	if (!Item || Item->GetValue() <= 0) return false;

	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (!OwnerPawn) return false;

	UInventoryComponent* Inventory = OwnerPawn->FindComponentByClass<UInventoryComponent>();
	if (!Inventory) return false;

	int PistolCount = 0, ShotgunCount = 0, FoodCount = 0, MedkitCount = 0, EmptySlots = 0;

	for (ABaseItem* InvItem : Inventory->GetInventory())
	{
		if (!InvItem)
		{
			EmptySlots++;
			continue;
		}
		if (InvItem->GetValue() > 0)
		{
			if (InvItem->IsA(APistol::StaticClass())) PistolCount++;
			else if (InvItem->IsA(AShotgun::StaticClass())) ShotgunCount++;
			else if (InvItem->IsA(AFood::StaticClass())) FoodCount++;
			else if (InvItem->IsA(AMedkit::StaticClass())) MedkitCount++;
		}
		else { EmptySlots++; }
	}

	if (EmptySlots == 0) return false;

	if (Item->IsA(APistol::StaticClass()) && PistolCount >= 1) return false;
	if (Item->IsA(AShotgun::StaticClass()) && ShotgunCount >= 1) return false;
	if (Item->IsA(AFood::StaticClass()) && FoodCount >= 2) return false;
	if (Item->IsA(AMedkit::StaticClass()) && MedkitCount >= 2) return false;

	return true;
}

void UStudentPerceptorClaesWout::OnPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus)
{
	UBlackboardComponent* Blackboard = GetBlackboard();
	if (!Blackboard) return;

	if (ABaseItem* Item = Cast<ABaseItem>(Actor))
	{
		if (Stimulus.WasSuccessfullySensed() && ShouldPickUpItem(Item)) 
		{
			Blackboard->SetValueAsObject(BBK_TargetItem, Item);
			Blackboard->SetValueAsBool(BBK_DesireItem, true);
		}
	}
	
	if (AHouse* House = Cast<AHouse>(Actor))
	{
		if (Stimulus.WasSuccessfullySensed() && !VisitedHouses.Contains(House)) 
		{
			Blackboard->SetValueAsObject(BBK_TargetHouse, House);
			Blackboard->SetValueAsBool(BBK_DesireHouse, true);
		}
	}
	
	if (Actor && Actor != GetOwner() && Actor->IsA(ABaseZombie::StaticClass()))
	{
		if (Stimulus.WasSuccessfullySensed())
		{
			Blackboard->SetValueAsObject(BBK_TargetZombie, Actor);
			Blackboard->SetValueAsBool(BBK_IsZombieVisible, true);

			APawn* OwnerPawn = Cast<APawn>(GetOwner());
			bool bHasWeapon = false;
			float CurrentHealth = 0.f;

			if (OwnerPawn)
			{
				if (UInventoryComponent* Inv = OwnerPawn->FindComponentByClass<UInventoryComponent>())
				{
					for (ABaseItem* Item : Inv->GetInventory())
					{
						if (Item && Item->IsA(AWeapon::StaticClass()) && Item->GetValue() > 0)
						{
							bHasWeapon = true;
							break;
						}
					}
				}
				if (UHealthComponent* HP = OwnerPawn->FindComponentByClass<UHealthComponent>())
					CurrentHealth = HP->GetHealth();
			}

			bool bFightDecision = bHasWeapon && CurrentHealth > 3;
			Blackboard->SetValueAsBool(BBK_ShouldFight, bFightDecision);
			Blackboard->SetValueAsBool(BBK_ShouldFlee, !bFightDecision);
		}
		else
		{
			Blackboard->SetValueAsBool(BBK_IsZombieVisible, false);
		}
	}
}

void UStudentPerceptorClaesWout::SetupNavCollision()
{
	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (!OwnerPawn) return;

	if (OwnerPawn->GetComponentByClass<UCapsuleComponent>()) return;

	UCapsuleComponent* Capsule = NewObject<UCapsuleComponent>(OwnerPawn, TEXT("NavCapsule"));
	Capsule->InitCapsuleSize(34.f, 88.f);
	Capsule->SetCollisionProfileName(TEXT("Pawn"));
	Capsule->RegisterComponent();
	OwnerPawn->SetRootComponent(Capsule);
}
