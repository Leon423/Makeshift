// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GameFramework/Actor.h"
#include "ShooterPowerup.generated.h"

/**
 * 
 */
UCLASS()
class SHOOTERGAME_API AShooterPowerup : public AActor
{
	GENERATED_BODY()
	
	
		AShooterPowerup(const FObjectInitializer& ObjectInitializer);

	UPROPERTY(EditDefaultsOnly, Category = Powerup)
		bool bIsOvershield;
	
};
