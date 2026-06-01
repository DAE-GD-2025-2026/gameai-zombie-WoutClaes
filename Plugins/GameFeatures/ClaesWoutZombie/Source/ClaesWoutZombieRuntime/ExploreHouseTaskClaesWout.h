#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "ExploreHouseTaskClaesWout.generated.h"

UCLASS()
class CLAESWOUTZOMBIERUNTIME_API UExploreHouseTaskClaesWout : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UExploreHouseTaskClaesWout();

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
	virtual EBTNodeResult::Type AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

protected:
	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FBlackboardKeySelector TargetActorKey;

	UPROPERTY(EditAnywhere, Category = "House")
	float HouseAcceptanceRadius = 150.f;

	UPROPERTY(EditAnywhere, Category = "House")
	float MaxExploreHouseTime = 5.f;

private:
	struct FExploreHouseMemory
	{
		float ExploreTimer = 0.f;
		FVector EntranceLocation = FVector::ZeroVector;
	};

	uint16 GetInstanceMemorySize() const override { return sizeof(FExploreHouseMemory); }
};
