// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "ShooterGame.h"
#include "Particles/ParticleSystemComponent.h"

AShooterProjectile::AShooterProjectile(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	CollisionComp = ObjectInitializer.CreateDefaultSubobject<USphereComponent>(this, TEXT("SphereComp"));
	CollisionComp->InitSphereRadius(5.0f);
	CollisionComp->AlwaysLoadOnClient = true;
	CollisionComp->AlwaysLoadOnServer = true;
	CollisionComp->bTraceComplexOnMove = true;
	CollisionComp->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	CollisionComp->SetCollisionObjectType(COLLISION_PROJECTILE);
	CollisionComp->SetCollisionResponseToAllChannels(ECR_Ignore);
	CollisionComp->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
	CollisionComp->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block);
	CollisionComp->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
	RootComponent = CollisionComp;

	ParticleComp = ObjectInitializer.CreateDefaultSubobject<UParticleSystemComponent>(this, TEXT("ParticleComp"));
	ParticleComp->bAutoActivate = false;
	ParticleComp->bAutoDestroy = false;
	ParticleComp->AttachParent = RootComponent;

	MovementComp = ObjectInitializer.CreateDefaultSubobject<UProjectileMovementComponent>(this, TEXT("ProjectileComp"));
	MovementComp->UpdatedComponent = CollisionComp;
	MovementComp->InitialSpeed = 2000.0f;
	MovementComp->MaxSpeed = 2000.0f;
	MovementComp->bRotationFollowsVelocity = true;
	MovementComp->ProjectileGravityScale = 0.f;

	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickGroup = TG_PrePhysics;
	SetRemoteRoleForBackwardsCompat(ROLE_SimulatedProxy);
	bReplicates = true;
	bReplicateInstigator = true;
	bReplicateMovement = true;

	bHasBounced = false;
}

void AShooterProjectile::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	if (MovementComp->bShouldBounce)
	{
		MovementComp->OnProjectileBounce.AddDynamic(this, &AShooterProjectile::OnBounce);
	}
	else
	{
		MovementComp->OnProjectileStop.AddDynamic(this, &AShooterProjectile::OnImpact);
	}

	CollisionComp->MoveIgnoreActors.Add(Instigator);

	AShooterWeapon_Projectile* OwnerWeapon = Cast<AShooterWeapon_Projectile>(GetOwner());
	if (OwnerWeapon)
	{
		OwnerWeapon->ApplyWeaponConfig(WeaponConfig);
	}

	SetLifeSpan(WeaponConfig.ProjectileLife);
	MyController = GetInstigatorController();

	if (bUseSafety)
	{
		GetWorldTimerManager().SetTimer(this, &AShooterProjectile::ExpireSafety, SafetyTimer, false);
	}

}

void AShooterProjectile::InitVelocity(FVector& ShootDirection)
{
	if (MovementComp)
	{
		MovementComp->Velocity = ShootDirection * MovementComp->InitialSpeed;
	}
}

void AShooterProjectile::OnImpact(const FHitResult& HitResult)
{
	if (bOnlyExplodeFromWeaponCall)
	{
		return;
	}

	if (Role == ROLE_Authority && !bExploded)
	{
		Explode(HitResult);
		DisableAndDestroy();
	}
}

