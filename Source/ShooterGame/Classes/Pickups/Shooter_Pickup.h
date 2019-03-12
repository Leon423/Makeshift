// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GameFramework/Actor.h"
#include "ShooterTypes.h"
#include "Shooter_Pickup.generated.h"

/**
 * 
 */
UCLASS()
class SHOOTERGAME_API AShooter_Pickup : public AActor
{
	GENERATED_BODY()
	
		AShooter_Pickup(const FObjectInitializer& ObjectInitializer);
	
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = MyStuff)
		TSubclassOf<class AShooterWeapon> Weapon_C;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = MyStuff)
		TEnumAsByte<EWeaponClassification::Type> WeaponClassification;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = MyStuff)
		int32 CurrentNumberOfBullets;
};
