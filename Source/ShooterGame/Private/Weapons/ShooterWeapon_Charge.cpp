// Fill out your copyright notice in the Description page of Project Settings.

#include "ShooterGame.h"
#include "ShooterGame/Classes/Weapons/ShooterWeapon_Charge.h"


AShooterWeapon_Charge::AShooterWeapon_Charge(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	ChargeConfig.ChargeRate = ChargeConfig.TotalChargeTime / ChargeConfig.ChargeIncrease;
}

////////////////////////////////////
/////////////// WEAPON USAGE

void AShooterWeapon_Charge::StartFire()
{
	//GEngine->AddOnScreenDebugMessage(-1, 120.0, FColor::Red, FString::Printf(TEXT("ChargeRate: %f"), ChargeConfig.ChargeRate));
	//GEngine->AddOnScreenDebugMessage(-1, 10.0f, FColor::Red, TEXT("START FIRING!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"));
	// this is where charging logic will happen.
	Super::StartFire();
}

void AShooterWeapon_Charge::StopFire()
{
	// this is where charging will stop and you will fire projectile based on charge
	Super::StopFire();
}

void AShooterWeapon_Charge::HandleFiring()
{
	//GEngine->AddOnScreenDebugMessage(-1, 10.0f, FColor::Red, TEXT("HandleFiring!"));
	if ((CurrentAmmoInClip > 0 || HasInfiniteClip() || HasInfiniteAmmo()) && CanFire() && CurrentHeat < 100)
	{
		if (GetNetMode() != NM_DedicatedServer)
		{
			SimulateWeaponFire();
		}

		if (MyPawn && MyPawn->IsLocallyControlled())
		{
			FireWeapon();

			UseAmmo();


			// update firing FX on remote clients if function was called on server
			BurstCounter++;
		}
	}
	else if (CanReload())
	{
		StartReload();
	}
	else if (MyPawn && MyPawn->IsLocallyControlled())
	{
		if (GetCurrentAmmo() == 0 && !bRefiring)
		{
			PlayWeaponSound(OutOfAmmoSound);
			AShooterPlayerController* MyPC = Cast<AShooterPlayerController>(MyPawn->Controller);
			AShooterHUD* MyHUD = MyPC ? Cast<AShooterHUD>(MyPC->GetHUD()) : NULL;
			if (MyHUD)
			{
				MyHUD->NotifyOutOfAmmo();
			}
		}

		// stop weapon fire FX, but stay in Firing state
		if (BurstCounter > 0)
		{
			OnBurstFinished();
		}
	}

	if (MyPawn && MyPawn->IsLocallyControlled())
	{
		// local client will notify server
		if (Role < ROLE_Authority)
		{
			ServerHandleFiring();
		}

		// reload after firing last round
		if (CurrentAmmoInClip <= 0 && CanReload())
		{
			StartReload();
		}

		// setup refire timer
		bRefiring = (CurrentState == EWeaponState::Firing && WeaponConfig.TimeBetweenShots > 0.0f && bIsAutomatic);
		if (bRefiring)
		{
			GetWorldTimerManager().SetTimer(this, &AShooterWeapon_Charge::HandleFiring, WeaponConfig.TimeBetweenShots, false);
		}
		else
		{
			LastFireTime = GetWorld()->GetTimeSeconds();
			StopFire();
			return;
		}
	}

	LastFireTime = GetWorld()->GetTimeSeconds();
}