void AShooterProjectile::Explode(const FHitResult& Impact)
{
	/*if (ParticleComp)
	{
		ParticleComp->Deactivate();
	}

	// effects and damage origin shouldn't be placed inside mesh at impact point
	const FVector NudgedImpactLocation = Impact.ImpactPoint + Impact.ImpactNormal * 10.0f;

	if (WeaponConfig.ExplosionDamage > 0 && WeaponConfig.ExplosionRadius > 0 && WeaponConfig.DamageType)
	{
		UGameplayStatics::ApplyRadialDamage(this, WeaponConfig.ExplosionDamage, NudgedImpactLocation, WeaponConfig.ExplosionRadius, WeaponConfig.DamageType, TArray<AActor*>(), this, MyController.Get());
	}

	if (ExplosionTemplate)
	{
		const FRotator SpawnRotation = Impact.ImpactNormal.Rotation();

		AShooterExplosionEffect* EffectActor = GetWorld()->SpawnActorDeferred<AShooterExplosionEffect>(ExplosionTemplate, NudgedImpactLocation, SpawnRotation);
		if (EffectActor)
		{
			EffectActor->SurfaceHit = Impact;
			UGameplayStatics::FinishSpawningActor(EffectActor, FTransform(SpawnRotation, NudgedImpactLocation));
		}
	}

	bExploded = true;

	*/

	if (ParticleComp)
	{
		ParticleComp->Deactivate();
	}


	FVector MyLocation = GetActorLocation();
	FRotator MyRotation = GetActorRotation();

	TArray<AActor*> IgnoreList;

	FString BoneString;

	BoneString = Impact.BoneName.ToString();

	//GEngine->AddOnScreenDebugMessage(-1, 5.0, FColor::Magenta, BoneString);
	if (Impact.BoneName == "b_head" || Impact.BoneName == "Head")
	{
		GEngine->AddOnScreenDebugMessage(-1, 1.0, FColor::Yellow, TEXT("PROJECTILE HEADSHOT"));
		FPointDamageEvent PDMG;
		PDMG.DamageTypeClass = DamageType;
		PDMG.HitInfo = Impact;
		PDMG.Damage = Damage;
		PDMG.Damage = Cast<AShooterCharacter>(Impact.GetActor())->CalculateDamageToUse(PDMG.Damage, PDMG, MyController.Get(), this, HeadshotDamage, ShieldDamage);
		Impact.GetActor()->TakeDamage(PDMG.Damage, PDMG, MyController.Get(), this);
		IgnoreList.Add(Impact.GetActor());
		if (bHasPlasmaStun)
		{
			if (Cast<AShooterCharacter>(Impact.GetActor()))
			{
				Cast<AShooterCharacter>(Impact.GetActor())->ApplyPlasmaStun(StunAmount, StunTime);
			}
		}
	}
	// Might need to add an else statement here, damage might be being applied twice to the headshotted player.

	else
	{
		if (Damage > 0 && DamageType)
		{
			if (Cast<AShooterCharacter>(Impact.GetActor()) && ExplosionRadius <= 0)
			{
				FPointDamageEvent PDMG;
				PDMG.DamageTypeClass = DamageType;
				PDMG.HitInfo = Impact;
				PDMG.Damage = Damage;
				PDMG.Damage = Cast<AShooterCharacter>(Impact.GetActor())->CalculateDamageToUse(PDMG.Damage, PDMG, MyController.Get(), this, 0.0, ShieldDamage);
				Impact.GetActor()->TakeDamage(PDMG.Damage, PDMG, MyController.Get(), this);
			}

			else if (bUseMaxDamageRadius && ExplosionRadius > 0)
			{
			//	UGameplayStatics::ApplyRadialDamageWithFalloff(this, Damage, MinimumGrenadeDamage, MyLocation, MaxDamageRadius, ExplosionRadius, 1, DamageType, IgnoreList, this, MyController.Get());
				UMakeshiftGameplayStatics::ApplyRadialDamageWithFalloffWithShieldDamage(this, Damage, MinimumGrenadeDamage, MyLocation, MaxDamageRadius, ExplosionRadius, 1, DamageType, IgnoreList, this, MyController.Get(), ECollisionChannel::ECC_Camera , ShieldDamage, 0.f);
			}
			else if (ExplosionRadius > 0)
			{
			//	UGameplayStatics::ApplyRadialDamage(this, Damage, MyLocation, ExplosionRadius, DamageType, TArray<AActor*>(), this, MyController.Get());
				UMakeshiftGameplayStatics::ApplyRadialDamageWithShieldDamage(this, Damage, MyLocation, ExplosionRadius, DamageType, TArray<AActor*>(), this, MyController.Get(),false, ECollisionChannel::ECC_Camera, ShieldDamage);
			}
			else
			{

			}

			if (bHasPlasmaStun)
			{

				if (Cast<AShooterCharacter>(Impact.GetActor()))
				{
					Cast<AShooterCharacter>(Impact.GetActor())->ApplyPlasmaStun(StunAmount, StunTime);
				}
			}
		}
	}
	if (ExplosionTemplate)
	{

		AShooterExplosionEffect* EffectActor = GetWorld()->SpawnActorDeferred<AShooterExplosionEffect>(ExplosionTemplate, MyLocation, MyRotation, this, Instigator, false);

		if (EffectActor)
		{
			UGameplayStatics::FinishSpawningActor(EffectActor, FTransform(MyRotation, MyLocation));
			FVector VecToLog = EffectActor->GetActorLocation();
		}
	}

	bExploded = true;
}

