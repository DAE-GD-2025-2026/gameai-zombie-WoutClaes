#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BehaviorTree.h"
#include "Components/ActorComponent.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"
#include "Perception/AISenseConfig_Damage.h"
#include "Perception/AISense_Damage.h"
#include "StudentPerceptorClaesWout.generated.h"

class UBLackboardComponent;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class CLAESWOUTZOMBIERUNTIME_API UStudentPerceptorClaesWout : public UActorComponent
{
	GENERATED_BODY()

public:
	UStudentPerceptorClaesWout();
	
	virtual void BeginPlay() override;

	UFUNCTION()
	virtual void OnPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus);
	
	void SetupNavCollision();
	
	void AddVisitedHouse(AActor* House);
	
	void DropBreadcrumb();
	const TArray<FVector>& GetBreadcrumbs() const { return Breadcrumbs; }
protected:
	UPROPERTY(EditDefaultsOnly, Category = "AI")
	UBehaviorTree* BehaviorTreeAsset;
	
	UBlackboardComponent* GetBlackboard() const;

	const FName BBK_TargetZombie = FName("TargetZombie");
	const FName BBK_TargetItem = FName("TargetItem");
	const FName BBK_TargetHouse = FName("TargetHouse");

	const FName BBK_IsZombieVisible = FName("IsZombieVisible");
	const FName BBK_ShouldFight = FName("ShouldFight");
	const FName BBK_ShouldFlee = FName("ShouldFlee");
	const FName BBK_DesireItem = FName("DesireItem");
	const FName BBK_DesireHouse = FName("DesireHouse");
	
	TSet<AActor*> VisitedHouses;
	TArray<FVector> Breadcrumbs;
	FTimerHandle BreadcrumbTimerHandle;
	
	bool ShouldPickUpItem(class ABaseItem* Item) const;
};
