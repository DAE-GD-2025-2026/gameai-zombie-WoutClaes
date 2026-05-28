#pragma once

#include "SurvivorMovement.generated.h"

UENUM()
enum class ESurvivorState : uint8
{
	Wander,
	ExploreHouse,
	ExitHouse,
};

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class CLAESWOUTZOMBIERUNTIME_API USurvivorMovement : public UActorComponent
{
	GENERATED_BODY()

public:
	USurvivorMovement();

protected:
	virtual void BeginPlay() override;

public:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	void StartExploringHouse(AActor* House);

private:
	//========================
	// Wander
	//========================
	ESurvivorState State = ESurvivorState::Wander;
	
	void TickWander(float DeltaTime);
	void PickNewWanderTarget();
	
	//========================
	// House Exploring
	//========================
	AActor* CurrentHouse = nullptr;
	TSet<AActor*> VisitedHouses;

	UPROPERTY(EditAnywhere)
	float WanderRadius = 2000.f;

	UPROPERTY(EditAnywhere)
	float AcceptanceRadius = 200.f;

	UPROPERTY(EditAnywhere)
	float HouseAcceptanceRadius = 150.f;
	
	void TickExploreHouse(float DeltaTime);
	void MoveToHouseCenter();

	//========================
	// House Exiting
	//========================
	FVector HouseExitLocation;
	FVector EntranceLocation;
	float ExploreHouseTimer = 0.f;
	float ExitHouseTimer = 0.f;
	
	UPROPERTY(EditAnywhere)
	float MaxExitHouseTime = 3.f;
	
	UPROPERTY(EditAnywhere)
	float MaxExploreHouseTime = 3.f;
	
	void TickExitHouse(float DeltaTime);
};
