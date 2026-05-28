#include "StudentPerceptor.h"
#include "SurvivorWanderer.h"
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
}
