// Fill out your copyright notice in the Description page of Project Settings.

#include "StudentPerceptor.h"
#include "SurvivorBrainComponent.h"
#include "SurvivorAIController.h"
#include "Survivor/SurvivorPawn.h"
#include "Zombies/BaseZombie.h"
#include "Items/BaseItem.h"
#include "PurgeZones/PurgeZone.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISense_Sight.h"
#include "Perception/AISense_Damage.h"


UStudentPerceptor::UStudentPerceptor()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UStudentPerceptor::BeginPlay()
{
	Super::BeginPlay();

	UE_LOG(LogTemp, Warning, TEXT("=============================="));
	UE_LOG(LogTemp, Warning, TEXT("[Perceptor] BeginPlay START"));
	UE_LOG(LogTemp, Warning, TEXT("[Perceptor] Owner: %s"), GetOwner() ? *GetOwner()->GetName() : TEXT("NULL"));

	// The GameFeature extension adds this component to the SurvivorPawn (not the AIController).
	// So GetOwner() IS the pawn — which is actually perfect, because the AIPerceptionComponent
	// also lives on the pawn. We just need to find the controller separately.

	// --- Step 1: Get the pawn we are on ---
	SurvivorPawn = Cast<ASurvivorPawn>(GetOwner());
	if (!SurvivorPawn)
	{
		UE_LOG(LogTemp, Error, TEXT("[Perceptor] FAIL: Owner is not ASurvivorPawn. Cannot continue."));
		return;
	}
	UE_LOG(LogTemp, Warning, TEXT("[Perceptor] OK: Owner is ASurvivorPawn."));

	// --- Step 2: Bind to the AIPerceptionComponent (also on the pawn) ---
	UAIPerceptionComponent* PerceptionComp = SurvivorPawn->GetComponentByClass<UAIPerceptionComponent>();
	if (!PerceptionComp)
	{
		UE_LOG(LogTemp, Error, TEXT("[Perceptor] FAIL: No UAIPerceptionComponent on pawn."));
	}
	else
	{
		PerceptionComp->OnTargetPerceptionUpdated.AddDynamic(this, &UStudentPerceptor::OnPerceptionUpdated);
		UE_LOG(LogTemp, Warning, TEXT("[Perceptor] OK: Perception delegate bound."));
	}

	// --- Step 3: Allocate blackboard ---
	Blackboard = MakeUnique<FSurvivorBlackboard>();
	UE_LOG(LogTemp, Warning, TEXT("[Perceptor] OK: Blackboard allocated."));

	// --- Step 4: Get the AIController ---
	// The controller may not be assigned yet at BeginPlay on the pawn side.
	// We try now and fall back to a timer if needed.
	TryRegisterBrain();

	UE_LOG(LogTemp, Warning, TEXT("[Perceptor] BeginPlay END"));
	UE_LOG(LogTemp, Warning, TEXT("=============================="));
}

void UStudentPerceptor::TryRegisterBrain()
{
	if (BrainComponent) return; // already done
	if (!SurvivorPawn)  return;

	ASurvivorAIController* Controller = Cast<ASurvivorAIController>(SurvivorPawn->GetController());
	if (!Controller)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Perceptor] Controller not ready yet, retrying in 0.1s..."));
		// Retry after a short delay - controller possession happens slightly after BeginPlay
		FTimerHandle Dummy;
		GetWorld()->GetTimerManager().SetTimer(Dummy, this, &UStudentPerceptor::TryRegisterBrain, 0.1f, false);
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("[Perceptor] OK: ASurvivorAIController found: %s"), *Controller->GetName());

	// --- Step 5: Create BrainComponent on the AIController ---
	BrainComponent = NewObject<USurvivorBrainComponent>(Controller, TEXT("SurvivorBrainComponent"));
	if (!BrainComponent)
	{
		UE_LOG(LogTemp, Error, TEXT("[Perceptor] FAIL: Could not create SurvivorBrainComponent."));
		return;
	}

	BrainComponent->SetBlackboard(Blackboard.Get());
	BrainComponent->RegisterComponent();

	UE_LOG(LogTemp, Warning, TEXT("[Perceptor] OK: Brain created and registered on controller. Tick: %s"),
		BrainComponent->IsComponentTickEnabled() ? TEXT("YES") : TEXT("NO"));
}

// ---------------------------------------------------------------------------
// Perception callback
// ---------------------------------------------------------------------------
void UStudentPerceptor::OnPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus)
{
	if (!Actor || !Blackboard) return;

	const bool bSensed   = Stimulus.WasSuccessfullySensed();
	const bool bIsSight  = Stimulus.Type == UAISense::GetSenseID<UAISense_Sight>();
	const bool bIsDamage = Stimulus.Type == UAISense::GetSenseID<UAISense_Damage>();

	UE_LOG(LogTemp, Log, TEXT("[Perceptor] Perceived: %s | Sensed=%d Sight=%d Damage=%d"),
		*Actor->GetName(), bSensed, bIsSight, bIsDamage);

	if (ABaseZombie* Zombie = Cast<ABaseZombie>(Actor))
	{
		TWeakObjectPtr<APawn> W(Zombie);
		if (bSensed) { Blackboard->KnownZombies.AddUnique(W); UE_LOG(LogTemp, Log, TEXT("[Perceptor] Zombie+ total=%d"), Blackboard->KnownZombies.Num()); }
		else if (bIsSight) { Blackboard->KnownZombies.Remove(W); UE_LOG(LogTemp, Log, TEXT("[Perceptor] Zombie- total=%d"), Blackboard->KnownZombies.Num()); }
		return;
	}

	if (ABaseItem* Item = Cast<ABaseItem>(Actor))
	{
		TWeakObjectPtr<ABaseItem> W(Item);
		if (bSensed && Item->GetValue() > 0) { Blackboard->KnownItems.AddUnique(W); UE_LOG(LogTemp, Log, TEXT("[Perceptor] Item+ %s total=%d"), *Actor->GetName(), Blackboard->KnownItems.Num()); }
		else { Blackboard->KnownItems.Remove(W); UE_LOG(LogTemp, Log, TEXT("[Perceptor] Item- total=%d"), Blackboard->KnownItems.Num()); }
		return;
	}

	if (APurgeZone* Purge = Cast<APurgeZone>(Actor))
	{
		TWeakObjectPtr<AActor> W(Purge);
		if (bSensed) { Blackboard->KnownPurgeZones.AddUnique(W); }
		else         { Blackboard->KnownPurgeZones.Remove(W); }
		return;
	}

	if (bIsDamage && Actor->IsA<APawn>())
	{
		TWeakObjectPtr<APawn> W(Cast<APawn>(Actor));
		Blackboard->KnownZombies.AddUnique(W);
		UE_LOG(LogTemp, Log, TEXT("[Perceptor] Damage-threat: %s"), *Actor->GetName());
	}
}
