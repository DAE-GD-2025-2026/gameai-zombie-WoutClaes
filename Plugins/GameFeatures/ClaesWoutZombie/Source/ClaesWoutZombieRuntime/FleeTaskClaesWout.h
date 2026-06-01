#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "FleeTaskClaesWout.generated.h"

UCLASS()
class CLAESWOUTZOMBIERUNTIME_API UFleeTaskClaesWout : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UFleeTaskClaesWout();

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
	virtual EBTNodeResult::Type AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

protected:
	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FBlackboardKeySelector TargetActorKey;

	UPROPERTY(EditAnywhere, Category = "Flee")
	float FleeDuration = 4.f;

	UPROPERTY(EditAnywhere, Category = "Flee")
	int32 FleeReengageHealthThreshold = 3;

private:
	struct FFleeMemory
	{
		float FleeTimer = 0.f;
		FVector FleeDestination = FVector::ZeroVector;
	};

	uint16 GetInstanceMemorySize() const override { return sizeof(FFleeMemory); }
};