void AShooterWeapon_Charge::OnBurstStarted()
{
//	GEngine->AddOnScreenDebugMessage(-1, 10.0f, FColor::Red, TEXT("OnBurstStarted!"));
	if (ChargeConfig.bIsCharge)
	{
		if (bShouldFire)
		{
			// start firing, can be delayed to satisfy TimeBetweenShots
			const float GameTime = GetWorld()->GetTimeSeconds();
			if (LastFireTime > 0 && WeaponConfig.TimeBetweenShots > 0.0f &&
				LastFireTime + WeaponConfig.TimeBetweenShots > GameTime)
			{
				GetWorldTimerManager().SetTimer(this, &AShooterWeapon_Charge::HandleFiring, LastFireTime + WeaponConfig.TimeBetweenShots - GameTime, false);
			}
			else
			{
				HandleFiring();
			}

			bShouldFire = false;
		}
		else
		{
			//GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Cyan, TEXT("ONBURSTSTARTED!!!!!!!!!!!!!!!!!!!!!!!!!!!"));
			// start charging can be delayed to satisfy chargerate
			const float GameTime = GetWorld()->GetTimeSeconds();

			if (LastChargeTime > 0 && ChargeConfig.ChargeRate > 0.0f &&
				LastChargeTime + ChargeConfig.ChargeRate > GameTime)
			{
				GetWorldTimerManager().SetTimer(this, &AShooterWeapon_Charge::HandleCharging, LastChargeTime + ChargeConfig.ChargeRate - GameTime, false);
			}
			else
			{
				HandleCharging();
			}
		}
	}
	else if (bIsOverheat)
	{
		// start firing, can be delayed to satisfy TimeBetweenShots
		const float GameTime = GetWorld()->GetTimeSeconds();
		if (LastFireTime > 0 && WeaponConfig.TimeBetweenShots > 0.0f &&
			LastFireTime + WeaponConfig.TimeBetweenShots > GameTime)
		{
			GetWorldTimerManager().SetTimer(this, &AShooterWeapon_Charge::HandleFiring, LastFireTime + WeaponConfig.TimeBetweenShots - GameTime, false);
		}
		else
		{
			HandleFiring();
		}

		if (CurrentHeat == 90 && Role < ROLE_Authority)
		{
			CurrentHeat = 100;
		}

		if (CurrentHeat >= 100)
		{
			// overheated from that last shot so we cant fire for some time
			//GEngine->AddOnScreenDebugMessage(-1, 10.0f, FColor::Green, TEXT("CurrentHeat >= 100"));
			Overheat();
		}
	}
	else
	{
		Super::OnBurstStarted();
	}
}

void AShooterWeapon_Charge::HandleCharging()
{
	//GEngine->AddOnScreenDebugMessage(-1, 10.0f, FColor::Red, TEXT("HandleCharging!"));

	//GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Cyan, TEXT("HANDLE CHARGING!!!!!!!!!!!!!!!!!!!!!!!!!!"));
	bShouldFire = false;
	if ((CurrentAmmoInClip > 0 || HasInfiniteClip() || HasInfiniteAmmo()) && CanFire())
	{
		if (GetNetMode() != NM_DedicatedServer)
		{
			//SimulateFire equivalent to charging
		}

		if (MyPawn && MyPawn->IsLocallyControlled())
		{
			ChargeWeapon();
		}
	}
	else if (CanReload())
	{
		StartReload();
	}
	else if (MyPawn && MyPawn->IsLocallyControlled())
	{
		if (GetCurrentAmmo() == 0 && !bRefiring)
		{
			PlayWeaponSound(OutOfAmmoSound);
			AShooterPlayerController* MyPC = Cast<AShooterPlayerController>(MyPawn->Controller);
			AShooterHUD* MyHUD = MyPC ? Cast<AShooterHUD>(MyPC->GetHUD()) : NULL;
			if (MyHUD)
			{
				MyHUD->NotifyOutOfAmmo();
			}

			if (BurstCounter > 0)
			{
				OnBurstFinished();
			}
		}
	}

	if (MyPawn && MyPawn->IsLocallyControlled())
	{
		if (Role < ROLE_Authority)
		{
			//GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, TEXT("ServerHandleCharging should be called!"));
			ServerHandleCharging();
		}

		bRefiring = (CurrentState == EWeaponState::Firing && ChargeConfig.ChargeRate > 0.0f);
		if (bRefiring)
		{
			GetWorldTimerManager().SetTimer(this, &AShooterWeapon_Charge::HandleCharging, ChargeConfig.ChargeRate, false);
		}
		else
		{
			LastChargeTime = GetWorld()->GetTimeSeconds();
			StopFire();
			return;
		}
	}

	LastChargeTime = GetWorld()->GetTimeSeconds();
}

void AShooterWeapon_Charge::ServerHandleCharging_Implementation()
{
	//GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("Charge amount, server: %f"), CurrentChargeAmount));
	HandleCharging();
}

bool AShooterWeapon_Charge::ServerHandleCharging_Validate()
{
	return true;
}

