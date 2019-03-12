// Fill out your copyright notice in the Description page of Project Settings.

#include "ShooterGame.h"
#include "ShooterGame/Classes/Weapons/ShooterThrownGrenade.h"

AShooterThrownGrenade::AShooterThrownGrenade(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	//CollisionComp = ObjectInitializer.CreateDefaultSubobject<USphereComponent>(this, TEXT("SphereComp"));
	GetCollisionComp()->InitSphereRadius(5.0f);
	GetCollisionComp()->AlwaysLoadOnClient = true;
	GetCollisionComp()->AlwaysLoadOnServer = true;
	GetCollisionComp()->bTraceComplexOnMove = true;
	GetCollisionComp()->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	GetCollisionComp()->SetCollisionObjectType(COLLISION_PROJECTILE);
	GetCollisionComp()->SetCollisionResponseToAllChannels(ECR_Ignore);
	GetCollisionComp()->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
	GetCollisionComp()->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block);
	GetCollisionComp()->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
	RootComponent = GetCollisionComp();


//	ParticleComp = ObjectInitializer.CreateDefaultSubobject<UParticleSystemComponent>(this, TEXT("ParticleComp"));
	GetParticleComp()->bAutoActivate = false;
	GetParticleComp()->bAutoDestroy = false;
	GetParticleComp()->AttachParent = RootComponent;

	//MovementComp = ObjectInitializer.CreateDefaultSubobject<UProjectileMovementComponent>(this, TEXT("ProjectileComp"));
	GetMovementComp()->UpdatedComponent = GetCollisionComp();
	GetMovementComp()->InitialSpeed = 2000.0f;
	GetMovementComp()->MaxSpeed = 2000.0f;
	GetMovementComp()->bRotationFollowsVelocity = true;
	GetMovementComp()->ProjectileGravityScale = 1.0;
	GetMovementComp()->bShouldBounce = true;
	GetMovementComp()->Bounciness = 0.5;
	//MovementComp->OnProjectileStop.AddDynamic(this, &AShooterProjectile::OnBounce);

	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickGroup = TG_PrePhysics;
	SetRemoteRoleForBackwardsCompat(ROLE_SimulatedProxy);
	bReplicates = true;
	bReplicateInstigator = true;
	bReplicateMovement = true;

	bHasBounced = false;
}

void AShooterThrownGrenade::InitVelocity(FVector& ShootDirection)
{
	if (GetMovementComp())
	{
		GetMovementComp()->Velocity = ShootDirection * GetMovementComp()->InitialSpeed;
	}
}

/*void AShooterThrownGrenade::OnBounce(const FHitResult& Impact)
{
MyImpact = Impact;
if (GetWorldTimerManager().IsTimerActive(this, &AShooterThrownGrenade::MyExplode))
{
return;
}
else
{
GetWorldTimerManager().SetTimer(this, &AShooterThrownGrenade::MyExplode, FuseTime, false);
}
}*/

/*void AShooterThrownGrenade::MyExplode()
{
if (Role == ROLE_Authority && !bExploded)
{
Explode(HitResult);
DisableAndDestroy();
}
}*/

void AShooterThrownGrenade::Explode(const FHitResult& Impact)
{
	if (GetParticleComp())
	{
		GetParticleComp()->Deactivate();
	}

	FVector MyLocation = GetActorLocation();
	FRotator MyRotation = GetActorRotation();

	if (GrenadeDamage > 0 && ExplosionRadius > 0 && GrenadeDamageType)
	{
		//UGameplayStatics::ApplyRadialDamageWithFalloff(this, GrenadeDamage, MinimumGrenadeDamage, MyLocation, MaxDamageRadius, ExplosionRadius, 1, GrenadeDamageType, TArray<AActor*>(), this, MyController.Get());
		UGameplayStatics::ApplyRadialDamage(this, GrenadeDamage, MyLocation, ExplosionRadius, GrenadeDamageType, TArray<AActor*>(), this, MyController.Get());
	}
	if (ExplosionTemplate)
	{
		AShooterExplosionEffect* EffectActor = GetWorld()->SpawnActorDeferred<AShooterExplosionEffect>(ExplosionTemplate, MyLocation, MyRotation);

		if (EffectActor)
		{
			UGameplayStatics::FinishSpawningActor(EffectActor, FTransform(MyRotation, MyLocation));
		}
	}

	bExploded = true;
}


