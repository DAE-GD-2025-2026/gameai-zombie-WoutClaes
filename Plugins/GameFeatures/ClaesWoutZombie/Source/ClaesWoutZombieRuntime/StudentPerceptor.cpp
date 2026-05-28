// Fill out your copyright notice in the Description page of Project Settings.

#include "StudentPerceptor.h"
#include "SurvivorWanderer.h"
#include "SurvivorItemPursuer.h"
#include "SurvivorHouseExplorer.h"
#include "Items/BaseItem.h"
#include "Village/House/House.h"
#include "Perception/AIPerceptionComponent.h"
#include "Components/CapsuleComponent.h"

UStudentPerceptor::UStudentPerceptor()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UStudentPerceptor::BeginPlay()
{
	Super::BeginPlay();

	SetupNavCollision();
	SetupBehaviourComponents();

	if (UAIPerceptionComponent* PerceptionComp = GetOwner()->GetComponentByClass<UAIPerceptionComponent>())
	{
		PerceptionComp->OnTargetPerceptionUpdated.AddDynamic(this, &UStudentPerceptor::OnPerceptionUpdated);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("StudentPerceptor: No UAIPerceptionComponent on %s"), *GetOwner()->GetName());
	}
}

void UStudentPerceptor::SetupNavCollision()
{
	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (!OwnerPawn)
		return;

	if (OwnerPawn->GetComponentByClass<UCapsuleComponent>())
		return;

	UCapsuleComponent* Capsule = NewObject<UCapsuleComponent>(OwnerPawn, TEXT("NavCapsule"));
	Capsule->InitCapsuleSize(34.f, 88.f);
	Capsule->SetCollisionProfileName(TEXT("Pawn"));
	Capsule->RegisterComponent();
	OwnerPawn->SetRootComponent(Capsule);
}

void UStudentPerceptor::SetupBehaviourComponents()
{
	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (!OwnerPawn)
		return;

	Wanderer = NewObject<USurvivorWanderer>(OwnerPawn, TEXT("SurvivorWanderer"));
	Wanderer->RegisterComponent();

	HouseExplorer = NewObject<USurvivorHouseExplorer>(OwnerPawn, TEXT("SurvivorHouseExplorer"));
	HouseExplorer->RegisterComponent();

	ItemPursuer = NewObject<USurvivorItemPursuer>(OwnerPawn, TEXT("SurvivorItemPursuer"));
	ItemPursuer->RegisterComponent();
	
	ItemPursuer->OnItemListEmpty.BindLambda([this]()
	{
		EndItemPursuit();
	});
	
	HouseExplorer->OnHouseListEmpty.BindLambda([this]()
	{
		EndHouseExploration();
	});
}

// Perception

void UStudentPerceptor::OnPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus)
{
	// Items: highest priority
	if (ABaseItem* Item = Cast<ABaseItem>(Actor))
	{
		if (Stimulus.WasSuccessfullySensed())
		{
			ItemPursuer->OnItemSpotted(Item);
			if (!ItemPursuer->IsEnabled())
				BeginItemPursuit();
		}
		else
		{
			ItemPursuer->OnItemLost(Item);
			if (!ItemPursuer->HasKnownItems())
				EndItemPursuit();
		}
		return;
	}

	// Houses: second priority
	if (AHouse* House = Cast<AHouse>(Actor))
	{
		if (Stimulus.WasSuccessfullySensed())
		{
			HouseExplorer->OnHouseSpotted(House);

			// Only switch to house exploration if we are currently just wandering
			if (Wanderer->IsEnabled() && !ItemPursuer->IsEnabled())
				BeginHouseExploration();
		}
		return;
	}
}

void UStudentPerceptor::BeginItemPursuit()
{
	Wanderer->SetEnabled(false);
	HouseExplorer->SetEnabled(false);
	ItemPursuer->SetEnabled(true);
}

void UStudentPerceptor::EndItemPursuit()
{
	ItemPursuer->SetEnabled(false);

	if (HouseExplorer->HasUnvisitedHouses())
		BeginHouseExploration();
	else
		Wanderer->SetEnabled(true);
}

void UStudentPerceptor::BeginHouseExploration()
{
	Wanderer->SetEnabled(false);
	HouseExplorer->SetEnabled(true);
}

void UStudentPerceptor::EndHouseExploration()
{
	HouseExplorer->SetEnabled(false);
	Wanderer->SetEnabled(true);
}
