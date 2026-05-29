// Fill out your copyright notice in the Description page of Project Settings.

#include "StudentPerceptorClaesWout.h"
#include "Components/CapsuleComponent.h"
#include "Village/House/House.h"
#include "Items/BaseItem.h"

UStudentPerceptorClaesWout::UStudentPerceptorClaesWout()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UStudentPerceptorClaesWout::BeginPlay()
{
	Super::BeginPlay();
	
	if (auto PerceptionComp = GetOwner()->GetComponentByClass<UAIPerceptionComponent>())
	{
		PerceptionComp->OnTargetPerceptionUpdated.AddDynamic(this, &UStudentPerceptorClaesWout::OnPerceptionUpdated);
	}
	
	SetupNavCollision();
	SetupBehaviourComponents();
}

void UStudentPerceptorClaesWout::OnPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus)
{
	if (ABaseItem* Item = Cast<ABaseItem>(Actor))
	{
		if (Stimulus.WasSuccessfullySensed())
		{
			if (Wanderer && Wanderer->CanOverride(ESurvivorState::PickupItem) && Wanderer->ShouldPickUpItem(Item))
			{
				Wanderer->StartPickingUpItem(Item);
			}
		}
	}
	
	if (AHouse* House = Cast<AHouse>(Actor))
	{
		if (Stimulus.WasSuccessfullySensed())
		{
			if (Wanderer && Wanderer->CanOverride(ESurvivorState::ExploreHouse))
			{
				Wanderer->StartExploringHouse(House);
			}
		}
	}
}

void UStudentPerceptorClaesWout::SetupNavCollision()
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

void UStudentPerceptorClaesWout::SetupBehaviourComponents()
{
	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (!OwnerPawn)
		return;

	Wanderer = NewObject<USurvivorMovementClaesWout>(OwnerPawn, TEXT("SurvivorWanderer"));
	Wanderer->RegisterComponent();
}
