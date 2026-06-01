#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTService.h"
#include "FleeSprintServiceClaesWout.generated.h"

UCLASS()
class CLAESWOUTZOMBIERUNTIME_API UFleeSprintServiceClaesWout : public UBTService
{
	GENERATED_BODY()
public:
	UFleeSprintServiceClaesWout() { NodeName = "Flee Sprint Handling"; bNotifyBecomeRelevant = true; bNotifyCeaseRelevant = true; }

	virtual void OnBecomeRelevant(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void OnCeaseRelevant(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
};