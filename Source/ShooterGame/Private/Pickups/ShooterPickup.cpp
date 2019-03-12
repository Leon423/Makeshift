// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "ShooterGame.h"
#include "Particles/ParticleSystemComponent.h"

AShooterPickup::AShooterPickup(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	UCapsuleComponent* CollisionComp = ObjectInitializer.CreateDefaultSubobject<UCapsuleComponent>(this, TEXT("CollisionComp"));
	CollisionComp->InitCapsuleSize(40.0f, 50.0f);
	CollisionComp->SetCollisionObjectType(COLLISION_PICKUP);
	CollisionComp->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	CollisionComp->SetCollisionResponseToAllChannels(ECR_Ignore);
	CollisionComp->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	RootComponent = CollisionComp;

	PickupPSC = ObjectInitializer.CreateDefaultSubobject<UParticleSystemComponent>(this, TEXT("PickupFX"));
	PickupPSC->bAutoActivate = false;
	PickupPSC->bAutoDestroy = false;
	PickupPSC->AttachParent = RootComponent;

	RespawnTime = 10.0f;
	bIsActive = false;
	PickedUpBy = NULL;

	SetRemoteRoleForBackwardsCompat(ROLE_SimulatedProxy);
	bReplicates = true;
}

void AShooterPickup::BeginPlay()
{
	Super::BeginPlay();

	RespawnPickup();

	// register on pickup list (server only), don't care about unregistering (in FinishDestroy) - no streaming
	AShooterGameMode* GameMode = GetWorld()->GetAuthGameMode<AShooterGameMode>();
	if (GameMode)
	{
		GameMode->LevelPickups.Add(this);
	}
}

void AShooterPickup::ReceiveActorBeginOverlap(class AActor* Other)
{
	Super::ReceiveActorBeginOverlap(Other);
	PickupOnTouch(Cast<AShooterCharacter>(Other));
}

bool AShooterPickup::CanBePickedUp(class AShooterCharacter* TestPawn) const
{
	return TestPawn && TestPawn->IsAlive();
}

void AShooterPickup::GivePickupTo(class AShooterCharacter* Pawn)
{
}

void AShooterPickup::PickupOnTouch(class AShooterCharacter* Pawn)
{
	if (bIsActive && Pawn && Pawn->IsAlive() && !IsPendingKill())
	{
		if (CanBePickedUp(Pawn))
		{
			GivePickupTo(Pawn);
			PickedUpBy = Pawn;

			if (!IsPendingKill())
			{
				bIsActive = false;
				OnPickedUp();

				if (RespawnTime > 0.0f)
				{
					GetWorldTimerManager().SetTimer(this, &AShooterPickup::RespawnPickup, RespawnTime, false);
				}
			}
		}
	}
}

void AShooterPickup::RespawnPickup()
{
	bIsActive = true;
	PickedUpBy = NULL;
	OnRespawned();

	TArray<AActor*> OverlappingPawns;
	GetOverlappingActors(OverlappingPawns, AShooterCharacter::StaticClass());

	for (int32 i = 0; i < OverlappingPawns.Num(); i++)
	{
		PickupOnTouch(Cast<AShooterCharacter>(OverlappingPawns[i]));	
	}
}

void AShooterPickup::OnPickedUp()
{
	if (RespawningFX)
	{
		PickupPSC->SetTemplate(RespawningFX);
		PickupPSC->ActivateSystem();
	}
	else
	{
		PickupPSC->DeactivateSystem();
	}

	if (PickupSound && PickedUpBy)
	{
		UGameplayStatics::PlaySoundAttached(PickupSound, PickedUpBy->GetRootComponent());
	}

	OnPickedUpEvent();
}

void AShooterPickup::OnRespawned()
{
	if (ActiveFX)
	{
		PickupPSC->SetTemplate(ActiveFX);
		PickupPSC->ActivateSystem();
	}
	else
	{
		PickupPSC->DeactivateSystem();
	}

	const bool bJustSpawned = CreationTime <= (GetWorld()->GetTimeSeconds() + 5.0f);
	if (RespawnSound && !bJustSpawned)
	{
		UGameplayStatics::PlaySoundAtLocation(this, RespawnSound, GetActorLocation());
	}

	OnRespawnEvent();
}

void AShooterPickup::OnRep_IsActive()
{
	if (bIsActive)
	{
		OnRespawned();
	}
	else
	{
		OnPickedUp();
	}
}

void AShooterPickup::GetLifetimeReplicatedProps( TArray< FLifetimeProperty > & OutLifetimeProps ) const
{
	Super::GetLifetimeReplicatedProps( OutLifetimeProps );

	DOREPLIFETIME( AShooterPickup, bIsActive );
	DOREPLIFETIME( AShooterPickup, PickedUpBy );
}