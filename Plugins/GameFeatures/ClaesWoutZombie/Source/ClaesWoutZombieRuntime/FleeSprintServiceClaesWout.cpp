#include "FleeSprintServiceClaesWout.h"
#include "AIController.h"
#include "Survivor/SurvivorPawn.h"

void UFleeSprintServiceClaesWout::OnBecomeRelevant(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	Super::OnBecomeRelevant(OwnerComp, NodeMemory);
	if (AAIController* AICon = OwnerComp.GetAIOwner())
	{
		if (ASurvivorPawn* Survivor = Cast<ASurvivorPawn>(AICon->GetPawn()))
		{
			Survivor->StartRunning();
		}
	}
}

void UFleeSprintServiceClaesWout::OnCeaseRelevant(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	Super::OnCeaseRelevant(OwnerComp, NodeMemory);
	if (AAIController* AICon = OwnerComp.GetAIOwner())
	{
		if (ASurvivorPawn* Survivor = Cast<ASurvivorPawn>(AICon->GetPawn()))
		{
			Survivor->StopRunning();
		}
	}
}
