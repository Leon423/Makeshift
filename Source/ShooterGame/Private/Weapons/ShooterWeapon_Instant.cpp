// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "ShooterGame.h"
#include "Particles/ParticleSystemComponent.h"

AShooterWeapon_Instant::AShooterWeapon_Instant(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	CurrentFiringSpread = 0.0f;
}

//////////////////////////////////////////////////////////////////////////
// Weapon usage

// changed stuff

void AShooterWeapon_Instant::FireWeapon()
{
	if (bIsOverheat)
	{
		//IncreaseHeat();
	}

	if (bIsShotgun)
	{
		const FVector AimDir = GetAdjustedAim();
		const FVector StartTrace = GetCameraDamageStartLocation(AimDir);

		for (int32 i = 0; i < Shells; i++)
		{
			const int32 RandomSeed = FMath::Rand();
			FRandomStream WeaponRandomStream(RandomSeed);
			const float CurrentSpread = GetCurrentSpread();
			const float ConeHalfAngle = FMath::DegreesToRadians(CurrentSpread * 0.5f);

			const FVector ShootDir = WeaponRandomStream.VRandCone(AimDir, ConeHalfAngle, ConeHalfAngle);
			const FVector EndTrace = StartTrace + ShootDir * InstantConfig.WeaponRange;

			const FHitResult Impact = WeaponTrace(StartTrace, EndTrace);

			ProcessInstantHit(Impact, StartTrace, ShootDir, RandomSeed, CurrentSpread);

			CurrentFiringSpread = FMath::Min(InstantConfig.FiringSpreadMax, CurrentFiringSpread + InstantConfig.FiringSpreadIncrement);
		}
	}
	else
	{


		const int32 RandomSeed = FMath::Rand();
		FRandomStream WeaponRandomStream(RandomSeed);
		const float CurrentSpread = GetCurrentSpread();
		const float ConeHalfAngle = FMath::DegreesToRadians(CurrentSpread * 0.5f);

		const FVector AimDir = GetAdjustedAim();
		const FVector StartTrace = GetCameraDamageStartLocation(AimDir);
		const FVector ShootDir = WeaponRandomStream.VRandCone(AimDir, ConeHalfAngle, ConeHalfAngle);
		const FVector EndTrace = StartTrace + ShootDir * InstantConfig.WeaponRange;

		const FHitResult Impact = WeaponTrace(StartTrace, EndTrace);


		ProcessInstantHit(Impact, StartTrace, ShootDir, RandomSeed, CurrentSpread);



		CurrentFiringSpread = FMath::Min(InstantConfig.FiringSpreadMax, CurrentFiringSpread + InstantConfig.FiringSpreadIncrement);
	}
	/*if (bIsOverheat && CurrentHeat >= 100)
	{
		bIsOverheated = true;
		OnBurstFinished();
	}*/
}

bool AShooterWeapon_Instant::ServerNotifyHit_Validate(const FHitResult Impact, FVector_NetQuantizeNormal ShootDir, int32 RandomSeed, float ReticleSpread)
{
	return true;
}

