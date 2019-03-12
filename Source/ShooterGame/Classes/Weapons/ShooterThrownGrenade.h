// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Weapons/ShooterProjectile.h"
#include "ShooterThrownGrenade.generated.h"

/**
 * 
 */
UCLASS()
class SHOOTERGAME_API AShooterThrownGrenade : public AShooterProjectile
{
	GENERATED_BODY()
	
		AShooterThrownGrenade(const FObjectInitializer& ObjectInitializer);
	
	void InitVelocity(FVector& ShootDirection);
	/*UFUNCTION()
	void OnBounce(const FHitResult& Impact);*/
	//UPROPERTY(EditDefaultsOnly, Category = Grenade)
	//float FuseTime;
	UPROPERTY(EditDefaultsOnly, Category = Grenade)
		float GrenadeDamage;
	//UPROPERTY(EditDefaultsOnly, Category = Grenade)
	//float ExplosionRadius;
	UPROPERTY(EditDefaultsOnly, Category = Grenade)
		TSubclassOf<UDamageType> GrenadeDamageType;
	//UPROPERTY()
	//FHitResult MyImpact;
	void MyExplode();

protected:
	void Explode(const FHitResult& Impact);
	
};
