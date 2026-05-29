// Fill out your copyright notice in the Description page of Project Settings.

#include "StudentPerceptor.h"
#include "Components/CapsuleComponent.h"
#include "Village/House/House.h"
#include "Items/BaseItem.h"

UStudentPerceptor::UStudentPerceptor()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UStudentPerceptor::BeginPlay()
{
	Super::BeginPlay();
	
	if (auto PerceptionComp = GetOwner()->GetComponentByClass<UAIPerceptionComponent>())
	{
		PerceptionComp->OnTargetPerceptionUpdated.AddDynamic(this, &UStudentPerceptor::OnPerceptionUpdated);
	}
	
	SetupNavCollision();
	SetupBehaviourComponents();
}

void UStudentPerceptor::OnPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus)
{
	if (ABaseItem* Item = Cast<ABaseItem>(Actor))
	{
		if (Stimulus.WasSuccessfullySensed())
		{
			if (Wanderer && Wanderer->CanOverride(ESurvivorState::PickupItem))
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

	Wanderer = NewObject<USurvivorMovement>(OwnerPawn, TEXT("SurvivorWanderer"));
	Wanderer->RegisterComponent();
}
