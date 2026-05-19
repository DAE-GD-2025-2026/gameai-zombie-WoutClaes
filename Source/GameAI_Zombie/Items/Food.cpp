// Fill out your copyright notice in the Description page of Project Settings.


#include "Food.h"

#include "Common/StaminaComponent.h"


// Sets default values
AFood::AFood()
{
	ItemType = EItemType::Food;
}

void AFood::UseItem(ASurvivorPawn& Survivor)
{
	Survivor.GetComponentByClass<UStaminaComponent>()->AddStamina(GetValue());
	Value = 0;
}