void AShooterWeapon_Charge::OnBurstFinished()
{
	if (bIsOverheat)
	{
		if (CurrentHeat >= 100)
		{
			Overheat();
		}
	}

	//GEngine->AddOnScreenDebugMessage(-1, 10.0f, FColor::Red, TEXT("ONBURSTFINISHED!"));
	if (ChargeConfig.bIsCharge)
	{
		if (bShouldFire == false)
		{
			if (GetNetMode() != NM_DedicatedServer)
			{
				// StopSimulateWeaponFire equivalent for charging
			}

			bShouldFire = true;
			OnBurstStarted();

			GetWorldTimerManager().ClearTimer(this, &AShooterWeapon_Charge::HandleCharging);
			GetWorldTimerManager().ClearTimer(this, &AShooterWeapon_Charge::HandleFiring);
			bRefiring = false;
			//CurrentChargeAmount = 0;
		}
		else
		{
			bShouldFire = false;
			CurrentChargeAmount = 0;
			Super::OnBurstFinished();
		}
		
	}
	else
	{
		Super::OnBurstFinished();
	}
}

void AShooterWeapon_Charge::ChargeWeapon()
{
	//CurrentChargeAmount = CurrentChargeAmount + ChargeConfig.ChargeIncrease;
	ServerChargeWeapon();
	//GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Cyan, TEXT("CHARGING!!!!!!!!!!!!!!!!!!!!!!!!!!"));
}

bool AShooterWeapon_Charge::ServerChargeWeapon_Validate()
{
	return true;
}

void AShooterWeapon_Charge::ServerChargeWeapon_Implementation()
{
	//GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Green, TEXT("ServerChargeWeapon"));
	CurrentChargeAmount = CurrentChargeAmount + ChargeConfig.ChargeIncrease;
}

void AShooterWeapon_Charge::FireWeapon()
{
	//GEngine->AddOnScreenDebugMessage(-1, 10.0f, FColor::Red, TEXT("FireWeapon!"));
	if (bIsOverheat)
	{
		IncreaseHeat();
	}
	FVector ShootDir = GetAdjustedAim();
	FVector Origin = GetMuzzleLocation();

	/*if (ChargeConfig.bIsShotgun)
	{
	// is a shotgun do this spread shit
	}
	else
	{*/
	// not a shotgun do normal shooty stuff
	const float ProjectileAdjustRange = 10000.0f;
	const FVector StartTrace = GetCameraDamageStartLocation(ShootDir);
	const FVector EndTrace = StartTrace + ShootDir * ProjectileAdjustRange;
	FHitResult Impact = WeaponTrace(StartTrace, EndTrace);

	if (Impact.bBlockingHit)
	{
		const FVector AdjustedDir = (Impact.ImpactPoint - Origin).SafeNormal();
		bool bWeaponPenetration = false;

		const float DirectionDot = FVector::DotProduct(AdjustedDir, ShootDir);
		if (DirectionDot < 0.0f)
		{
			// shooting backwards = weapon is penetrating
			bWeaponPenetration = true;
		}
		else if (DirectionDot < 0.5f)
		{
			// check for weapon penetration if angle difference is big enough
			// raycast along weapon mesh to check if there's blocking hit

			FVector MuzzleStartTrace = Origin - GetMuzzleDirection() * 150.0f;
			FVector MuzzleEndTrace = Origin;
			FHitResult MuzzleImpact = WeaponTrace(MuzzleStartTrace, MuzzleEndTrace);

			if (MuzzleImpact.bBlockingHit)
			{
				bWeaponPenetration = true;
			}
		}

		if (bWeaponPenetration)
		{
			// spawn at crosshair position
			Origin = Impact.ImpactPoint - ShootDir * 10.0f;
		}
		else
		{
			// adjust direction to hit
			ShootDir = AdjustedDir;
		}
	}

	ServerFireProjectile(Origin, ShootDir);

	//}

	OnBurstFinished();
}

bool AShooterWeapon_Charge::ServerFireProjectile_Validate(FVector Origin, FVector_NetQuantizeNormal ShootDir)
{
	return true;
}

