// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Weapons/ShooterWeapon.h"
#include "ShooterWeapon_Charge.generated.h"

USTRUCT()
struct FChargeWeaponData
{
	GENERATED_USTRUCT_BODY()

		/** Charge time */
		UPROPERTY(EditDefaultsOnly, Category = Charge)
		float TotalChargeTime;

	/** the amount the CurrentChargeAmount is increased each time (time betweens increased is ChargeRate */
	UPROPERTY(VisibleAnywhere, Category = Charge)
		float ChargeIncrease;
	/** The Charge Rate. Set this to TotalChargeTime divided by ChargeIncrease. For now ChargeIncrease is always 10. So a 5 second charge would be 5 / 10 = .5 Charge rate.*/
	UPROPERTY(EditDefaultsOnly, Category = Charge)
		float ChargeRate;
	/** Whether or not this charge weapon is a shotgun */
	UPROPERTY(EditDefaultsOnly, Category = Charge)
		bool bIsShotgun;
	/** how many shells this gun should shoot, if shotgun */
	UPROPERTY(EditDefaultsOnly, Category = Charge)
		int32 Shells;

	/** projectile class non-charged */
	UPROPERTY(EditDefaultsOnly, Category = Charge)
		TSubclassOf<class AShooterProjectile> ProjectileClassNoCharge;
	/** projectile class Charge */
	UPROPERTY(EditDefaultsOnly, Category = Charge)
		TSubclassOf<class AShooterProjectile> ProjectileClassCharge;

	UPROPERTY(EditDefaultsOnly, Category = Charge)
		bool bIsCharge;

	// overheat stuff

//	UPROPERTY(EditDefaultsOnly, Category = Overheat)
//		bool bIsOverheat;
	/** the heat produced with each shot */
//	UPROPERTY(EditDefaultsOnly, Category = Overheat)
//		float HeatIncrease;
	UPROPERTY(EditDefaultsOnly, Category = Overheat)
		float ChargeShotHeatIncrease;
	/** the rate at which heat decreases. NOTE: this sets the timer for the next decrease.
	1.0 is one second. each time the current heat is decreased by 10 */
//	UPROPERTY(EditDefaultsOnly, Category = Overheat)
//		float CooldownRate;


	//dat comment doh

	/** defaults */
	FChargeWeaponData()
	{
		ProjectileClassNoCharge = NULL;
		ProjectileClassCharge = NULL;
		TotalChargeTime = 2;
		ChargeIncrease = 10;
		ChargeRate = TotalChargeTime / ChargeIncrease;
		bIsShotgun = false;
		Shells = 0;
	}
};

/**
 * 
 */
UCLASS()
class SHOOTERGAME_API AShooterWeapon_Charge : public AShooterWeapon
{
	GENERATED_BODY()
	
		AShooterWeapon_Charge(const FObjectInitializer& ObjectInitializer);
protected:
	/** Current Charge Amount */
	UPROPERTY(VisibleAnywhere, replicated, BlueprintReadOnly, Category = Charge)
		float CurrentChargeAmount;
private:
//	UPROPERTY(VisibleAnywhere, replicated, Category = Overheat)
//		float CurrentHeat;

	void ApplyWeaponConfig(FChargeWeaponData& Data);

	float GetCurrentSpread() const;

	UPROPERTY(EditDefaultsOnly, Category = Charge)
		float Spread;

	void UseAmmo();
protected:

	virtual EAmmoType GetAmmoType() const override
	{
		return EAmmoType::ERocket;
	}

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Config)
		FChargeWeaponData ChargeConfig;

	virtual void FireWeapon() override;

	virtual void StartFire() override;

	virtual void StopFire() override;

	virtual void OnBurstStarted() override;

	virtual void OnBurstFinished() override;

	void HandleCharging();

	void HandleFiring();

	UFUNCTION(reliable, server, WithValidation)
		void ServerHandleCharging();

	UFUNCTION(reliable, server, WithValidation)
		void ServerChargeWeapon();

	void ChargeWeapon();

	bool bShouldFire;

	float LastChargeTime;

	UFUNCTION(reliable, server, WithValidation)
		void ServerFireProjectile(FVector Origin, FVector_NetQuantizeNormal ShootDir);


	void Cooldown();

	UFUNCTION(reliable, server, WithValidation)
		void ServerCooldown();

	UFUNCTION(reliable, server, WithValidation)
		void IncreaseHeat();

	UFUNCTION(reliable, server, WithValidation)
		void IncreaseHeatFromChargeShot();

	void Overheat();

	UFUNCTION(reliable, server, WithValidation)
		void ServerOverheat();

//	UPROPERTY(VisibleAnywhere, replicated, Category = Overheat)
	//	bool bIsOverheated;
public:
	bool CanFire() const;

	virtual void StopReload() override;
};
