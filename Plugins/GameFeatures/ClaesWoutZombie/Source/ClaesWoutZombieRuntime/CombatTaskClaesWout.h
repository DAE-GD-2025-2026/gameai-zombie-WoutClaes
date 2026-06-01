#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "CombatTaskClaesWout.generated.h"

UCLASS()
class CLAESWOUTZOMBIERUNTIME_API UCombatTaskClaesWout : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UCombatTaskClaesWout();

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
	virtual EBTNodeResult::Type AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

protected:
	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FBlackboardKeySelector TargetActorKey;

	UPROPERTY(EditAnywhere, Category = "Combat")
	float CombatEngageRange = 800.f;

	UPROPERTY(EditAnywhere, Category = "Combat")
	float WeaponFireCooldownDuration = 0.6f;

	UPROPERTY(EditAnywhere, Category = "Combat")
	float ZombieMemoryDuration = 4.f;

private:
	struct FCombatMemory
	{
		float WeaponFireTimer = 0.f;
		float TimeSinceZombieSeen = 0.f;
	};

	uint16 GetInstanceMemorySize() const override { return sizeof(FCombatMemory); }
};
