#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SurvivorWanderer.generated.h"

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class CLAESWOUTZOMBIERUNTIME_API USurvivorWanderer : public UActorComponent
{
	GENERATED_BODY()

public:
	USurvivorWanderer();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	
protected:
	virtual void BeginPlay() override;

private:
	void PickNewWanderTarget();

	UPROPERTY(EditAnywhere, Category="AI|Wander")
	float WanderRadius{2000.f};

	UPROPERTY(EditAnywhere, Category="AI|Wander")
	float AcceptanceRadius{200.f};
};
