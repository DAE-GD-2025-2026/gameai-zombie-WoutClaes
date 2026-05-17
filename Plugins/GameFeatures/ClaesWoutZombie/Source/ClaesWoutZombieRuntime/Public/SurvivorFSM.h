// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

/**
 * High-level survival states.
 * 
 * Transition priority (evaluated top-down each tick):
 *
 *  ANY STATE:
 *    health < 30 % AND medkit available  ->  UseMedkit    (immediate)
 *    nearest zombie < FleeRadius         ->  Flee
 *    nearest zombie < FightRadius AND weapon available AND health > 40 %  -> Fight
 *
 *  UseMedkit   ->  previous state when heal done
 *  Flee        ->  Fight   when zombie in range and weapon available and health ok
 *  Flee        ->  Explore when no zombies known
 *  Fight       ->  Flee    when health drops below threshold OR no ammo
 *  Fight       ->  Explore when no zombies known
 *  Explore     ->  SeekItem when a priority item is known
 *  SeekItem    ->  Explore  when item picked up or gone
 */
enum class ESurvivorState : uint8
{
	Explore,    // Wander, scan for items / threats
	SeekItem,   // Move toward highest-priority nearby item
	Fight,      // Face nearest zombie and shoot
	Flee,       // Run away from nearest zombie
	UseMedkit,  // Stand still, consume medkit
};

inline FString SurvivorStateToString(ESurvivorState State)
{
	switch (State)
	{
	case ESurvivorState::Explore:   return TEXT("Explore");
	case ESurvivorState::SeekItem:  return TEXT("SeekItem");
	case ESurvivorState::Fight:     return TEXT("Fight");
	case ESurvivorState::Flee:      return TEXT("Flee");
	case ESurvivorState::UseMedkit: return TEXT("UseMedkit");
	default:                        return TEXT("Unknown");
	}
}
