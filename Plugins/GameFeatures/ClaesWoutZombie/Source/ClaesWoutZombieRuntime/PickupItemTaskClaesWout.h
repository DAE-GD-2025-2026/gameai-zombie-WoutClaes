#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "PickupItemTaskClaesWout.generated.h"

class ABaseItem;

UCLASS()
class CLAESWOUTZOMBIERUNTIME_API UPickupItemTaskClaesWout : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UPickupItemTaskClaesWout();

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
	virtual EBTNodeResult::Type AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

protected:
	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FBlackboardKeySelector TargetActorKey;

	UPROPERTY(EditAnywhere, Category = "Pickup")
	float ItemPickupRadius = 120.f;

	UPROPERTY(EditAnywhere, Category = "Pickup")
	float MaxPickupTime = 3.f;

private:
	struct FPickupItemMemory
	{
		float PickupTimer = 0.f;
	};

	uint16 GetInstanceMemorySize() const override { return sizeof(FPickupItemMemory); }
};
