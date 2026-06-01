#pragma once
#include "CoreMinimal.h"
#include "BehaviorTree/BTService.h"
#include "SelfPreservationServiceClaesWout.generated.h"

UCLASS()
class CLAESWOUTZOMBIERUNTIME_API USelfPreservationServiceClaesWout : public UBTService
{
	GENERATED_BODY()
	
public:
	USelfPreservationServiceClaesWout() { NodeName = "Self Preservation Check"; bNotifyTick = true; Interval = 1.0f; }
	virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
};
