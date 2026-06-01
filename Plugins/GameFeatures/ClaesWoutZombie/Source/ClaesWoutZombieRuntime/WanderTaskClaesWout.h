#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "WanderTaskClaesWout.generated.h"

UCLASS()
class CLAESWOUTZOMBIERUNTIME_API UWanderTaskClaesWout : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UWanderTaskClaesWout();
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;

protected:
	UPROPERTY(EditAnywhere, Category = "Wander")
	float ForwardDistance = 800.f;

	UPROPERTY(EditAnywhere, Category = "Wander")
	float ForwardRadius = 400.f;
	
	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FBlackboardKeySelector DestinationKey;

private:
	bool PickWanderLocation(class AAIController* AICon, FVector& OutLocation);
};