// CHANGED STUFF IN THIS
void AShooterWeapon_Instant::ServerNotifyHit_Implementation(const FHitResult Impact, FVector_NetQuantizeNormal ShootDir, int32 RandomSeed, float ReticleSpread)
{
	const float WeaponAngleDot = FMath::Abs(FMath::Sin(ReticleSpread * PI / 180.f));

	bool IgnoreDotCheck = false;

	if (Impact.GetActor())
	{
		FVector TargetLoc = Impact.GetActor()->GetActorLocation();

		FVector MyLoc = MyPawn->GetActorLocation();


		float f = MyLoc.Dist(MyLoc, TargetLoc);
		//GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, FString::Printf(TEXT("f: %f"), f));
		if (f < 170)
		{
			IgnoreDotCheck = true;
		}
	}

	/*if ((MyX - TarX) < 5 || (TarX - MyX) < 5 || (MyY - TarY) < 5 || (TarY - MyY) < 5)
	{
	GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Green, TEXT("less than 5 units away from eachother, assume the client is correct about the dot product"));
	IgnoreDotCheck = true;
	}*/

	// if we have an instigator, calculate dot between the view and the shot
	if (Instigator && (Impact.GetActor() || Impact.bBlockingHit))
	{
		const FVector Origin = GetMuzzleLocation();
		const FVector ViewDir = (Impact.Location - Origin).SafeNormal();

		// is the angle between the hit and the view within allowed limits (limit + weapon max angle)
		const float ViewDotHitDir = FVector::DotProduct(Instigator->GetViewRotation().Vector(), ViewDir);
		if ((ViewDotHitDir > InstantConfig.AllowedViewDotHitDir - WeaponAngleDot) || IgnoreDotCheck == true)
		{
			if (CurrentState != EWeaponState::Idle)
			{
				if (Impact.GetActor() == NULL)
				{
					if (Impact.bBlockingHit)
					{
						ProcessInstantHit_Confirmed(Impact, Origin, ShootDir, RandomSeed, ReticleSpread);
					}
				}
				// assume it told the truth about static things because the don't move and the hit 
				// usually doesn't have significant gameplay implications
				else if (Impact.GetActor()->IsRootComponentStatic() || Impact.GetActor()->IsRootComponentStationary())
				{
					ProcessInstantHit_Confirmed(Impact, Origin, ShootDir, RandomSeed, ReticleSpread);
				}
				else
				{
					// Get the component bounding box
					const FBox HitBox = Impact.GetActor()->GetComponentsBoundingBox();

					// calculate the box extent, and increase by a leeway
					FVector BoxExtent = 0.5 * (HitBox.Max - HitBox.Min);
					BoxExtent *= InstantConfig.ClientSideHitLeeway;

					// avoid precision errors with really thin objects
					BoxExtent.X = FMath::Max(20.0f, BoxExtent.X);
					BoxExtent.Y = FMath::Max(20.0f, BoxExtent.Y);
					BoxExtent.Z = FMath::Max(20.0f, BoxExtent.Z);

					// Get the box center
					const FVector BoxCenter = (HitBox.Min + HitBox.Max) * 0.5;

					// if we are within client tolerance
					if (FMath::Abs(Impact.Location.Z - BoxCenter.Z) < BoxExtent.Z &&
						FMath::Abs(Impact.Location.X - BoxCenter.X) < BoxExtent.X &&
						FMath::Abs(Impact.Location.Y - BoxCenter.Y) < BoxExtent.Y)
					{
						ProcessInstantHit_Confirmed(Impact, Origin, ShootDir, RandomSeed, ReticleSpread);
					}
					else
					{
						UE_LOG(LogShooterWeapon, Log, TEXT("%s Rejected client side hit of %s (outside bounding box tolerance)"), *GetNameSafe(this), *GetNameSafe(Impact.GetActor()));
						GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Green, TEXT("Rejected client side hit, outside bounding box tolerance"));
					}
					//GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("Charge amount, server: %f"), CurrentChargeAmount));
					GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("ViewDotHitDir registered: %f"), ViewDotHitDir));
				}
			}
		}
		else if (ViewDotHitDir <= InstantConfig.AllowedViewDotHitDir)
		{
			UE_LOG(LogShooterWeapon, Log, TEXT("%s Rejected client side hit of %s (facing too far from the hit direction)"), *GetNameSafe(this), *GetNameSafe(Impact.GetActor()));
			GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Green, TEXT("Rejected client side hit, facing too far from the hit direction"));
			GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Red, FString::Printf(TEXT("ViewDotHitDir not registered: %f"), ViewDotHitDir));
		}
		else
		{
			UE_LOG(LogShooterWeapon, Log, TEXT("%s Rejected client side hit of %s"), *GetNameSafe(this), *GetNameSafe(Impact.GetActor()));
			GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Green, TEXT("Rejected client side hit, whatever this is"));
		}
	}
}

bool AShooterWeapon_Instant::ServerNotifyMiss_Validate(FVector_NetQuantizeNormal ShootDir, int32 RandomSeed, float ReticleSpread)
{
	return true;
}

void AShooterWeapon_Instant::ServerNotifyMiss_Implementation(FVector_NetQuantizeNormal ShootDir, int32 RandomSeed, float ReticleSpread)
{
	const FVector Origin = GetMuzzleLocation();

	// play FX on remote clients
	HitNotify.Origin = Origin;
	HitNotify.RandomSeed = RandomSeed;
	HitNotify.ReticleSpread = ReticleSpread;

	// play FX locally
	if (GetNetMode() != NM_DedicatedServer)
	{
		const FVector EndTrace = Origin + ShootDir * InstantConfig.WeaponRange;
		SpawnTrailEffect(EndTrace);
	}
}

