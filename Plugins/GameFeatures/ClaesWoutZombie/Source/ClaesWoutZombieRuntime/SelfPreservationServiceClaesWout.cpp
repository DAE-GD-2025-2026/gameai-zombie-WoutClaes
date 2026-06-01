#include "SelfPreservationServiceClaesWout.h"
#include "AIController.h"
#include "Common/InventoryComponent.h"
#include "Items/Food.h"
#include "Items/Medkit.h"

void USelfPreservationServiceClaesWout::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory,
                                                 float DeltaSeconds)
{
	Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);

	AAIController* AICon = OwnerComp.GetAIOwner();
	if (!AICon || !AICon->GetPawn()) return;

	APawn* Pawn = AICon->GetPawn();
	UHealthComponent* HP = Pawn->FindComponentByClass<UHealthComponent>();
	UInventoryComponent* Inv = Pawn->FindComponentByClass<UInventoryComponent>();
	UStaminaComponent* StaminaComp = Pawn->FindComponentByClass<UStaminaComponent>();

	if (!HP || !Inv) return;

	if (HP->GetHealth() <= 3.0f) 
	{
		for (int32 i = 0; i < Inv->GetInventoryCapacity(); ++i)
		{
			ABaseItem* Item = Inv->GetInventory()[i];
			if (Item && Item->IsA(AMedkit::StaticClass()))
			{
				if (Inv->UseItem(i))
				{
					Inv->RemoveItem(i);
					break; 
				}
			}
		}
	}
	
	if (StaminaComp && StaminaComp->GetCurrentStamina() <= 3.0f)
	{
		for (int32 i = 0; i < Inv->GetInventoryCapacity(); ++i)
		{
			ABaseItem* Item = Inv->GetInventory()[i];
			if (Item && Item->IsValidLowLevel() && Item->IsA(AFood::StaticClass()))
			{
				if (Inv->UseItem(i))
				{
					Inv->RemoveItem(i);
					break;
				}
			}
		}
	}
	
	for (int32 i = 0; i < Inv->GetInventoryCapacity(); ++i)
	{
		ABaseItem* Item = Inv->GetInventory()[i];
		if (Item && Item->GetValue() <= 0)
		{
			Inv->RemoveItem(i);
		}
	}
}