void AShooterWeapon_Charge::ServerFireProjectile_Implementation(FVector Origin, FVector_NetQuantizeNormal ShootDir)
{
	if (CurrentChargeAmount < 100)
	{
		// normal shot
		if (ChargeConfig.bIsShotgun)
		{

			FTransform SpawnTM(ShootDir.Rotation(), Origin);



			for (int32 i = 0; i < ChargeConfig.Shells; i++)
			{
				if (i == 0)
				{
					// first projectile will always fly straight and true!
					AShooterProjectile* Projectile = Cast<AShooterProjectile>(UGameplayStatics::BeginSpawningActorFromClass(this, ChargeConfig.ProjectileClassNoCharge, SpawnTM));
					if (Projectile)
					{
						Projectile->Instigator = Instigator;
						Projectile->SetOwner(this);
						Projectile->InitVelocity(ShootDir);

						UGameplayStatics::FinishSpawningActor(Projectile, SpawnTM);
					}
				}
				else
				{
					// we need to do a new trace everytime to do spread
					const int32 RandomSeed = FMath::Rand();
					FRandomStream WeaponRandomStream(RandomSeed);
					const float CurrentSpread = GetCurrentSpread();
					const float ConeHalfAngle = FMath::DegreesToRadians(CurrentSpread * 0.5f);

					FVector AimDir = GetAdjustedAim();
					const FVector StartTrace = GetCameraDamageStartLocation(AimDir);
					FVector newShootDir = WeaponRandomStream.VRandCone(AimDir, ConeHalfAngle, ConeHalfAngle);
					//const FVector EndTrace = StartTrace + ShootDir * 10000.0f;

					AShooterProjectile* Projectile = Cast<AShooterProjectile>(UGameplayStatics::BeginSpawningActorFromClass(this, ChargeConfig.ProjectileClassNoCharge, SpawnTM));
					if (Projectile)
					{
						Projectile->Instigator = Instigator;
						Projectile->SetOwner(this);
						Projectile->InitVelocity(newShootDir);

						UGameplayStatics::FinishSpawningActor(Projectile, SpawnTM);
					}

				}
			}
		}
		else
		{
			FTransform SpawnTM(ShootDir.Rotation(), Origin);
			AShooterProjectile* Projectile = Cast<AShooterProjectile>(UGameplayStatics::BeginSpawningActorFromClass(this, ChargeConfig.ProjectileClassNoCharge, SpawnTM));
			if (Projectile)
			{
				Projectile->Instigator = Instigator;
				Projectile->SetOwner(this);
				Projectile->InitVelocity(ShootDir);

				UGameplayStatics::FinishSpawningActor(Projectile, SpawnTM);
			}
		}
	}
	else
	{
		// charge shot
		FTransform SpawnTM(ShootDir.Rotation(), Origin);
		AShooterProjectile* Projectile = Cast<AShooterProjectile>(UGameplayStatics::BeginSpawningActorFromClass(this, ChargeConfig.ProjectileClassCharge, SpawnTM));
		if (Projectile)
		{
			Projectile->Instigator = Instigator;
			Projectile->SetOwner(this);
			Projectile->InitVelocity(ShootDir);

			UGameplayStatics::FinishSpawningActor(Projectile, SpawnTM);
		}

		IncreaseHeatFromChargeShot();
	}

	CurrentChargeAmount = 0;
}

float AShooterWeapon_Charge::GetCurrentSpread() const
{
	float FinalSpread;

	if (CurrentChargeAmount < 100)
	{
		// would add things to change spread based on charge here
		if (CurrentChargeAmount > 74)
		{
			FinalSpread = Spread / 4;
		}
		else if (CurrentChargeAmount > 49)
		{
			FinalSpread = Spread / 3;
		}
		else if (CurrentChargeAmount > 24)
		{
			FinalSpread = Spread / 2;
		}
		else
		{
			FinalSpread = Spread;
		}
	}
	else
	{
		FinalSpread = 0;
	}

	return FinalSpread;
}

/*void AShooterWeapon_Charge::ApplyWeaponConfig(FChargeWeaponData& Data)
{
Data = ChargeConfig;
}*/

void AShooterWeapon_Charge::GetLifetimeReplicatedProps(TArray< FLifetimeProperty > & OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AShooterWeapon_Charge, CurrentChargeAmount);
	DOREPLIFETIME(AShooterWeapon_Charge, CurrentHeat);
	DOREPLIFETIME(AShooterWeapon_Charge, bIsOverheated);
}