void AShooterProjectile::DisableAndDestroy()
{
	UAudioComponent* ProjAudioComp = FindComponentByClass<UAudioComponent>();
	if (ProjAudioComp && ProjAudioComp->IsPlaying())
	{
		ProjAudioComp->FadeOut(0.1f, 0.f);
	}

	MovementComp->StopMovementImmediately();

	// give clients some time to show explosion
	SetLifeSpan( 2.0f );
}

void AShooterProjectile::OnRep_Exploded()
{
	FVector ProjDirection = GetActorRotation().Vector();

	const FVector StartTrace = GetActorLocation() - ProjDirection * 200;
	const FVector EndTrace = GetActorLocation() + ProjDirection * 150;
	FHitResult Impact;
	
	if (!GetWorld()->LineTraceSingle(Impact, StartTrace, EndTrace, COLLISION_PROJECTILE, FCollisionQueryParams(TEXT("ProjClient"), true, Instigator)))
	{
		// failsafe
		Impact.ImpactPoint = GetActorLocation();
		Impact.ImpactNormal = -ProjDirection;
	}

	Explode(Impact);
}

void AShooterProjectile::PostNetReceiveVelocity(const FVector& NewVelocity)
{
	if (MovementComp)
	{
		MovementComp->Velocity = NewVelocity;
	}
}

void AShooterProjectile::GetLifetimeReplicatedProps( TArray< FLifetimeProperty > & OutLifetimeProps ) const
{
	Super::GetLifetimeReplicatedProps( OutLifetimeProps );
	
	DOREPLIFETIME( AShooterProjectile, bExploded );
	DOREPLIFETIME(AShooterProjectile, bSafetyExpired);
	DOREPLIFETIME(AShooterProjectile, bWantsToExplode);
}

///////////// ADDED BELOW THIS:::::

void AShooterProjectile::MyExplode()
{
	if (Role == ROLE_Authority && !bExploded)
	{
		if (bUseSafety)
		{
			if (bSafetyExpired)
			{
				Explode(MyImpact);
				DisableAndDestroy();
			}
			else
			{
				bWantsToExplode = true;
			}
		}
		else
		{
			Explode(MyImpact);
			DisableAndDestroy();
		}
	}
}

void AShooterProjectile::OnBounce(const FHitResult& Impact, const FVector& Vec)
{
	if (bOnlyExplodeFromWeaponCall)
	{
		return;
	}

	MyImpact = Impact;

	if (GEngine)
	{
		//GEngine->AddOnScreenDebugMessage(-1, 1.0, FColor::Red, TEXT("OnBounce"));
	}

	if (bHasBounced == true)
	{
		return;
	}
	else if (bArmOnBounce)
	{
		GetWorldTimerManager().SetTimer(this, &AShooterProjectile::MyExplode, FuseTime, false);
		bHasBounced = true;
	}

	if (bArmOnBounce == false && bHasBounced == false)
	{
		FVector MyVec = GetVelocity();

		//GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("Velocity.Size: %f"), MyVec.Size()));

		if (MyVec.Size() <= ArmVelocity)
		{
			bHasBounced = true;
			GetWorldTimerManager().SetTimer(this, &AShooterProjectile::MyExplode, FuseTime, false);
		}
	}
}

void AShooterProjectile::LaunchProj(FVector LaunchVelocity)
{
	if (Role == ROLE_Authority && MovementComp)
	{
		FVector FinalVel = LaunchVelocity;

		MovementComp->Velocity = LaunchVelocity;
	}
}

void AShooterProjectile::ExpireSafety()
{
	bSafetyExpired = true;
	if (bWantsToExplode == true)
	{
		MyExplode();
	}
}