// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "ShooterWeapon_Projectile.h"

#include "ShooterProjectile.generated.h"

// 
UCLASS(Abstract, Blueprintable)
class AShooterProjectile : public AActor
{
	GENERATED_UCLASS_BODY()

	/** initial setup */
	virtual void PostInitializeComponents() override;

	/** setup velocity */
	void InitVelocity(FVector& ShootDirection);

	/** handle hit */
	UFUNCTION()
	void OnImpact(const FHitResult& HitResult);

private:
	/** movement component */
	UPROPERTY(VisibleDefaultsOnly, Category=Projectile)
	UProjectileMovementComponent* MovementComp;

	/** collisions */
	UPROPERTY(VisibleDefaultsOnly, Category=Projectile)
	USphereComponent* CollisionComp;

	UPROPERTY(VisibleDefaultsOnly, Category=Projectile)
	UParticleSystemComponent* ParticleComp;
protected:

	/** effects for explosion */
	UPROPERTY(EditDefaultsOnly, Category=Effects)
	TSubclassOf<class AShooterExplosionEffect> ExplosionTemplate;

	/** controller that fired me (cache for damage calculations) */
	TWeakObjectPtr<AController> MyController;

	/** projectile data */
	struct FProjectileWeaponData WeaponConfig;

	/** did it explode? */
	UPROPERTY(Transient, ReplicatedUsing=OnRep_Exploded)
	bool bExploded;

	/** [client] explosion happened */
	UFUNCTION()
	void OnRep_Exploded();

	/** trigger explosion */
	void Explode(const FHitResult& Impact);

	/** shutdown projectile and prepare for destruction */
	void DisableAndDestroy();

	/** update velocity on client */
	virtual void PostNetReceiveVelocity(const FVector& NewVelocity) override;

protected:
	/** Returns MovementComp subobject **/
	FORCEINLINE UProjectileMovementComponent* GetMovementComp() const { return MovementComp; }
	/** Returns CollisionComp subobject **/
	FORCEINLINE USphereComponent* GetCollisionComp() const { return CollisionComp; }
	/** Returns ParticleComp subobject **/
	FORCEINLINE UParticleSystemComponent* GetParticleComp() const { return ParticleComp; }

	//////////////////// ADDED EVERYTHING BELOW::::::::::

public:
	UFUNCTION(BlueprintCallable, Category = "Physics")
		virtual void LaunchProj(FVector LaunchVelocity);

	UFUNCTION()
		void OnBounce(const FHitResult& Impact, const FVector& Vec);
	UPROPERTY(EditDefaultsOnly, Category = Grenade)
		float FuseTime;
	UPROPERTY(EditDefaultsOnly, Category = Grenade)
		bool bArmOnBounce;
	/* User in conjunction with bArmOnBounce. When the velocity is below ArmVelocity, it will arm the grenade. 20 seems to be the lowest number that works*/
	UPROPERTY(EditDefaultsOnly, Category = Grenade)
		float ArmVelocity;
	UPROPERTY(EditDefaultsOnly, Category = Grenade)
		float Damage;
	UPROPERTY(EditDefaultsOnly, Category = Grenade)
		float ExplosionRadius;
	UPROPERTY(EditDefaultsOnly, Category = Grenade)
		TSubclassOf<UDamageType> DamageType;
	UPROPERTY()
		FHitResult MyImpact;
	UPROPERTY(EditDefaultsOnly, Category = Grenade)
		bool bIsGrenade;
	UPROPERTY(EditDefaultsOnly, Category = Projectile)
		float ProjectileLifeSpan;

	UPROPERTY()
		bool bHasBounced;

	void MyExplode();

	UPROPERTY(EditDefaultsOnly, Category = Grenade)
		float MinimumGrenadeDamage;
	UPROPERTY(EditDefaultsOnly, Category = Grenade)
		float MaxDamageRadius;
	UPROPERTY(EditDefaultsOnly, Category = Projectile)
		bool bUseMaxDamageRadius;

	UPROPERTY(EditDefaultsOnly, Category = Grenade)
		float HeadshotDamage;

	UPROPERTY(EditDefaultsOnly, Category = Grenade)
		float ShieldDamage;

	//////////////////// PLASMA STUN
	UPROPERTY(EditDefaultsOnly, category = Stun)
		bool bHasPlasmaStun;
	/** The percentage of movement taken away per shot. Example: If a character moves at 100 speed and stun amount is .5. After being hit with one bullet he will move at 50. A second bullet will make him move at 25 and so on. */
	UPROPERTY(EditDefaultsOnly, category = Stun)
		float StunAmount;
	/** The amount of time before movement speed resets.*/
	UPROPERTY(EditDefaultsOnly, category = Stun)
		float StunTime;

	//////////////////// PRO PIPE STUFF

	/** Check this is you want the projectile to only explode from the weapon calling the explode function on it. Basically this is only for the pro pipe! */
	UPROPERTY(EditDefaultsOnly, Category = Projectile)
		bool bOnlyExplodeFromWeaponCall;

	/** this is used for the pro pipe to declare the minimum amount of time that has to pass before the projectile can detonate */
	UPROPERTY(EditDefaultsOnly, Category = Projectile)
		float SafetyTimer;
	UPROPERTY(replicated)
		bool bSafetyExpired;

	UPROPERTY(EditDefaultsOnly, Category = Projectile)
		bool bUseSafety;

	void ExpireSafety();

	UPROPERTY(replicated)
		bool bWantsToExplode;
};