void AShooterWeapon_Instant::ProcessInstantHit(const FHitResult& Impact, const FVector& Origin, const FVector& ShootDir, int32 RandomSeed, float ReticleSpread)
{
	if (MyPawn && MyPawn->IsLocallyControlled() && GetNetMode() == NM_Client)
	{
		// if we're a client and we've hit something that is being controlled by the server
		if (Impact.GetActor() && Impact.GetActor()->GetRemoteRole() == ROLE_Authority)
		{
			// notify the server of the hit
			ServerNotifyHit(Impact, ShootDir, RandomSeed, ReticleSpread);
		}
		else if (Impact.GetActor() == NULL)
		{
			if (Impact.bBlockingHit)
			{
				// notify the server of the hit
				ServerNotifyHit(Impact, ShootDir, RandomSeed, ReticleSpread);
			}
			else
			{
				// notify server of the miss
				ServerNotifyMiss(ShootDir, RandomSeed, ReticleSpread);
			}
		}
	}

	// process a confirmed hit
	ProcessInstantHit_Confirmed(Impact, Origin, ShootDir, RandomSeed, ReticleSpread);
}

void AShooterWeapon_Instant::ProcessInstantHit_Confirmed(const FHitResult& Impact, const FVector& Origin, const FVector& ShootDir, int32 RandomSeed, float ReticleSpread)
{
	// handle damage
	if (ShouldDealDamage(Impact.GetActor()))
	{
		DealDamage(Impact, ShootDir);
	}

	// play FX on remote clients
	if (Role == ROLE_Authority)
	{
		HitNotify.Origin = Origin;
		HitNotify.RandomSeed = RandomSeed;
		HitNotify.ReticleSpread = ReticleSpread;
	}

	// play FX locally
	if (GetNetMode() != NM_DedicatedServer)
	{
		const FVector EndTrace = Origin + ShootDir * InstantConfig.WeaponRange;
		const FVector EndPoint = Impact.GetActor() ? Impact.ImpactPoint : EndTrace;

		SpawnTrailEffect(EndPoint);
		SpawnImpactEffects(Impact);
	}
}

bool AShooterWeapon_Instant::ShouldDealDamage(AActor* TestActor) const
{
	// if we're an actor on the server, or the actor's role is authoritative, we should register damage
	if (TestActor)
	{
		if (GetNetMode() != NM_Client ||
			TestActor->Role == ROLE_Authority ||
			TestActor->bTearOff)
		{
			return true;
		}
	}

	return false;
}

// CHANGED STUFF IN THIS
void AShooterWeapon_Instant::DealDamage(const FHitResult& Impact, const FVector& ShootDir)
{
	FPointDamageEvent PointDmg;
	PointDmg.DamageTypeClass = InstantConfig.DamageType;
	PointDmg.HitInfo = Impact;
	PointDmg.ShotDirection = ShootDir;
	PointDmg.Damage = InstantConfig.HitDamage;

	if (PointDmg.HitInfo.BoneName == "b_head" || Impact.BoneName == "Head")
	{
		GEngine->AddOnScreenDebugMessage(-1, 10.0f, FColor::Red, TEXT("HEADSHOT"));
		Cast<AShooterCharacter>(Impact.GetActor())->EndZoom();
		PointDmg.Damage = Cast<AShooterCharacter>(Impact.GetActor())->CalculateDamageToUse(PointDmg.Damage, PointDmg, MyPawn->Controller, this, HeadshotDamage, ShieldDamage);
	}
	else
	{
		if (Cast<AShooterCharacter>(Impact.GetActor()))
		{
			PointDmg.Damage = Cast<AShooterCharacter>(Impact.GetActor())->CalculateDamageToUse(PointDmg.Damage, PointDmg, MyPawn->Controller, this, 0.0, ShieldDamage);
		}
		else
		{
			//Impact.GetActor()->TakeDamage(PointDmg.Damage, PointDmg, MyPawn->Controller, this);
		}
	}

	Impact.GetActor()->TakeDamage(PointDmg.Damage, PointDmg, MyPawn->Controller, this);

	if (bHasPlasmaStun)
	{
		if (Cast<AShooterCharacter>(Impact.GetActor()))
		{
			Cast<AShooterCharacter>(Impact.GetActor())->ApplyPlasmaStun(StunAmount, StunTime);
		}
	}
}

void AShooterWeapon_Instant::OnBurstFinished()
{
	Super::OnBurstFinished();

	if (bIsOverheat)
	{
		if (CurrentHeat >= 100)
		{
			Overheat();
		}
	}

	CurrentFiringSpread = 0.0f;
}


//////////////////////////////////////////////////////////////////////////
// Weapon usage helpers

