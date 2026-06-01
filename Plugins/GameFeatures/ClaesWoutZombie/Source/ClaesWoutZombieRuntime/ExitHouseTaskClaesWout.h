#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "ExitHouseTaskClaesWout.generated.h"

UCLASS()
class CLAESWOUTZOMBIERUNTIME_API UExitHouseTaskClaesWout : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UExitHouseTaskClaesWout();

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
	virtual EBTNodeResult::Type AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

protected:
	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FBlackboardKeySelector ExitLocationKey;

	UPROPERTY(EditAnywhere, Category = "House")
	float ExitAcceptanceRadius = 150.f;

	UPROPERTY(EditAnywhere, Category = "House")
	float MaxExitHouseTime = 3.f;

private:
	struct FExitHouseMemory
	{
		float ExitTimer = 0.f;
		FVector ExitLocation = FVector::ZeroVector;
	};

	uint16 GetInstanceMemorySize() const override { return sizeof(FExitHouseMemory); }
};
