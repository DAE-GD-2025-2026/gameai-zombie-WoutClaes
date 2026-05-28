#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "StudentPerceptor.generated.h"

class USurvivorWanderer;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class CLAESWOUTZOMBIERUNTIME_API UStudentPerceptor : public UActorComponent
{
	GENERATED_BODY()

public:
	UStudentPerceptor();

protected:
	virtual void BeginPlay() override;

private:
	void SetupNavCollision();
	void SetupBehaviourComponents();

	UPROPERTY()
	USurvivorWanderer* Wanderer{nullptr};
};