float AShooterWeapon_Instant::GetCurrentSpread() const
{
	float FinalSpread = InstantConfig.WeaponSpread + CurrentFiringSpread;
	if (MyPawn && MyPawn->IsTargeting())
	{
		FinalSpread *= InstantConfig.TargetingSpreadMod;
	}

	return FinalSpread;
}


//////////////////////////////////////////////////////////////////////////
// Replication & effects

void AShooterWeapon_Instant::OnRep_HitNotify()
{
	SimulateInstantHit(HitNotify.Origin, HitNotify.RandomSeed, HitNotify.ReticleSpread);
}

void AShooterWeapon_Instant::SimulateInstantHit(const FVector& ShotOrigin, int32 RandomSeed, float ReticleSpread)
{
	FRandomStream WeaponRandomStream(RandomSeed);
	const float ConeHalfAngle = FMath::DegreesToRadians(ReticleSpread * 0.5f);

	const FVector StartTrace = ShotOrigin;
	const FVector AimDir = GetAdjustedAim();
	const FVector ShootDir = WeaponRandomStream.VRandCone(AimDir, ConeHalfAngle, ConeHalfAngle);
	const FVector EndTrace = StartTrace + ShootDir * InstantConfig.WeaponRange;

	FHitResult Impact = WeaponTrace(StartTrace, EndTrace);
	if (Impact.bBlockingHit)
	{
		SpawnImpactEffects(Impact);
		SpawnTrailEffect(Impact.ImpactPoint);
	}
	else
	{
		SpawnTrailEffect(EndTrace);
	}
}

void AShooterWeapon_Instant::SpawnImpactEffects(const FHitResult& Impact)
{
	if (ImpactTemplate && Impact.bBlockingHit)
	{
		FHitResult UseImpact = Impact;

		// trace again to find component lost during replication
		if (!Impact.Component.IsValid())
		{
			const FVector StartTrace = Impact.ImpactPoint + Impact.ImpactNormal * 10.0f;
			const FVector EndTrace = Impact.ImpactPoint - Impact.ImpactNormal * 10.0f;
			FHitResult Hit = WeaponTrace(StartTrace, EndTrace);
			UseImpact = Hit;
		}

		AShooterImpactEffect* EffectActor = GetWorld()->SpawnActorDeferred<AShooterImpactEffect>(ImpactTemplate, Impact.ImpactPoint, Impact.ImpactNormal.Rotation());
		if (EffectActor)
		{
			EffectActor->SurfaceHit = UseImpact;
			UGameplayStatics::FinishSpawningActor(EffectActor, FTransform(Impact.ImpactNormal.Rotation(), Impact.ImpactPoint));
		}
	}
}

void AShooterWeapon_Instant::SpawnTrailEffect(const FVector& EndPoint)
{
	if (TrailFX)
	{
		const FVector Origin = GetMuzzleLocation();

		UParticleSystemComponent* TrailPSC = UGameplayStatics::SpawnEmitterAtLocation(this, TrailFX, Origin);
		if (TrailPSC)
		{
			TrailPSC->SetVectorParameter(TrailTargetParam, EndPoint);
		}
	}
}
// changed stuff
void AShooterWeapon_Instant::GetLifetimeReplicatedProps( TArray< FLifetimeProperty > & OutLifetimeProps ) const
{
	Super::GetLifetimeReplicatedProps( OutLifetimeProps );

	DOREPLIFETIME_CONDITION( AShooterWeapon_Instant, HitNotify, COND_SkipOwner );

	DOREPLIFETIME(AShooterWeapon_Instant, CurrentHeat);
	//DOREPLIFETIME(AShooterWeapon_Instant, bIsOverheated);
}

// ADDED ALL THIS

void AShooterWeapon_Instant::Cooldown()
{
	//GEngine->AddOnScreenDebugMessage(-1, 1.0, FColor::Black, TEXT("Cooldown"));
	ServerCooldown();

	if (CurrentHeat == 0)
	{
		bIsOverheated = false;
		GetWorldTimerManager().ClearTimer(this, &AShooterWeapon_Instant::Cooldown);
	}
}

bool AShooterWeapon_Instant::ServerCooldown_Validate()
{
	return true;
}

void AShooterWeapon_Instant::ServerCooldown_Implementation()
{
	//GEngine->AddOnScreenDebugMessage(-1, 1.0, FColor::Black, TEXT("ServerCooldown"));
	//GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("heat amount, server: %f"), CurrentHeat));

	if (CurrentHeat < 1)
	{
		CurrentHeat = 0;
		bIsOverheated = false;
		GetWorldTimerManager().ClearTimer(this, &AShooterWeapon_Instant::Cooldown);
	}
	else
	{
		CurrentHeat -= 10;
	}
}

