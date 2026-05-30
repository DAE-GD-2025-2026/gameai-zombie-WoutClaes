#pragma once
#include "Common/InventoryComponent.h"
#include "SurvivorMovementClaesWout.generated.h"

UENUM()
enum class ESurvivorState : uint8
{
	Wander = 0,
	ExploreHouse = 1,
	ExitHouse = 2,
	PickupItem = 3,
	Combat = 4,
	Flee = 5,
};

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class CLAESWOUTZOMBIERUNTIME_API USurvivorMovementClaesWout : public UActorComponent
{
	GENERATED_BODY()

public:
	USurvivorMovementClaesWout();
	
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	ESurvivorState GetState() const{ return State; }
	
	void StartExploringHouse(AActor* House);
	void StartPickingUpItem(ABaseItem* Item);

	bool CanOverride(ESurvivorState NewState) const;
	bool ShouldPickUpItem(ABaseItem* Item);
	
	void HandleZombieSpotted(AActor* Zombie);
	void HandleZombieLost(AActor* Zombie);
	
	bool HasWeapon() const;
	
protected:
	virtual void BeginPlay() override;

private:
	//========================
	// Radar Spin Meta
	//========================
	float SpinAngle = 0.f;

	UPROPERTY(EditAnywhere, Category = "Wander")
	float SpinSpeed = 180.f;
	
	//========================
	// Cached Components
	//========================
	UPROPERTY()
	APawn* MyPawn = nullptr;

	UPROPERTY()
	AAIController* MyAIController = nullptr;
	
	ASurvivorPawn* Survivor = nullptr;
	//========================
	// State Tracking
	//========================
	ESurvivorState State = ESurvivorState::Wander;
	ESurvivorState PreviousState = ESurvivorState::Wander;
	
	//========================
	// Helper
	//========================
	int32 GetPriority(ESurvivorState CheckState) const { return static_cast<int32>(CheckState); }
	
	//========================
	// Wander
	//========================
	FVector WanderHeading = FVector::ZeroVector;
	float WanderHeadingTimer = 0.f;
	
	UPROPERTY(EditAnywhere, Category = "Wander")
	float WanderHeadingChangeDuration = 6.f;
	
	UPROPERTY(EditAnywhere, Category = "Wander")
	float WanderHeadingVariance = 30.f;
	
	UPROPERTY(EditAnywhere, Category = "Wander")
	float WanderForwardDistance = 800.f;

	UPROPERTY(EditAnywhere, Category = "Wander")
	float WanderForwardRadius = 400.f;
	
	void TickWander(float DeltaTime);
	void PickNewWanderTarget(bool bPickNewHeading);
	
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
	float MaxExploreHouseTime = 5.f;
	
	void TickExitHouse(float DeltaTime);
	
	//========================
	// Pickup Item
	//========================
	ABaseItem* CurrentItem = nullptr;
	TSet<AActor*> PickedUpItems;
	UInventoryComponent* Inventory = nullptr;
	float PickupTimer = 0.f;
	float MaxPickupTime = 3.f;

	UPROPERTY(EditAnywhere)
	float ItemPickupRadius = 120.f;
	
	void TickPickupItem(float DeltaTime);
	
	//========================
	// Inventory
	//========================
	UHealthComponent* Health = nullptr;
	UStaminaComponent* Stamina = nullptr;
	float ItemCooldownTimer = 0.f;
	
	UPROPERTY(EditAnywhere, Category = "Inventory")
	float ItemUseCooldownDuration = 2.0f;

	void TryUseInventory(float DeltaTime);
	
	//========================
	// Zombie Engagement
	//========================
	float WeaponFireTimer = 0.f;
	float FleeTimer = 0.f;
	bool bIsZombieVisible = false;
	float TimeSinceZombieSeen = 0.f;
	FVector FleeDestination = FVector::ZeroVector;
	bool bEnteredHouseWhileFleeing = false;
	
	UPROPERTY()
	AActor* TargetZombie = nullptr;

	UPROPERTY(EditAnywhere, Category = "Combat")
	float WeaponFireCooldownDuration = 0.6f;
	
	UPROPERTY(EditAnywhere, Category = "Flee")
	float FleeRepathInterval = 0.4f;
	
	UPROPERTY(EditAnywhere, Category = "Flee")
	float ZombieMemoryDuration = 4.0f;
	
	UPROPERTY(EditAnywhere, Category = "Combat")
	float CombatEngageRange = 800.f;
	
	UPROPERTY(EditAnywhere, Category = "Combat")
	float PanicRadius = 400.f;
	
	UPROPERTY(EditAnywhere, Category = "Flee")
	int FleeReengageHealthThreshold = 3;
	
	void TickFlee(float DeltaTime);
	void TickCombat(float DeltaTime);

	int32 GetWeaponSlot() const;
};