void AShooterWeapon_Charge::UseAmmo()
{
	if (bIsOverheat)
	{
		if (!HasInfiniteAmmo())
		{
			CurrentAmmo--;
		}

		if (!HasInfiniteAmmo() && !HasInfiniteClip())
		{
			CurrentAmmo--;
		}

		AShooterAIController* BotAI = MyPawn ? Cast<AShooterAIController>(MyPawn->GetController()) : NULL;
		AShooterPlayerController* PlayerController = MyPawn ? Cast<AShooterPlayerController>(MyPawn->GetController()) : NULL;
		if (BotAI)
		{
			BotAI->CheckAmmo(this);
		}
		else if (PlayerController)
		{
			AShooterPlayerState* PlayerState = Cast<AShooterPlayerState>(PlayerController->PlayerState);
			switch (GetAmmoType())
			{
			case EAmmoType::ERocket:
				PlayerState->AddRocketsFired(1);
				break;
			case EAmmoType::EBullet:
			default:
				PlayerState->AddBulletsFired(1);
				break;
			}
		}
	}
	else
	{
		Super::UseAmmo();
	}
}

void AShooterWeapon_Charge::Cooldown()
{
	GEngine->AddOnScreenDebugMessage(-1, 1.0, FColor::Black, TEXT("Cooldown"));
	ServerCooldown();

	if (CurrentHeat == 0)
	{
		bIsOverheated = false;
		GetWorldTimerManager().ClearTimer(this, &AShooterWeapon_Charge::Cooldown);
	}
}

bool AShooterWeapon_Charge::ServerCooldown_Validate()
{
	return true;
}

void AShooterWeapon_Charge::ServerCooldown_Implementation()
{
	//GEngine->AddOnScreenDebugMessage(-1, 1.0, FColor::Black, TEXT("ServerCooldown"));
	//GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("heat amount, server: %f"), CurrentHeat));

	if (CurrentHeat <= 0)
	{
		CurrentHeat = 0;
		bIsOverheated = false;
		GetWorldTimerManager().ClearTimer(this, &AShooterWeapon_Charge::Cooldown);
	}
	else
	{
		CurrentHeat -= 10;
	}
}

bool AShooterWeapon_Charge::IncreaseHeat_Validate()
{
	return true;
}

void AShooterWeapon_Charge::IncreaseHeat_Implementation()
{
	//GEngine->AddOnScreenDebugMessage(-1, 1.0, FColor::Black, TEXT("Heat ++++++"));
	if (GetWorldTimerManager().IsTimerActive(this, &AShooterWeapon_Charge::Cooldown))
	{
		GetWorldTimerManager().ClearTimer(this, &AShooterWeapon_Charge::Cooldown);
	}
	CurrentHeat += HeatIncrease;
	GetWorldTimerManager().SetTimer(this, &AShooterWeapon_Charge::Cooldown, CooldownRate, true);

}

bool AShooterWeapon_Charge::IncreaseHeatFromChargeShot_Validate()
{
	return true;
}

void AShooterWeapon_Charge::IncreaseHeatFromChargeShot_Implementation()
{
	GEngine->AddOnScreenDebugMessage(-1, 1.0, FColor::Black, TEXT("HeatFromChargeShot ++++++"));
	if (GetWorldTimerManager().IsTimerActive(this, &AShooterWeapon_Charge::Cooldown))
	{
		GetWorldTimerManager().ClearTimer(this, &AShooterWeapon_Charge::Cooldown);
	}
	CurrentHeat += ChargeConfig.ChargeShotHeatIncrease;
	GetWorldTimerManager().SetTimer(this, &AShooterWeapon_Charge::Cooldown, CooldownRate, true);
}

void AShooterWeapon_Charge::Overheat()
{
	GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Blue, TEXT("OVERHEATED!!"));
	if (Role < ROLE_Authority)
	{
		ServerOverheat();
	}

	bIsOverheated = true;
	// possibly: play overheat animations here
}

bool AShooterWeapon_Charge::ServerOverheat_Validate()
{
	return true;
}

void AShooterWeapon_Charge::ServerOverheat_Implementation()
{
	Overheat();
}

bool AShooterWeapon_Charge::CanFire() const
{
	bool bCanFire = MyPawn && MyPawn->CanFire();
	bool bStateOKToFire = ((CurrentState == EWeaponState::Idle) || (CurrentState == EWeaponState::Firing));
	return ((bCanFire == true) && (bStateOKToFire == true) && (bPendingReload == false) && (bIsOverheated == false));
}

void AShooterWeapon_Charge::StopReload()
{
	Super::StopReload();

	if (bPendingReload)
	{
		bPendingReload = false;
	}
}