bool AShooterWeapon_Instant::IncreaseHeat_Validate()
{
	return true;
}

void AShooterWeapon_Instant::IncreaseHeat_Implementation()
{
	//GEngine->AddOnScreenDebugMessage(-1, 1.0, FColor::Black, TEXT("Heat ++++++"));
	if (GetWorldTimerManager().IsTimerActive(this, &AShooterWeapon_Instant::Cooldown))
	{
		GetWorldTimerManager().ClearTimer(this, &AShooterWeapon_Instant::Cooldown);
	}
	CurrentHeat += HeatIncrease;

	if (CurrentHeat >= 100)
	{
		Overheat();
	}
	GetWorldTimerManager().SetTimer(this, &AShooterWeapon_Instant::Cooldown, CooldownRate, true);

}

void AShooterWeapon_Instant::Overheat()
{
	//GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Blue, TEXT("OVERHEATED!!"));
	if (Role < ROLE_Authority)
	{
		ServerOverheat();
	}

	bIsOverheated = true;
	// possibly: play overheat animations here
}

bool AShooterWeapon_Instant::ServerOverheat_Validate()
{
	return true;
}

void AShooterWeapon_Instant::ServerOverheat_Implementation()
{
	Overheat();
}

void AShooterWeapon_Instant::OnBurstStarted()
{
	if (CanFire())
	{
		if (bIsOverheat)
		{
			// start firing, can be delayed to satisfy TimeBetweenShots
			const float GameTime = GetWorld()->GetTimeSeconds();
			if (LastFireTime > 0 && WeaponConfig.TimeBetweenShots > 0.0f &&
				LastFireTime + WeaponConfig.TimeBetweenShots > GameTime)
			{
				GetWorldTimerManager().SetTimer(this, &AShooterWeapon_Instant::HandleFiring, LastFireTime + WeaponConfig.TimeBetweenShots - GameTime, false);
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
	
}

bool AShooterWeapon_Instant::CanFire() const
{
	bool bCanFire = MyPawn && MyPawn->CanFire();
	bool bStateOKToFire = ((CurrentState == EWeaponState::Idle) || (CurrentState == EWeaponState::Firing));

	if (bIsOverheated)
	{
		//GEngine->AddOnScreenDebugMessage(-1, 50.0f, FColor::Yellow, TEXT("I AM OVERHEATED SO WHY CAN I FIRE"));
	}
	else
	{
		//GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Yellow, TEXT("NOT OVERHEATED?"));
	}

	return ((bCanFire == true) && (bStateOKToFire == true) && (bPendingReload == false) && (bIsOverheated == false));
}

void AShooterWeapon_Instant::HandleFiring()
{
	if((CurrentAmmoInClip > 0 || HasInfiniteClip() || HasInfiniteAmmo()) && CanFire())
	{
		if (bIsOverheat)
		{
			IncreaseHeat();
		}
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

		AShooterPlayerController* MyPC = Cast<AShooterPlayerController>(MyPawn->Controller);
		if (MyPC && CurrentAmmo != 0 && (bShouldRecoil || bIsAutomatic == false))
		{
			if (CurrentZoomLevel > 0)
			{
				MyPC->AddPitchInput(RecoilPerShot.Pitch * ZoomRecoilMod);
				MyPC->AddYawInput(RecoilPerShot.Yaw * ZoomRecoilMod);
				MyPC->AddRollInput(RecoilPerShot.Roll * ZoomRecoilMod);
			}
			else
			{
				MyPC->AddPitchInput(RecoilPerShot.Pitch);
				MyPC->AddYawInput(RecoilPerShot.Yaw);
				MyPC->AddRollInput(RecoilPerShot.Roll);
			}
		}

		if (bShouldRecoil == false && bIsAutomatic == true)
		{
			if (GetWorldTimerManager().IsTimerActive(this, &AShooterWeapon::ActivateRecoil))
			{

			}
			else
			{
				GetWorldTimerManager().SetTimer(this, &AShooterWeapon::ActivateRecoil, TimeBeforeRecoil, false);
			}

		}

		// setup refire timer
		bRefiring = (CurrentState == EWeaponState::Firing && WeaponConfig.TimeBetweenShots > 0.0f && bIsAutomatic);
		if (bRefiring)
		{
			GetWorldTimerManager().SetTimer(this, &AShooterWeapon_Instant::HandleFiring, WeaponConfig.TimeBetweenShots, false);
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