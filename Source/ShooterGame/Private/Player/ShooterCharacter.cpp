// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "ShooterGame.h"

AShooterCharacter::AShooterCharacter(const FObjectInitializer& ObjectInitializer) 
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UShooterCharacterMovement>(ACharacter::CharacterMovementComponentName))
{
	//I ADDED THIS: Create Camera Component
	FirstPersonCameraComponent = ObjectInitializer.CreateDefaultSubobject<UCameraComponent>(this, TEXT("FirstPersonCamera"));
	FirstPersonCameraComponent->AttachParent = GetCapsuleComponent();
	// position camera a bit above eyes
	//FirstPersonCameraComponent->RelativeLocation = FVector(0, 0, 50.0f + BaseEyeHeight);

	Mesh1P = ObjectInitializer.CreateDefaultSubobject<USkeletalMeshComponent>(this, TEXT("PawnMesh1P"));
	Mesh1P->AttachParent = FirstPersonCameraComponent; // this used to be attached to CapsuleComponent. changing it to firstpersoncameracomponent should fix crouch issue
	Mesh1P->bOnlyOwnerSee = true;
	Mesh1P->bOwnerNoSee = false;
	Mesh1P->bCastDynamicShadow = false;
	Mesh1P->bReceivesDecals = false;
	Mesh1P->MeshComponentUpdateFlag = EMeshComponentUpdateFlag::OnlyTickPoseWhenRendered;
	Mesh1P->PrimaryComponentTick.TickGroup = TG_PrePhysics;
	Mesh1P->bChartDistanceFactor = false;
	Mesh1P->SetCollisionObjectType(ECC_Pawn);
	Mesh1P->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Mesh1P->SetCollisionResponseToAllChannels(ECR_Ignore);

	GetMesh()->bOnlyOwnerSee = false;
	GetMesh()->bOwnerNoSee = true;
	GetMesh()->bReceivesDecals = false;
	GetMesh()->SetCollisionObjectType(ECC_Pawn);
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	GetMesh()->SetCollisionResponseToChannel(COLLISION_WEAPON, ECR_Block);
	GetMesh()->SetCollisionResponseToChannel(COLLISION_PROJECTILE, ECR_Block);
	GetMesh()->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);

	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	GetCapsuleComponent()->SetCollisionResponseToChannel(COLLISION_PROJECTILE, ECR_Block);
	GetCapsuleComponent()->SetCollisionResponseToChannel(COLLISION_WEAPON, ECR_Ignore);

	TargetingSpeedModifier = 0.5f;
	bIsTargeting = false;
	RunningSpeedModifier = 1.5f;
	bWantsToRun = false;
	bWantsToFire = false;
	bWantsToMeleeNew = false;
	LowHealthPercentage = 0.5f;

	BaseTurnRate = 45.f;
	BaseLookUpRate = 45.f;

	// I ADDED THIS:
	ShieldBreakParticleComp = ObjectInitializer.CreateDefaultSubobject<UParticleSystemComponent>(this, TEXT("ShieldBreakParticleComp"));
	ShieldBreakParticleComp->bAutoActivate = false;
	ShieldBreakParticleComp->bAutoDestroy = false;
	ShieldBreakParticleComp->AttachParent = GetMesh();

	CachedMovementSpeedOnSpawn = GetCharacterMovement()->MaxWalkSpeed;
}

void AShooterCharacter::PostInitializeComponents()
{

	Super::PostInitializeComponents();

	if (Role == ROLE_Authority)
	{
		Health = GetMaxHealth();
		SpawnDefaultInventory();
	}

	// set initial mesh visibility (3rd person view)
	UpdatePawnMeshes();
	
	// create material instance for setting team colors (3rd person view)
	for (int32 iMat = 0; iMat < GetMesh()->GetNumMaterials(); iMat++)
	{
		MeshMIDs.Add(GetMesh()->CreateAndSetMaterialInstanceDynamic(iMat));
	}

	// play respawn effects
	if (GetNetMode() != NM_DedicatedServer)
	{
		if (RespawnFX)
		{
			UGameplayStatics::SpawnEmitterAtLocation(this, RespawnFX, GetActorLocation(), GetActorRotation());
		}

		if (RespawnSound)
		{
			UGameplayStatics::PlaySoundAtLocation(this, RespawnSound, GetActorLocation());
		}
	}
}

void AShooterCharacter::Destroyed()
{
	Super::Destroyed();
	DestroyInventory();
}

void AShooterCharacter::PawnClientRestart()
{
	Super::PawnClientRestart();

	// switch mesh to 1st person view
	UpdatePawnMeshes();

	// reattach weapon if needed
	SetCurrentWeapon(CurrentWeapon);

	// set team colors for 1st person view
	UMaterialInstanceDynamic* Mesh1PMID = Mesh1P->CreateAndSetMaterialInstanceDynamic(0);
	UpdateTeamColors(Mesh1PMID);
}

void AShooterCharacter::PossessedBy(class AController* InController)
{
	Super::PossessedBy(InController);

	// [server] as soon as PlayerState is assigned, set team colors of this pawn for local player
	UpdateTeamColorsAllMIDs();
}

void AShooterCharacter::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();

	// [client] as soon as PlayerState is assigned, set team colors of this pawn for local player
	if (PlayerState != NULL)
	{
		UpdateTeamColorsAllMIDs();
	}
}

FRotator AShooterCharacter::GetAimOffsets() const
{
	const FVector AimDirWS = GetBaseAimRotation().Vector();
	const FVector AimDirLS = ActorToWorld().InverseTransformVectorNoScale(AimDirWS);
	const FRotator AimRotLS = AimDirLS.Rotation();

	return AimRotLS;
}

bool AShooterCharacter::IsEnemyFor(AController* TestPC) const
{
	if (TestPC == Controller || TestPC == NULL)
	{
		return false;
	}

	AShooterPlayerState* TestPlayerState = Cast<AShooterPlayerState>(TestPC->PlayerState);
	AShooterPlayerState* MyPlayerState = Cast<AShooterPlayerState>(PlayerState);

	bool bIsEnemy = true;
	if (GetWorld()->GameState && GetWorld()->GameState->GameModeClass)
	{
		const AShooterGameMode* DefGame = GetWorld()->GameState->GameModeClass->GetDefaultObject<AShooterGameMode>();
		if (DefGame && MyPlayerState && TestPlayerState)
		{
			bIsEnemy = DefGame->CanDealDamage(TestPlayerState, MyPlayerState);
		}
	}

	return bIsEnemy;
}


//////////////////////////////////////////////////////////////////////////
// Meshes

void AShooterCharacter::UpdatePawnMeshes()
{
	bool const bFirstPerson = IsFirstPerson();

	Mesh1P->MeshComponentUpdateFlag = !bFirstPerson ? EMeshComponentUpdateFlag::OnlyTickPoseWhenRendered : EMeshComponentUpdateFlag::AlwaysTickPoseAndRefreshBones;
	Mesh1P->SetOwnerNoSee(!bFirstPerson);

	GetMesh()->MeshComponentUpdateFlag = bFirstPerson ? EMeshComponentUpdateFlag::OnlyTickPoseWhenRendered : EMeshComponentUpdateFlag::AlwaysTickPoseAndRefreshBones;
	GetMesh()->SetOwnerNoSee(bFirstPerson);
}

void AShooterCharacter::UpdateTeamColors(UMaterialInstanceDynamic* UseMID)
{
	if (UseMID)
	{
		AShooterPlayerState* MyPlayerState = Cast<AShooterPlayerState>(PlayerState);
		if (MyPlayerState != NULL)
		{
			float MaterialParam = (float)MyPlayerState->GetTeamNum();
			UseMID->SetScalarParameterValue(TEXT("Team Color Index"), MaterialParam);
		}
	}
}

// CHANGED STUFF IN THIS
void AShooterCharacter::OnCameraUpdate(const FVector& CameraLocation, const FRotator& CameraRotation)
{
	USkeletalMeshComponent* DefMesh1P = Cast<USkeletalMeshComponent>(GetClass()->GetDefaultSubobjectByName(TEXT("PawnMesh1P")));
	const FMatrix DefMeshLS = FRotationTranslationMatrix(DefMesh1P->RelativeRotation, DefMesh1P->RelativeLocation);
	const FMatrix LocalToWorld = ActorToWorld().ToMatrixWithScale();

	// Mesh rotating code expect uniform scale in LocalToWorld matrix

	const FRotator RotCameraPitch(CameraRotation.Pitch, 0.0f, 0.0f);
	const FRotator RotCameraYaw(0.0f, CameraRotation.Yaw, 0.0f);

	const FMatrix LeveledCameraLS = FRotationTranslationMatrix(RotCameraYaw, CameraLocation) * LocalToWorld.Inverse();
	const FMatrix PitchedCameraLS = FRotationMatrix(RotCameraPitch) * LeveledCameraLS;
	const FMatrix MeshRelativeToCamera = DefMeshLS * LeveledCameraLS.Inverse();
	const FMatrix PitchedMesh = MeshRelativeToCamera * PitchedCameraLS;

	//Mesh1P->SetRelativeLocationAndRotation(PitchedMesh.GetOrigin(), PitchedMesh.Rotator());
}


//////////////////////////////////////////////////////////////////////////
// Damage & death


void AShooterCharacter::FellOutOfWorld(const class UDamageType& dmgType)
{
	Die(Health, FDamageEvent(dmgType.GetClass()), NULL, NULL);
}

void AShooterCharacter::Suicide()
{
	KilledBy(this);
}

void AShooterCharacter::KilledBy(APawn* EventInstigator)
{
	if (Role == ROLE_Authority && !bIsDying)
	{
		AController* Killer = NULL;
		if (EventInstigator != NULL)
		{
			Killer = EventInstigator->Controller;
			LastHitBy = NULL;
		}

		Die(Health, FDamageEvent(UDamageType::StaticClass()), Killer, NULL);
	}
}

// CHANGED STUFF IN THIS
float AShooterCharacter::TakeDamage(float Damage, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, class AActor* DamageCauser)
{
	AShooterPlayerController* MyPC = Cast<AShooterPlayerController>(Controller);

	if (MyPC && MyPC->HasGodMode())
	{
		return 0.f;
	}

	if (Health <= 0.f)
	{
		return 0.f;
	}

	if (bOvershieldCharging)
	{
		return 0.f;
	}

	// Modify based on game rules.
	AShooterGameMode* const Game = GetWorld()->GetAuthGameMode<AShooterGameMode>();
	Damage = Game ? Game->ModifyDamage(Damage, this, DamageEvent, EventInstigator, DamageCauser) : 0.f;

	const float ActualDamage = Super::TakeDamage(Damage, DamageEvent, EventInstigator, DamageCauser);
	float MyDamage = ActualDamage;
	if (MyDamage > 0.f)
	{
		if (CurrentWeapon->CurrentZoomLevel != 0)
		{
			//EndZoom();
		}
		//EndZoom();
		if (CurrentShieldAmount > 0 && bHasShields)
		{
			if (MyDamage > CurrentShieldAmount)
			{
				//GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("MyDamage > ShieldAmount, HeadshotDamage: %f"), Damage));

				MyDamage -= CurrentShieldAmount;
				CurrentShieldAmount = 0;

				if (MyDamage > 0.f)
				{
					GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow, TEXT("TOOK HealthDamage 1"));
					Health -= MyDamage;
				}

				// shields would have broken, play particle effect
				if (ShieldBreakParticleComp)
				{
					ShieldBreakParticleComp->Activate(true);
				}
			}
			else
			{
				CurrentShieldAmount -= MyDamage;
			}
		}
		else
		{
			GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow, TEXT("TOOK HealthDamage 2"));
			Health -= MyDamage;
		}
		if (Health <= 0)
		{
			PlayerDiedDropWeapon();
			Die(MyDamage, DamageEvent, EventInstigator, DamageCauser);
		}
		else
		{
			PlayHit(MyDamage, DamageEvent, EventInstigator ? EventInstigator->GetPawn() : NULL, DamageCauser);
		}

		MakeNoise(1.0f, EventInstigator ? EventInstigator->GetPawn() : this);

		if (bHasShields)
		{
			if (GetWorldTimerManager().IsTimerActive(this, &AShooterCharacter::CalculateShieldChargeTime))
			{
				GetWorldTimerManager().ClearTimer(this, &AShooterCharacter::CalculateShieldChargeTime);
			}

			if (GetWorldTimerManager().IsTimerActive(this, &AShooterCharacter::RechargeShield))
			{
				GetWorldTimerManager().ClearTimer(this, &AShooterCharacter::RechargeShield);
			}
			GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow, TEXT("TOOK DAMAGE"));
			GetWorldTimerManager().SetTimer(this, &AShooterCharacter::CalculateShieldChargeTime, 3.0f, false);
		}
	}

	return MyDamage;
}


bool AShooterCharacter::CanDie(float KillingDamage, FDamageEvent const& DamageEvent, AController* Killer, AActor* DamageCauser) const
{
	if ( bIsDying										// already dying
		|| IsPendingKill()								// already destroyed
		|| Role != ROLE_Authority						// not authority
		|| GetWorld()->GetAuthGameMode() == NULL
		|| GetWorld()->GetAuthGameMode()->GetMatchState() == MatchState::LeavingMap)	// level transition occurring
	{
		return false;
	}

	return true;
}


bool AShooterCharacter::Die(float KillingDamage, FDamageEvent const& DamageEvent, AController* Killer, AActor* DamageCauser)
{
	if (!CanDie(KillingDamage, DamageEvent, Killer, DamageCauser))
	{
		return false;
	}

	Health = FMath::Min(0.0f, Health);

	// if this is an environmental death then refer to the previous killer so that they receive credit (knocked into lava pits, etc)
	UDamageType const* const DamageType = DamageEvent.DamageTypeClass ? DamageEvent.DamageTypeClass->GetDefaultObject<UDamageType>() : GetDefault<UDamageType>();
	Killer = GetDamageInstigator(Killer, *DamageType);

	AController* const KilledPlayer = (Controller != NULL) ? Controller : Cast<AController>(GetOwner());
	GetWorld()->GetAuthGameMode<AShooterGameMode>()->Killed(Killer, KilledPlayer, this, DamageType);

	NetUpdateFrequency = GetDefault<AShooterCharacter>()->NetUpdateFrequency;
	GetCharacterMovement()->ForceReplicationUpdate();

	OnDeath(KillingDamage, DamageEvent, Killer ? Killer->GetPawn() : NULL, DamageCauser);
	return true;
}


void AShooterCharacter::OnDeath(float KillingDamage, struct FDamageEvent const& DamageEvent, class APawn* PawnInstigator, class AActor* DamageCauser)
{
	if (bIsDying)
	{
		return;
	}

	bReplicateMovement = false;
	bTearOff = true;
	bIsDying = true;

	if (Role == ROLE_Authority)
	{
		ReplicateHit(KillingDamage, DamageEvent, PawnInstigator, DamageCauser, true);	

		// play the force feedback effect on the client player controller
		APlayerController* PC = Cast<APlayerController>(Controller);
		if (PC && DamageEvent.DamageTypeClass)
		{
			UShooterDamageType *DamageType = Cast<UShooterDamageType>(DamageEvent.DamageTypeClass->GetDefaultObject());
			if (DamageType && DamageType->KilledForceFeedback)
			{
				PC->ClientPlayForceFeedback(DamageType->KilledForceFeedback, false, "Damage");
			}
		}
	}

	// cannot use IsLocallyControlled here, because even local client's controller may be NULL here
	if (GetNetMode() != NM_DedicatedServer && DeathSound && Mesh1P && Mesh1P->IsVisible())
	{
		UGameplayStatics::PlaySoundAtLocation(this, DeathSound, GetActorLocation());
	}

	// remove all weapons
	DestroyInventory();
	
	// switch back to 3rd person view
	UpdatePawnMeshes();

	DetachFromControllerPendingDestroy();
	StopAllAnimMontages();

	if (LowHealthWarningPlayer && LowHealthWarningPlayer->IsPlaying())
	{
		LowHealthWarningPlayer->Stop();
	}

	if (RunLoopAC)
	{
		RunLoopAC->Stop();
	}

	// disable collisions on capsule
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GetCapsuleComponent()->SetCollisionResponseToAllChannels(ECR_Ignore);

	if (GetMesh())
	{
		static FName CollisionProfileName(TEXT("Ragdoll"));
		GetMesh()->SetCollisionProfileName(CollisionProfileName);
	}
	SetActorEnableCollision(true);

	// Death anim
	float DeathAnimDuration = PlayAnimMontage(DeathAnim);

	// Ragdoll
	if (DeathAnimDuration > 0.f)
	{
		GetWorldTimerManager().SetTimer(this, &AShooterCharacter::SetRagdollPhysics, FMath::Min(0.1f, DeathAnimDuration), false);
	}
	else
	{
		SetRagdollPhysics();
	}
}

// CHANGED STUFF IN THIS
void AShooterCharacter::PlayHit(float DamageTaken, struct FDamageEvent const& DamageEvent, class APawn* PawnInstigator, class AActor* DamageCauser)
{
	if (Role == ROLE_Authority)
	{
		ReplicateHit(DamageTaken, DamageEvent, PawnInstigator, DamageCauser, false);

		// play the force feedback effect on the client player controller
		APlayerController* PC = Cast<APlayerController>(Controller);
		if (PC && DamageEvent.DamageTypeClass)
		{
			UShooterDamageType *DamageType = Cast<UShooterDamageType>(DamageEvent.DamageTypeClass->GetDefaultObject());
			if (DamageType && DamageType->HitForceFeedback)
			{
				PC->ClientPlayForceFeedback(DamageType->HitForceFeedback, false, "Damage");
			}
		}
	}

	if (DamageTaken > 0.f)
	{
		ApplyDamageMomentum(DamageTaken, DamageEvent, PawnInstigator, DamageCauser);
	}

	AShooterPlayerController* MyPC = Cast<AShooterPlayerController>(Controller);
	AShooterHUD* MyHUD = MyPC ? Cast<AShooterHUD>(MyPC->GetHUD()) : NULL;
	if (MyHUD)
	{
		MyHUD->NotifyHit(DamageTaken, DamageEvent, PawnInstigator);
	}

	if (PawnInstigator && PawnInstigator != this && PawnInstigator->IsLocallyControlled())
	{
		AShooterPlayerController* InstigatorPC = Cast<AShooterPlayerController>(PawnInstigator->Controller);
		AShooterHUD* InstigatorHUD = InstigatorPC ? Cast<AShooterHUD>(InstigatorPC->GetHUD()) : NULL;
		if (InstigatorHUD)
		{
			InstigatorHUD->NotifyEnemyHit();
		}
	}

	EndZoom();
}


void AShooterCharacter::SetRagdollPhysics()
{
	bool bInRagdoll = false;

	if (IsPendingKill())
	{
		bInRagdoll = false;
	}
	else if (!GetMesh() || !GetMesh()->GetPhysicsAsset())
	{
		bInRagdoll = false;
	}
	else
	{
		// initialize physics/etc
		GetMesh()->SetAllBodiesSimulatePhysics(true);
		GetMesh()->SetSimulatePhysics(true);
		GetMesh()->WakeAllRigidBodies();
		GetMesh()->bBlendPhysics = true;

		bInRagdoll = true;
	}

	GetCharacterMovement()->StopMovementImmediately();
	GetCharacterMovement()->DisableMovement();
	GetCharacterMovement()->SetComponentTickEnabled(false);

	if (!bInRagdoll)
	{
		// hide and set short lifespan
		TurnOff();
		SetActorHiddenInGame(true);
		SetLifeSpan( 1.0f );
	}
	else
	{
		SetLifeSpan( 10.0f );
	}
}



void AShooterCharacter::ReplicateHit(float Damage, struct FDamageEvent const& DamageEvent, class APawn* PawnInstigator, class AActor* DamageCauser, bool bKilled)
{
	const float TimeoutTime = GetWorld()->GetTimeSeconds() + 0.5f;

	FDamageEvent const& LastDamageEvent = LastTakeHitInfo.GetDamageEvent();
	if ((PawnInstigator == LastTakeHitInfo.PawnInstigator.Get()) && (LastDamageEvent.DamageTypeClass == LastTakeHitInfo.DamageTypeClass) && (LastTakeHitTimeTimeout == TimeoutTime))
	{
		// same frame damage
		if (bKilled && LastTakeHitInfo.bKilled)
		{
			// Redundant death take hit, just ignore it
			return;
		}

		// otherwise, accumulate damage done this frame
		Damage += LastTakeHitInfo.ActualDamage;
	}

	LastTakeHitInfo.ActualDamage = Damage;
	LastTakeHitInfo.PawnInstigator = Cast<AShooterCharacter>(PawnInstigator);
	LastTakeHitInfo.DamageCauser = DamageCauser;
	LastTakeHitInfo.SetDamageEvent(DamageEvent);		
	LastTakeHitInfo.bKilled = bKilled;
	LastTakeHitInfo.EnsureReplication();

	LastTakeHitTimeTimeout = TimeoutTime;
}

void AShooterCharacter::OnRep_LastTakeHitInfo()
{
	if (LastTakeHitInfo.bKilled)
	{
		OnDeath(LastTakeHitInfo.ActualDamage, LastTakeHitInfo.GetDamageEvent(), LastTakeHitInfo.PawnInstigator.Get(), LastTakeHitInfo.DamageCauser.Get());
	}
	else
	{
		PlayHit(LastTakeHitInfo.ActualDamage, LastTakeHitInfo.GetDamageEvent(), LastTakeHitInfo.PawnInstigator.Get(), LastTakeHitInfo.DamageCauser.Get());
	}
}

//Pawn::PlayDying sets this lifespan, but when that function is called on client, dead pawn's role is still SimulatedProxy despite bTearOff being true. 
void AShooterCharacter::TornOff()
{
	SetLifeSpan(25.f);
}


//////////////////////////////////////////////////////////////////////////
// Inventory

void AShooterCharacter::SpawnDefaultInventory()
{
	if (Role < ROLE_Authority)
	{
		return;
	}

	int32 NumWeaponClasses = DefaultInventoryClasses.Num();	
	for (int32 i = 0; i < NumWeaponClasses; i++)
	{
		if (DefaultInventoryClasses[i])
		{
			FActorSpawnParameters SpawnInfo;
			SpawnInfo.bNoCollisionFail = true;
			AShooterWeapon* NewWeapon = GetWorld()->SpawnActor<AShooterWeapon>(DefaultInventoryClasses[i], SpawnInfo);
			NewWeapon->SetActorRotation(FRotator(0, 0, -90));
			AddWeapon(NewWeapon);
		}
	}

	// equip first weapon in inventory
	if (Inventory.Num() > 0)
	{
		EquipWeapon(Inventory[0]);
	}
}

void AShooterCharacter::DestroyInventory()
{
	GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Green, TEXT("DESTROY INVENTORY, THIS SHOULD NOT BE SHOW BEFORE THE SPAWN STUFF"));
	if (Role < ROLE_Authority)
	{
		return;
	}

	// remove all weapons from inventory and destroy them
	for (int32 i = Inventory.Num() - 1; i >= 0; i--)
	{
		AShooterWeapon* Weapon = Inventory[i];
		if (Weapon)
		{
			RemoveWeapon(Weapon);
			Weapon->Destroy();
		}
	}
}

void AShooterCharacter::AddWeapon(AShooterWeapon* Weapon)
{
	if (Weapon && Role == ROLE_Authority)
	{
		Weapon->OnEnterInventory(this);
		Inventory.AddUnique(Weapon);
	}
}

void AShooterCharacter::RemoveWeapon(AShooterWeapon* Weapon)
{
	if (Weapon && Role == ROLE_Authority)
	{
		Weapon->OnLeaveInventory();
		Inventory.RemoveSingle(Weapon);
	}
}

AShooterWeapon* AShooterCharacter::FindWeapon(TSubclassOf<AShooterWeapon> WeaponClass)
{
	for (int32 i = 0; i < Inventory.Num(); i++)
	{
		if (Inventory[i] && Inventory[i]->IsA(WeaponClass))
		{
			return Inventory[i];
		}
	}

	return NULL;
}

void AShooterCharacter::EquipWeapon(AShooterWeapon* Weapon)
{
	if (Weapon)
	{
		if (Role == ROLE_Authority)
		{
			SetCurrentWeapon(Weapon);
		}
		else
		{
			ServerEquipWeapon(Weapon);
		}
	}
}

bool AShooterCharacter::ServerEquipWeapon_Validate(AShooterWeapon* Weapon)
{
	return true;
}

void AShooterCharacter::ServerEquipWeapon_Implementation(AShooterWeapon* Weapon)
{
	EquipWeapon(Weapon);
}

void AShooterCharacter::OnRep_CurrentWeapon(AShooterWeapon* LastWeapon)
{
	SetCurrentWeapon(CurrentWeapon, LastWeapon);
}

void AShooterCharacter::SetCurrentWeapon(class AShooterWeapon* NewWeapon, class AShooterWeapon* LastWeapon)
{
	AShooterWeapon* LocalLastWeapon = NULL;
	
	if (LastWeapon != NULL)
	{
		LocalLastWeapon = LastWeapon;
	}
	else if (NewWeapon != CurrentWeapon)
	{
		LocalLastWeapon = CurrentWeapon;
	}

	// unequip previous
	if (LocalLastWeapon)
	{
		LocalLastWeapon->OnUnEquip();
	}

	CurrentWeapon = NewWeapon;

	// equip new one
	if (NewWeapon)
	{
		NewWeapon->SetOwningPawn(this);	// Make sure weapon's MyPawn is pointing back to us. During replication, we can't guarantee APawn::CurrentWeapon will rep after AWeapon::MyPawn!
		NewWeapon->OnEquip();
	}
}


//////////////////////////////////////////////////////////////////////////
// Weapon usage

void AShooterCharacter::StartWeaponFire()
{
	if (!bWantsToFire)
	{
		bWantsToFire = true;
		if (CurrentWeapon)
		{
			CurrentWeapon->StartFire();
		}
	}
}

void AShooterCharacter::StopWeaponFire()
{
	if (bWantsToFire)
	{
		bWantsToFire = false;
		if (CurrentWeapon)
		{
			CurrentWeapon->StopFire();
		}
	}
}

bool AShooterCharacter::CanFire() const
{
	return IsAlive();
}

bool AShooterCharacter::CanReload() const
{
	return true;
}

void AShooterCharacter::SetTargeting(bool bNewTargeting)
{
	bIsTargeting = bNewTargeting;

	if (TargetingSound)
	{
		UGameplayStatics::PlaySoundAttached(TargetingSound, GetRootComponent());
	}

	if (Role < ROLE_Authority)
	{
		ServerSetTargeting(bNewTargeting);
	}
}

bool AShooterCharacter::ServerSetTargeting_Validate(bool bNewTargeting)
{
	return true;
}

void AShooterCharacter::ServerSetTargeting_Implementation(bool bNewTargeting)
{
	SetTargeting(bNewTargeting);
}

//////////////////////////////////////////////////////////////////////////
// Movement

void AShooterCharacter::SetRunning(bool bNewRunning, bool bToggle)
{
	bWantsToRun = bNewRunning;
	bWantsToRunToggled = bNewRunning && bToggle;

	if (Role < ROLE_Authority)
	{
		ServerSetRunning(bNewRunning, bToggle);
	}

	UpdateRunSounds(bNewRunning);
}

bool AShooterCharacter::ServerSetRunning_Validate(bool bNewRunning, bool bToggle)
{
	return true;
}

void AShooterCharacter::ServerSetRunning_Implementation(bool bNewRunning, bool bToggle)
{
	SetRunning(bNewRunning, bToggle);
}

void AShooterCharacter::UpdateRunSounds(bool bNewRunning)
{
	if (bNewRunning)
	{
		if (!RunLoopAC && RunLoopSound)
		{
			RunLoopAC = UGameplayStatics::PlaySoundAttached(RunLoopSound, GetRootComponent());
			if (RunLoopAC)
			{
				RunLoopAC->bAutoDestroy = false;
			}
			
		}
		else if (RunLoopAC)
		{
			RunLoopAC->Play();
		}
	}
	else
	{
		if (RunLoopAC)
		{
			RunLoopAC->Stop();
		}

		if (RunStopSound)
		{
			UGameplayStatics::PlaySoundAttached(RunStopSound, GetRootComponent());
		}
	}
}

//////////////////////////////////////////////////////////////////////////
// Animations

float AShooterCharacter::PlayAnimMontage(class UAnimMontage* AnimMontage, float InPlayRate, FName StartSectionName) 
{
	USkeletalMeshComponent* UseMesh = GetPawnMesh();
	if (AnimMontage && UseMesh && UseMesh->AnimScriptInstance)
	{
		return UseMesh->AnimScriptInstance->Montage_Play(AnimMontage, InPlayRate);
	}

	return 0.0f;
}

void AShooterCharacter::StopAnimMontage(class UAnimMontage* AnimMontage)
{
	USkeletalMeshComponent* UseMesh = GetPawnMesh();
	if (AnimMontage && UseMesh && UseMesh->AnimScriptInstance &&
		UseMesh->AnimScriptInstance->Montage_IsPlaying(AnimMontage))
	{
		UseMesh->AnimScriptInstance->Montage_Stop(AnimMontage->BlendOutTime);
	}
}

void AShooterCharacter::StopAllAnimMontages()
{
	USkeletalMeshComponent* UseMesh = GetPawnMesh();
	if (UseMesh && UseMesh->AnimScriptInstance)
	{
		UseMesh->AnimScriptInstance->Montage_Stop(0.0f);
	}
}


//////////////////////////////////////////////////////////////////////////
// Input


// CHANGED STUFF IN THIS
void AShooterCharacter::SetupPlayerInputComponent(class UInputComponent* InputComponent)
{
	check(InputComponent);
	InputComponent->BindAxis("MoveForward", this, &AShooterCharacter::MoveForward);
	InputComponent->BindAxis("MoveRight", this, &AShooterCharacter::MoveRight);
	InputComponent->BindAxis("MoveUp", this, &AShooterCharacter::MoveUp);
	InputComponent->BindAxis("Turn", this, &APawn::AddControllerYawInput);
	InputComponent->BindAxis("TurnRate", this, &AShooterCharacter::TurnAtRate);
	InputComponent->BindAxis("LookUp", this, &APawn::AddControllerPitchInput);
	InputComponent->BindAxis("LookUpRate", this, &AShooterCharacter::LookUpAtRate);

	InputComponent->BindAction("Fire", IE_Pressed, this, &AShooterCharacter::OnStartFire);
	InputComponent->BindAction("Fire", IE_Released, this, &AShooterCharacter::OnStopFire);

	InputComponent->BindAction("Target", IE_Pressed, this, &AShooterCharacter::OnStartTargeting);
	InputComponent->BindAction("Target", IE_Released, this, &AShooterCharacter::OnStopTargeting);

	InputComponent->BindAction("NextWeapon", IE_Pressed, this, &AShooterCharacter::OnNextWeapon);
	InputComponent->BindAction("PrevWeapon", IE_Pressed, this, &AShooterCharacter::OnPrevWeapon);

	InputComponent->BindAction("Reload", IE_Pressed, this, &AShooterCharacter::OnReload);

	InputComponent->BindAction("Jump", IE_Pressed, this, &AShooterCharacter::OnStartJump);
	InputComponent->BindAction("Jump", IE_Released, this, &AShooterCharacter::OnStopJump);

	InputComponent->BindAction("Run", IE_Pressed, this, &AShooterCharacter::OnStartRunning);
	InputComponent->BindAction("RunToggle", IE_Pressed, this, &AShooterCharacter::OnStartRunningToggle);
	InputComponent->BindAction("Run", IE_Released, this, &AShooterCharacter::OnStopRunning);

	InputComponent->BindAction("Crouch", IE_Pressed, this, &AShooterCharacter::MyStartCrouch);
	InputComponent->BindAction("Crouch", IE_Released, this, &AShooterCharacter::MyStopCrouch);

	InputComponent->BindAction("ThrowGrenade", IE_Pressed, this, &AShooterCharacter::StartGrenadeThrow);

	InputComponent->BindAction("Use", IE_Pressed, this, &AShooterCharacter::UseSomething);
	InputComponent->BindAction("Use", IE_Released, this, &AShooterCharacter::UnUseSomething);

	InputComponent->BindAction("MeleeNew", IE_Pressed, this, &AShooterCharacter::OnStartMeleeNew);

	InputComponent->BindAction("ThrowGrenadeNew", IE_Pressed, this, &AShooterCharacter::OnStartGrenadeNew);
}


void AShooterCharacter::MoveForward(float Val)
{
	if (Controller && Val != 0.f)
	{
		// Limit pitch when walking or falling
		const bool bLimitRotation = (GetCharacterMovement()->IsMovingOnGround() || GetCharacterMovement()->IsFalling());
		const FRotator Rotation = bLimitRotation ? GetActorRotation() : Controller->GetControlRotation();
		const FVector Direction = FRotationMatrix(Rotation).GetScaledAxis( EAxis::X );
		AddMovementInput(Direction, Val);
	}
}

void AShooterCharacter::MoveRight(float Val)
{
	if (Val != 0.f)
	{
		const FRotator Rotation = GetActorRotation();
		const FVector Direction = FRotationMatrix(Rotation).GetScaledAxis( EAxis::Y );
		AddMovementInput(Direction, Val);
	}
}

void AShooterCharacter::MoveUp(float Val)
{
	if (Val != 0.f)
	{
		// Not when walking or falling.
		if (GetCharacterMovement()->IsMovingOnGround() || GetCharacterMovement()->IsFalling())
		{
			return;
		}

		AddMovementInput(FVector::UpVector, Val);
	}
}

void AShooterCharacter::TurnAtRate(float Val)
{
	// calculate delta for this frame from the rate information
	AddControllerYawInput(Val * BaseTurnRate * GetWorld()->GetDeltaSeconds());
}

void AShooterCharacter::LookUpAtRate(float Val)
{
	// calculate delta for this frame from the rate information
	AddControllerPitchInput(Val * BaseLookUpRate * GetWorld()->GetDeltaSeconds());
}

void AShooterCharacter::OnStartFire()
{
	AShooterPlayerController* MyPC = Cast<AShooterPlayerController>(Controller);
	if (MyPC && MyPC->IsGameInputAllowed())
	{
		if (IsRunning())
		{
			SetRunning(false, false);
		}
		StartWeaponFire();
	}
}

void AShooterCharacter::OnStopFire()
{
	StopWeaponFire();
}

// CHANGED STUFF IN THIS
void AShooterCharacter::OnStartTargeting()
{
	GEngine->AddOnScreenDebugMessage(-1, 1.0f, FColor::Yellow, TEXT("ONSTARTTARGETTING"));
	AShooterPlayerController* MyPC = Cast<AShooterPlayerController>(Controller);
	if (MyPC && MyPC->IsGameInputAllowed())
	{
		if (bTargetingAllowed)
		{


			if (IsRunning())
			{
				SetRunning(false, false);
			}
			SetTargeting(true);
		}
		else
		{
			if (CurrentWeapon->bZoomingAllowed)
			{
				StartZoom();
			}
		}
	}
}

void AShooterCharacter::OnStopTargeting()
{
	SetTargeting(false);
}

// CHANGED STUFF IN THIS
void AShooterCharacter::OnNextWeapon()
{
	AShooterPlayerController* MyPC = Cast<AShooterPlayerController>(Controller);
	if (MyPC && MyPC->IsGameInputAllowed())
	{
		EndZoom();
		if (Inventory.Num() >= 2 && (CurrentWeapon == NULL || CurrentWeapon->GetCurrentState() != EWeaponState::Equipping))
		{
			UpdatePreviousWeapon();
			const int32 CurrentWeaponIdx = Inventory.IndexOfByKey(CurrentWeapon);
			AShooterWeapon* NextWeapon = Inventory[(CurrentWeaponIdx + 1) % Inventory.Num()];
			EquipWeapon(NextWeapon);
		}
	}
}

// CHANGED STUFF IN THIS
void AShooterCharacter::OnPrevWeapon()
{
	AShooterPlayerController* MyPC = Cast<AShooterPlayerController>(Controller);
	if (MyPC && MyPC->IsGameInputAllowed())
	{
		EndZoom();
		if (Inventory.Num() >= 2 && (CurrentWeapon == NULL || CurrentWeapon->GetCurrentState() != EWeaponState::Equipping))
		{
			UpdatePreviousWeapon();
			const int32 CurrentWeaponIdx = Inventory.IndexOfByKey(CurrentWeapon);
			AShooterWeapon* PrevWeapon = Inventory[(CurrentWeaponIdx - 1 + Inventory.Num()) % Inventory.Num()];
			EquipWeapon(PrevWeapon);
		}
	}
}

// CHANGED STUFF IN THIS
void AShooterCharacter::OnReload()
{
	AShooterPlayerController* MyPC = Cast<AShooterPlayerController>(Controller);
	if (MyPC && MyPC->IsGameInputAllowed())
	{
		EndZoom();
		if (CurrentWeapon)
		{
			CurrentWeapon->StartReload();
		}
	}
}

// CHANGED STUFF IN THIS
void AShooterCharacter::OnStartRunning()
{
	AShooterPlayerController* MyPC = Cast<AShooterPlayerController>(Controller);
	if (MyPC && MyPC->IsGameInputAllowed() && bRunningAllowed)
	{
		if (IsTargeting())
		{
			SetTargeting(false);
		}
		StopWeaponFire();
		SetRunning(true, false);
	}
}

// CHANGED STUFF IN THIS
void AShooterCharacter::OnStartRunningToggle()
{
	AShooterPlayerController* MyPC = Cast<AShooterPlayerController>(Controller);
	if (MyPC && MyPC->IsGameInputAllowed() && bRunningAllowed)
	{
		EndZoom();
		if (IsTargeting())
		{
			SetTargeting(false);
		}
		StopWeaponFire();
		SetRunning(true, true);
	}
}

void AShooterCharacter::OnStopRunning()
{
	SetRunning(false, false);
}

bool AShooterCharacter::IsRunning() const
{	
	if (!GetCharacterMovement())
	{
		return false;
	}
	
	return (bWantsToRun || bWantsToRunToggled) && !GetVelocity().IsZero() && (GetVelocity().SafeNormal2D() | GetActorRotation().Vector()) > -0.1;
}

void AShooterCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (bWantsToRunToggled && !IsRunning())
	{
		SetRunning(false, false);
	}
	AShooterPlayerController* MyPC = Cast<AShooterPlayerController>(Controller);
	if (MyPC && MyPC->HasHealthRegen())
	{
		if (this->Health < this->GetMaxHealth())
		{
			this->Health +=  5 * DeltaSeconds;
			if (Health > this->GetMaxHealth())
			{
				Health = this->GetMaxHealth();
			}
		}
	}
	
	if (LowHealthSound && GEngine->UseSound())
	{
		if ((this->Health > 0 && this->Health < this->GetMaxHealth() * LowHealthPercentage) && (!LowHealthWarningPlayer || !LowHealthWarningPlayer->IsPlaying()))
		{
			LowHealthWarningPlayer = UGameplayStatics::PlaySoundAttached(LowHealthSound, GetRootComponent(),
				NAME_None, FVector(ForceInit), EAttachLocation::KeepRelativeOffset, true);
			LowHealthWarningPlayer->SetVolumeMultiplier(0.0f);
		} 
		else if ((this->Health > this->GetMaxHealth() * LowHealthPercentage || this->Health < 0) && LowHealthWarningPlayer && LowHealthWarningPlayer->IsPlaying())
		{
			LowHealthWarningPlayer->Stop();
		}
		if (LowHealthWarningPlayer && LowHealthWarningPlayer->IsPlaying())
		{
			const float MinVolume = 0.3f;
			const float VolumeMultiplier = (1.0f - (this->Health / (this->GetMaxHealth() * LowHealthPercentage)));
			LowHealthWarningPlayer->SetVolumeMultiplier(MinVolume + (1.0f - MinVolume) * VolumeMultiplier);
		}
	}
}

void AShooterCharacter::OnStartJump()
{
	AShooterPlayerController* MyPC = Cast<AShooterPlayerController>(Controller);
	if (MyPC && MyPC->IsGameInputAllowed())
	{
		bPressedJump = true;
	}
}

void AShooterCharacter::OnStopJump()
{
	bPressedJump = false;
}

//////////////////////////////////////////////////////////////////////////
// Replication

void AShooterCharacter::PreReplication( IRepChangedPropertyTracker & ChangedPropertyTracker )
{
	Super::PreReplication( ChangedPropertyTracker );

	// Only replicate this property for a short duration after it changes so join in progress players don't get spammed with fx when joining late
	DOREPLIFETIME_ACTIVE_OVERRIDE( AShooterCharacter, LastTakeHitInfo, GetWorld() && GetWorld()->GetTimeSeconds() < LastTakeHitTimeTimeout );
}

// CHANGED STUFF IN THIS
void AShooterCharacter::GetLifetimeReplicatedProps( TArray< FLifetimeProperty > & OutLifetimeProps ) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// only to local owner: weapon change requests are locally instigated, other clients don't need it
	DOREPLIFETIME_CONDITION(AShooterCharacter, Inventory, COND_OwnerOnly);

	// everyone except local owner: flag change is locally instigated
	DOREPLIFETIME_CONDITION(AShooterCharacter, bIsTargeting, COND_SkipOwner);
	DOREPLIFETIME_CONDITION(AShooterCharacter, bWantsToRun, COND_SkipOwner);

	DOREPLIFETIME_CONDITION(AShooterCharacter, LastTakeHitInfo, COND_Custom);

	// everyone
	DOREPLIFETIME(AShooterCharacter, CurrentWeapon);
	DOREPLIFETIME(AShooterCharacter, Health);
	DOREPLIFETIME(AShooterCharacter, CurrentShieldAmount);
	//DOREPLIFETIME(AShooterCharacter, PreviousWeapon);
}

AShooterWeapon* AShooterCharacter::GetWeapon() const
{
	return CurrentWeapon;
}

int32 AShooterCharacter::GetInventoryCount() const
{
	return Inventory.Num();
}

AShooterWeapon* AShooterCharacter::GetInventoryWeapon(int32 index) const
{
	return Inventory[index];
}

USkeletalMeshComponent* AShooterCharacter::GetPawnMesh() const
{
	return IsFirstPerson() ? Mesh1P : GetMesh();
}

USkeletalMeshComponent* AShooterCharacter::GetSpecifcPawnMesh( bool WantFirstPerson ) const
{
	return WantFirstPerson == true  ? Mesh1P : GetMesh();
}

FName AShooterCharacter::GetWeaponAttachPoint() const
{
	return WeaponAttachPoint;
}

float AShooterCharacter::GetTargetingSpeedModifier() const
{
	return TargetingSpeedModifier;
}

bool AShooterCharacter::IsTargeting() const
{
	return bIsTargeting;
}

float AShooterCharacter::GetRunningSpeedModifier() const
{
	return RunningSpeedModifier;
}

bool AShooterCharacter::IsFiring() const
{
	return bWantsToFire;
};

bool AShooterCharacter::IsFirstPerson() const
{
	return IsAlive() && Controller && Controller->IsLocalPlayerController();
}

int32 AShooterCharacter::GetMaxHealth() const
{
	return GetClass()->GetDefaultObject<AShooterCharacter>()->Health;
}

bool AShooterCharacter::IsAlive() const
{
	return Health > 0;
}

float AShooterCharacter::GetLowHealthPercentage() const
{
	return LowHealthPercentage;
}

void AShooterCharacter::UpdateTeamColorsAllMIDs()
{
	for (int32 i = 0; i < MeshMIDs.Num(); ++i)
	{
		UpdateTeamColors(MeshMIDs[i]);
	}
}

///////////////// ADDED EVERYTHING BELOW:::::::: some functions above have been changed.

void AShooterCharacter::TakeHeadshotDamage(float Damage, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, class AActor* DamageCauser, float HeadshotDamage)
{
	if (HeadshotDamage != 0.f)
	{
		GEngine->AddOnScreenDebugMessage(-1, 10.0f, FColor::Red, TEXT("headshot damage != 0.f"));


		// a headshot took place
		if (bHasShields && CurrentShieldAmount > 0.0)
		{
			if (Damage > CurrentShieldAmount)
			{
				// the normal damage would cause health damage, so the headshot damage should be applied
				TakeDamage(HeadshotDamage, DamageEvent, EventInstigator, DamageCauser);
			}
			else
			{
				// the normal damage would not cause health damage, so headshot is not applied
				TakeDamage(Damage, DamageEvent, EventInstigator, DamageCauser);
			}
		}
		else
		{
			TakeDamage(HeadshotDamage, DamageEvent, EventInstigator, DamageCauser);
		}
	}
	else
	{
		// a headshot didnt take place, so just send the damage
		TakeDamage(Damage, DamageEvent, EventInstigator, DamageCauser);
	}
}

float AShooterCharacter::CalculateDamageToUse(float Damage, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, class AActor* DamageCauser, float HeadshotDamage, float ShieldDamage)
{
	if (DamageEvent.IsOfType(FRadialDamageEvent::ClassID))
	{
		FRadialDamageEvent* const RadialDamageEvent = (FRadialDamageEvent*)&DamageEvent;

		// radial damage caused this so we need to calculate the damage to use seperately
		float  ActualDamageShield;

		ActualDamageShield = InternalTakeRadialDamage(ShieldDamage, *RadialDamageEvent , EventInstigator, DamageCauser);

		if (bHasShields && CurrentShieldAmount > 0.0)
		{
			if (ActualDamageShield > CurrentShieldAmount)
			{
				// the damage to shields is more than the shields left so we do the health damage

				//return ActualDamageHealth;

				float ActualHealthDamage = InternalTakeRadialDamage(Damage, *RadialDamageEvent, EventInstigator, DamageCauser);

				GEngine->AddOnScreenDebugMessage(-1, 10.0f, FColor::Green, TEXT("ACTUALDAMAGESHILED > CURRENTSHIELDAMOUNT"));
				// actualhealthdamage - actualdamageshield + currentshieldamount
				//return ActualHealthDamage + CurrentShieldAmount;

				if (ActualHealthDamage > ActualDamageShield)
				{
					// just do the health damages amount as it will bleed through naturally
					return ActualHealthDamage;
				}
				else
				{
					return CurrentShieldAmount;
				}
			}
			else
			{
				// damage is less than shields left so we do shield damage
				//return ActualDamageShield;
				GEngine->AddOnScreenDebugMessage(-1, 10.0f, FColor::Green, TEXT("ACTUALDAMAGESHILED < CURRENTSHIELDAMOUNT"));
				//return ShieldDamage;

				return ActualDamageShield;
			}
		}
		else
		{
			float ActualHealthDamage;

			ActualHealthDamage = InternalTakeRadialDamage(Damage, *RadialDamageEvent, EventInstigator, DamageCauser);

			GEngine->AddOnScreenDebugMessage(-1, 10.0f, FColor::Green, TEXT("HAS NO SHIELDS"));

			return ActualHealthDamage;
		}
		
	}

	if (HeadshotDamage != 0.f)
	{
		// A headshot happened

		if (bHasShields && CurrentShieldAmount > 0.0)
		{
			// shields are left, need further calculation
			if (ShieldDamage > CurrentShieldAmount)
			{
				// The damage to shields is more than the shields left so a headshot should be applied
				//TakeDamage(HeadshotDamage, DamageEvent, EventInstigator, DamageCauser);

				return HeadshotDamage;
			}
			else
			{
				// Shields wouldnt break so do shield damage
				//TakeDamage(ShieldDamage, DamageEvent, EventInstigator, DamageCauser);

				return ShieldDamage;
			}
		}
		else
		{
			// no shields so just do headshot damage
			//TakeDamage(HeadshotDamage, DamageEvent, EventInstigator, DamageCauser);

			return HeadshotDamage;
		}
	}
	else
	{
		// a headshot did not happen
		if (bHasShields && CurrentShieldAmount > 0.0)
		{
			// has shields do calculations
			if (ShieldDamage > CurrentShieldAmount)
			{
				//TakeDamage(Damage, DamageEvent, EventInstigator, DamageCauser);
				return Damage;
			}
			else
			{
				//TakeDamage(ShieldDamage, DamageEvent, EventInstigator, DamageCauser);
				return ShieldDamage;
			}
		}
		else
		{
			// no shields, do base damage
			//TakeDamage(Damage, DamageEvent, EventInstigator, DamageCauser);
			return Damage;
		}
	}
}

////////////////// SHIELDS

void AShooterCharacter::CalculateShieldChargeTime()
{
	ShieldRecoverRate = TotalRechargeTime / MaxShieldAmount;
	RechargeShield();
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow, TEXT("CALCULATED SHIELDS"));
	}
}

void AShooterCharacter::RechargeShield()
{
	if (bHasOvershield || bOvershieldCharging)
	{
		return;
	}

	if (CurrentShieldAmount < MaxShieldAmount)
	{
		CurrentShieldAmount += 1;

		if (CurrentShieldAmount < MaxShieldAmount)
		{
			GetWorldTimerManager().SetTimer(this, &AShooterCharacter::RechargeShield, ShieldRecoverRate, false);
		}
		else
		{
			if (CurrentShieldAmount > MaxShieldAmount)
			{
				CurrentShieldAmount = MaxShieldAmount;
			}
		}
	}
	else
	{
		CurrentShieldAmount = MaxShieldAmount;
	}
}

void AShooterCharacter::MyAddWeapon(AShooterWeapon* Weapon)
{
	AddWeapon(Weapon);
}

void AShooterCharacter::UseSomething()
{
	bCanUse = true;
	SetCanUse(true);
}

void AShooterCharacter::UnUseSomething()
{
	bCanUse = false;
	SetCanUse(false);
}

bool AShooterCharacter::SetCanUse_Validate(bool b)
{
	return true;
}

void AShooterCharacter::SetCanUse_Implementation(bool b)
{
	bCanUse = b;
}

bool AShooterCharacter::TryPickUp_Validate(class AShooter_Pickup* P, int32 C)
{
	return true;
}

void AShooterCharacter::TryPickUp_Implementation(class AShooter_Pickup* P, int32 C)
{
	GEngine->AddOnScreenDebugMessage(-1, 10.0f, FColor::Yellow, TEXT("Try Pickup Implementation!"));
	if (P->Weapon_C)
	{

		FVector const& Loc = FVector(0, 0, 0);
		FRotator const& Rot = FRotator(0, 0, -90);
		AShooterWeapon* Weap = Cast<AShooterWeapon>(GetWorld()->SpawnActor(P->Weapon_C, &Loc, &Rot));
		Weap->PickedUp(C);
		MyAddWeapon(Weap);
		OnNextWeapon();
		//P->Destroy();
	}
}

bool AShooterCharacter::TryPowerup_Validate(class AShooterPowerup* P, float Power)
{
	return true;
}

void AShooterCharacter::TryPowerup_Implementation(class AShooterPowerup * P, float Power)
{
	if (P)
	{
		OSPower = Power;
		CalculateOvershield();
	}
}

void AShooterCharacter::MyRemoveWeapon(class AShooterWeapon* Weapon)
{
	RemoveWeapon(Weapon);
}


bool AShooterCharacter::EvaluatePickup_Validate(class AShooter_Pickup* P)
{
	return true;
}

void AShooterCharacter::EvaluatePickup_Implementation(class AShooter_Pickup* P)
{
	GEngine->AddOnScreenDebugMessage(-1, 10.0f, FColor::Red, TEXT("EVALUATE PICKUP MOTHERFUCKER!"));

	if (P)
	{
		GEngine->AddOnScreenDebugMessage(-1, 10.0f, FColor::Red, TEXT("P IS TOTALLY VALID"));
		if (P->WeaponClassification == CurrentWeapon->WeaponClassification)
		{
			GEngine->AddOnScreenDebugMessage(-1, 10.0f, FColor::Red, TEXT("SAME AS CURRENT WEAPON!"));
			// same type of weapon, should be adding ammo
			if (CurrentWeapon->GetCurrentAmmo() < CurrentWeapon->GetMaxAmmo())
			{
				// ammo not full, so add ammo and destroy pickup if necessary
				int32 NewAmmoAmount;
				NewAmmoAmount = P->CurrentNumberOfBullets - (CurrentWeapon->GetMaxAmmo() - CurrentWeapon->GetCurrentAmmo());
				CurrentWeapon->AddAmmoFromPickup(P->CurrentNumberOfBullets);
				if (NewAmmoAmount <= 0)
				{
					// no ammo left, destroy pickup
					P->Destroy();
				}
				else
				{
					// ammo left, so set Pickups ammo to new amount
					P->CurrentNumberOfBullets = NewAmmoAmount;
				}

			}
		}
		else if (PreviousWeapon && P->WeaponClassification == PreviousWeapon->WeaponClassification)
		{
				GEngine->AddOnScreenDebugMessage(-1, 10.0f, FColor::Red, TEXT("SAME AS PREVIOUS WEAPON!"));
				// not same as current weapon, but same as previous weapon. should be adding ammo to previous weapon
				if (PreviousWeapon->GetCurrentAmmo() < PreviousWeapon->GetMaxAmmo())
				{
					// ammo not full so add ammo and destroy pickup if necessary
					int32 NewAmmoAmount;
					NewAmmoAmount = P->CurrentNumberOfBullets - (PreviousWeapon->GetMaxAmmo() - PreviousWeapon->GetCurrentAmmo());
					PreviousWeapon->AddAmmoFromPickup(P->CurrentNumberOfBullets);
					if (NewAmmoAmount <= 0)
					{
						// no ammo left, destroy pickup
						P->Destroy();
					}
					else
					{
						// ammo left, so set Pickups ammo to new amount
						P->CurrentNumberOfBullets = NewAmmoAmount;
					}
				}
		}
		else
		{

			GEngine->AddOnScreenDebugMessage(-1, 10.0f, FColor::Red, TEXT("PICKUP THE DAMN GUN!"));
			// not the same type as either weapon, should drop current weapon and add weapon from pickup
			DropCurrentWeapon2();



			if (P->Weapon_C)
			{

				FVector const& Loc = FVector(0, 0, 0);
				FRotator const& Rot = FRotator(0, 0, -90);
				AShooterWeapon* Weap = Cast<AShooterWeapon>(GetWorld()->SpawnActor(P->Weapon_C, &Loc, &Rot));
				Weap->PickedUp(P->CurrentNumberOfBullets);
				MyAddWeapon(Weap);
				EquipWeapon(Weap);
				//P->Destroy();
			}

			P->Destroy();
		}
	}
}

bool AShooterCharacter::DropCurrentWeapon2_Validate()
{
	return true;
}

void AShooterCharacter::DropCurrentWeapon2_Implementation()
{
	if (CurrentWeapon->PickupBP)
	{
		GEngine->AddOnScreenDebugMessage(-1, 10.0f, FColor::Red, TEXT("HAS A PICKUPBP!"));
		if (CurrentWeapon->GetCurrentAmmo() > 0 && CurrentWeapon->PickupBP)
		{
			// has ammo so drop pickup
			FVector const& Loc = GetActorLocation();
			FRotator const& Rot = GetActorRotation();
			AShooter_Pickup* Pickup = Cast<AShooter_Pickup>(GetWorld()->SpawnActor(CurrentWeapon->PickupBP, &Loc, &Rot));
			if (Pickup)
			{
				Pickup->CurrentNumberOfBullets = CurrentWeapon->GetCurrentAmmo();
			}
			RemoveWeapon(CurrentWeapon);
		}
		else
		{
			RemoveWeapon(CurrentWeapon);
		}
	}
}

bool AShooterCharacter::EvaluateAddingAmmoFromPickup_Validate(class AShooter_Pickup* P)
{
	return true;
}

void AShooterCharacter::EvaluateAddingAmmoFromPickup_Implementation(class AShooter_Pickup* P)
{
	if (P)
	{

		if (CurrentWeapon && CurrentWeapon->WeaponClassification)
		{


			if (P->WeaponClassification == CurrentWeapon->WeaponClassification)
			{
				// same weapon type add ammo

				int32 NewAmountOfBullets = P->CurrentNumberOfBullets - (CurrentWeapon->GetMaxAmmo() - CurrentWeapon->GetCurrentAmmo());
				CurrentWeapon->AddAmmoFromPickup(P->CurrentNumberOfBullets);

				if (NewAmountOfBullets > 0)
				{
					P->CurrentNumberOfBullets = NewAmountOfBullets;
				}
				else
				{
					P->Destroy();
				}

			}
			else if (PreviousWeapon && PreviousWeapon->WeaponClassification)
			{
				if (P->WeaponClassification == PreviousWeapon->WeaponClassification)
				{
					// same as previous weapon, so add ammo
					int32 NewAmountOfBullets = P->CurrentNumberOfBullets - (PreviousWeapon->GetMaxAmmo() - PreviousWeapon->GetCurrentAmmo());
					PreviousWeapon->AddAmmoFromPickup(P->CurrentNumberOfBullets);

					if (NewAmountOfBullets > 0)
					{
						P->CurrentNumberOfBullets = NewAmountOfBullets;
					}
					else
					{
						P->Destroy();
					}
				}
			}
		}
		else if (PreviousWeapon && PreviousWeapon->WeaponClassification)
		{
			if (P->WeaponClassification == PreviousWeapon->WeaponClassification)
			{
				// same as previous weapon, so add ammo
				int32 NewAmountOfBullets = P->CurrentNumberOfBullets - (PreviousWeapon->GetMaxAmmo() - PreviousWeapon->GetCurrentAmmo());
				PreviousWeapon->AddAmmoFromPickup(P->CurrentNumberOfBullets);

				if (NewAmountOfBullets > 0)
				{
					P->CurrentNumberOfBullets = NewAmountOfBullets;
				}
				else
				{
					P->Destroy();
				}
			}
		}
	}
}

void AShooterCharacter::StartGrenadeThrow()
{
	/*if (Role < ROLE_Authority)
	{
	ServerGrenadeThrow();
	}*/

	GrenadeThrow();
	EndZoom();
}

void AShooterCharacter::GrenadeThrow()
{
	if (bThrowingGrenade || Grenades <= 0)
	{
		return;
	}

	// Don't allow grenade throwing
	bThrowingGrenade = true;

	if (bThrowingGrenade)
	{
		if (Grenades > 0)
		{
			CurrentWeapon->StartGrenadeAnim();
			GetWorldTimerManager().SetTimer(this, &AShooterCharacter::PerformGrenadeThrow, SpawnGrenTime, false);

			if (Grenades >= 0)
			{
				Grenades--;
			}
			else
			{
				Grenades = 0;
			}
		}
	}
	GetWorldTimerManager().SetTimer(this, &AShooterCharacter::RefillGrenades, ThrowTime, false);
}

bool AShooterCharacter::ServerGrenadeThrow_Validate(FVector StartLocation, FVector_NetQuantizeNormal MyAim)
{
	return true;
}

void AShooterCharacter::ServerGrenadeThrow_Implementation(FVector StartLocation, FVector_NetQuantizeNormal MyAim)
{
	FTransform SpawnTM(MyAim.Rotation(), StartLocation);
	AShooterProjectile* Grenade = Cast<AShooterProjectile>(UGameplayStatics::BeginSpawningActorFromClass(this, GrenadeConfig.GrenadeClass, SpawnTM));

	GEngine->AddOnScreenDebugMessage(-1, 10.0f, FColor::Yellow, TEXT("SERVER GRENADE THROW IMPLEMENTATION!!!!!!!!"));
	if (Grenade)
	{
		GEngine->AddOnScreenDebugMessage(-1, 10.0f, FColor::Yellow, TEXT("SERVER GRENADE THROW IMPLEMENTATION 2!!!!!!!!"));
		Grenade->Instigator = Instigator;
		Grenade->SetOwner(this);
		Grenade->InitVelocity(MyAim);

		UGameplayStatics::FinishSpawningActor(Grenade, SpawnTM);

		CurrentWeapon->StartGrenadeAnim();

		GEngine->AddOnScreenDebugMessage(-1, 10.0f, FColor::Yellow, TEXT("SERVER GRENADE THROW IMPLEMENTATION 3!!!!!!!!"));
	}

	/*if (bThrowingGrenade || Grenades <= 0)
	{
	return;
	}

	// Don't allow grenade throwing
	bThrowingGrenade = true;

	if (bThrowingGrenade)
	{
	if (Grenades > 0)
	{
	GetWorldTimerManager().SetTimer(this, &AShooterCharacter::PerformGrenadeThrow, SpawnGrenTime, false);
	}
	}
	GetWorldTimerManager().SetTimer(this, &AShooterCharacter::RefillGrenades, ThrowTime, false);
	*/
}

void AShooterCharacter::RefillGrenades()
{

	// allow throwing grenades again
	bThrowingGrenade = false;
}

void AShooterCharacter::PerformGrenadeThrow()
{
	CurrentWeapon->ThrowGrenade(GrenadeConfig);

	/*FVector CameraLoc;
	FRotator CameraRot;
	GetActorEyesViewPoint(CameraLoc, CameraRot);

	FVector const GrenadeLoc = CameraLoc + FTransform(CameraRot).TransformVector(GrenadeOffset);
	FRotator GrenadeRotation = CameraRot;

	UWorld* const World = GetWorld();

	FVector const LaunchDir = GrenadeRotation.Vector();

	FRotator const rot = FirstPersonCameraComponent->GetComponentRotation();

	FVector const Launch = rot.Vector();

	ServerGrenadeThrow(GrenadeLoc, Launch);*/

	/*
	FVector SpreadVector;
	FVector Aim;
	FVector SpawnLocation, AddLocation;
	FVector CamLoc;
	FRotator CamRot;

	USkeletalMeshComponent* UseMesh = CurrentWeapon->GetWeaponMesh();

	SpawnLocation = UseMesh->GetSocketLocation(CurrentWeapon->MyMuzzleAttachPoint);

	AddLocation = FVector(0, 0, 100);
	//SpawnLocation = CharacterMovement->GetActorLocation() + AddLocation;
	Controller->GetPlayerViewPoint(CamLoc, CamRot);
	Aim = CamRot.Vector();

	if (bThrowingGrenade)
	{
	ServerGrenadeThrow(SpawnLocation, Aim);
	}
	*/
}

void AShooterCharacter::ApplyGrenadeConfig(FGrenadeData& Data)
{
	Data = GrenadeConfig;
}

void AShooterCharacter::MyStartCrouch()
{
	GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Yellow, TEXT("MY START CROUCH"));
	GetCharacterMovement()->bWantsToCrouch = true;

	if (GetCharacterMovement()->IsFalling())
	{
		GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Yellow, TEXT("CROUCH JUMP"));
		//FVector newLoc = GetMesh()->RelativeLocation;
		//newLoc.Z = newLoc.Z - 9999;
		//GetMesh()->SetRelativeLocation(newLoc);

	}
}

void AShooterCharacter::MyStopCrouch()
{
	GEngine->AddOnScreenDebugMessage(-1, 1.0f, FColor::Yellow, TEXT("MY STOP CROUCH"));
	GetCharacterMovement()->bWantsToCrouch = false;

	if (GetCharacterMovement()->IsFalling())
	{
		GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Yellow, TEXT("CROUCH JUMP END"));
		//FVector newLoc = GetMesh()->RelativeLocation;
		//newLoc.Z = newLoc.Z + 9999;
		//GetMesh()->SetRelativeLocation(newLoc);
	}
}

void AShooterCharacter::MyRecalculateBaseEyeHeight()
{
	RecalculateBaseEyeHeight();
	if (bIsCrouched)
	{
		GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Yellow, TEXT("bIsCrouched = true"));
	}
	else
	{
		GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Yellow, TEXT("bIsCrouched = false"));
	}

}

void AShooterCharacter::OnStartCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust)
{
	RecalculateBaseEyeHeight();

	if (GetMesh())
	{
		if (GetCharacterMovement()->IsFalling())
		{
			// we are in the air so this is custom

			if (CrouchJumpMeshOffsetZ != 0)
			{
				GetMesh()->RelativeLocation.Z = GetDefault<AShooterCharacter>(GetClass())->GetMesh()->RelativeLocation.Z + CrouchJumpMeshOffsetZ;
				BaseTranslationOffset.Z = GetMesh()->RelativeLocation.Z;
			}
			FirstPersonCameraComponent->RelativeLocation.Z += CrouchCameraOffsetZ;
			
		}
		else
		{
			if (!bCrouchCameraOffsetOnlyInAir)
			{
				FirstPersonCameraComponent->RelativeLocation.Z += CrouchCameraOffsetZ;
			}

			GetMesh()->RelativeLocation.Z = GetDefault<AShooterCharacter>(GetClass())->GetMesh()->RelativeLocation.Z + HalfHeightAdjust;
			BaseTranslationOffset.Z = GetMesh()->RelativeLocation.Z;
		}
	}
	else
	{
		BaseTranslationOffset.Z = GetDefault<AShooterCharacter>(GetClass())->GetMesh()->RelativeLocation.Z + HalfHeightAdjust;

	}

	K2_OnStartCrouch(HalfHeightAdjust, ScaledHalfHeightAdjust);
}

void AShooterCharacter::OnEndCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust)
{
	Super::OnEndCrouch(HalfHeightAdjust, ScaledHalfHeightAdjust);

	FirstPersonCameraComponent->RelativeLocation.Z = GetDefault<AShooterCharacter>(GetClass())->FirstPersonCameraComponent->RelativeLocation.Z;
}

void AShooterCharacter::UpdateZoom(float DeltaTime)
{
	AShooterPlayerController* MyPC = Cast<AShooterPlayerController>(Controller);
	if (CurrentWeapon->CurrentZoomLevel != CurrentWeapon->MaxZoomLevel)
	{
		MyPC->PlayerCameraManager->DefaultFOV = CurrentWeapon->ZoomLevelFOV[CurrentWeapon->CurrentZoomLevel - 1];
	}
	else
	{
		//unzoom
		MyPC->PlayerCameraManager->DefaultFOV = 110;
	}
}

void AShooterCharacter::StartZoom()
{
	/*if (CurrentZoomLevel != MaxZoomLevel)
	{
	CurrentZoomLevel += 1;
	PerformZoom(CurrentZoomLevel);
	}
	else
	{
	EndZoom();
	}*/

	AShooterPlayerController* MyPC = Cast<AShooterPlayerController>(Controller);
	MyPC->PlayerCameraManager->UnlockFOV();

	GEngine->AddOnScreenDebugMessage(-1, 1.0f, FColor::Yellow, TEXT("START ZOOM"));

	if (CurrentWeapon->CurrentZoomLevel != CurrentWeapon->MaxZoomLevel)
	{
		CurrentWeapon->CurrentZoomLevel += 1;
		//PerformZoom(CurrentWeapon->CurrentZoomLevel);
	}
	else
	{
		CurrentWeapon->CurrentZoomLevel = 0;
		//EndZoom();
	}
}

void AShooterCharacter::PerformZoom(int32 ZoomLevel)
{
	AShooterPlayerController* MyPC = Cast<AShooterPlayerController>(Controller);
	MyPC->PlayerCameraManager->SetFOV(CurrentWeapon->ZoomLevelFOV[ZoomLevel - 1]);
	//MyPC->PlayerCameraManager->DefaultFOV = CurrentWeapon->ZoomLevelFOV[ZoomLevel - 1];
	//Should call the zoom overlay draw here as well at a later date
}

void AShooterCharacter::EndZoom()
{

	if (Controller)
	{
		AShooterPlayerController* MyPC = Cast<AShooterPlayerController>(Controller);
		if (CurrentWeapon->CurrentZoomLevel > 0)
		{
			CurrentWeapon->CurrentZoomLevel = 0;
			//MyPC->PlayerCameraManager->SetFOV(MyPC->PlayerCameraManager->DefaultFOV);
			//float f = 110;
			//MyPC->PlayerCameraManager->SetFOV(StandardFOV);
			//MyPC->PlayerCameraManager->DefaultFOV = f;
		}
	}

	/*
	AShooterPlayerController* MyPC = Cast<AShooterPlayerController>(Controller);
	if (CurrentWeapon->CurrentZoomLevel > 0)
	{
	CurrentWeapon->CurrentZoomLevel = 0;
	//MyPC->PlayerCameraManager->SetFOV(MyPC->PlayerCameraManager->DefaultFOV);
	float f = 110;
	MyPC->PlayerCameraManager->SetFOV(f);
	//MyPC->PlayerCameraManager->DefaultFOV = f;
	}

	*/

	/*if (Role == ROLE_Authority)
	{
	ClientEndZoom();
	}*/
	//CurrentWeapon->CurrentZoomLevel = 0;
	//MyPC->PlayerCameraManager->SetFOV(MyPC->PlayerCameraManager->DefaultFOV);
}

void AShooterCharacter::ClientEndZoom_Implementation()
{
	AShooterPlayerController* MyPC = Cast<AShooterPlayerController>(Controller);
	CurrentWeapon->CurrentZoomLevel = 0;
	MyPC->PlayerCameraManager->SetFOV(MyPC->PlayerCameraManager->DefaultFOV);
	float f = MyPC->PlayerCameraManager->DefaultFOV;
	MyPC->PlayerCameraManager->SetFOV(f);
}



///////////////////////////////////////
// OVERSHIELD

void AShooterCharacter::CalculateOvershield()
{
	OvershieldChargeRate = (OvershieldChargeTime / (OSPower - CurrentShieldAmount)) * 5;

	bOvershieldCharging = true;

	ChargeOvershield();
}

void AShooterCharacter::ChargeOvershield()
{
	if (CurrentShieldAmount < OSPower)
	{
		CurrentShieldAmount += 5;

		if (CurrentShieldAmount < OSPower)
		{
			GetWorldTimerManager().SetTimer(this, &AShooterCharacter::ChargeOvershield, OvershieldChargeRate, false);
		}
		else
		{
			bOvershieldCharging = false;
			bHasOvershield = true;

			CurrentShieldAmount = OSPower;

			/////////////////////////////////// CALCULATE OS DECAY

			OvershieldDecayRate = (OvershieldDecayTime / (OSPower - MaxShieldAmount));

			DecayOvershield();
		}
	}
	else
	{
		bOvershieldCharging = false;
	}
}

void AShooterCharacter::DecayOvershield()
{

	if (CurrentShieldAmount > MaxShieldAmount)
	{
		CurrentShieldAmount -= 1;


		if (CurrentShieldAmount > MaxShieldAmount)
		{
			GetWorldTimerManager().SetTimer(this, &AShooterCharacter::DecayOvershield, OvershieldDecayRate, false);
		}
		else
		{
			bHasOvershield = false;
		}
	}
	else
	{
		bHasOvershield = false;
	}
}


bool AShooterCharacter::UpdatePreviousWeapon_Validate()
{
	return true;
}

void AShooterCharacter::UpdatePreviousWeapon_Implementation()
{
	PreviousWeapon = CurrentWeapon;
	
}

bool AShooterCharacter::ApplyPlasmaStun_Validate(float StunPower, float StunTime)
{
	return true;
}

void AShooterCharacter::ApplyPlasmaStun_Implementation(float StunPower, float StunTime)
{
	if (bAffectedByPlasmaStun)
	{
		GetCharacterMovement()->MaxWalkSpeed = GetCharacterMovement()->MaxWalkSpeed * StunPower;
		GetWorld()->GetTimerManager().SetTimer(this, &AShooterCharacter::ResetStun, StunTime, false);
	}
}

bool AShooterCharacter::ResetStun_Validate()
{
	return true;
}

void AShooterCharacter::ResetStun_Implementation()
{
	GetCharacterMovement()->MaxWalkSpeed = CachedMovementSpeedOnSpawn;
}






///////////////////// NEW MELEE STUFF TESTING
void AShooterCharacter::OnStartMeleeNew()
{
	GEngine->AddOnScreenDebugMessage(-1, 10.0f, FColor::Yellow, TEXT("ONSTARTMELEENEW!!!"));
	AShooterPlayerController* MyPC = Cast<AShooterPlayerController>(Controller);
	if (MyPC && MyPC->IsGameInputAllowed())
	{
		EndZoom();

		if (IsRunning())
		{
			SetRunning(false, false);
		}
		StartWeaponMelee();
	}
}

void AShooterCharacter::StartWeaponMelee()
{
	GEngine->AddOnScreenDebugMessage(-1, 10.0f, FColor::Yellow, TEXT("STARTWEAPONMELEE!!!"));
	if (!bWantsToMeleeNew)
	{
		//bWantsToMeleeNew = true;
		if (CurrentWeapon)
		{
			CurrentWeapon->StartMeleeNew();
		}
	}
}

///////////////////////// NEW GRENADE STUFF
void AShooterCharacter::OnStartGrenadeNew()
{
	AShooterPlayerController* MyPC = Cast<AShooterPlayerController>(Controller);
	if (MyPC && MyPC->IsGameInputAllowed())
	{
		if (IsRunning())
		{
			SetRunning(false, false);
		}
		StartGrenadeThrowNew();
	}
}

void AShooterCharacter::StartGrenadeThrowNew()
{
	if (!bWantsToGrenade)
	{
		if (CurrentWeapon)
		{
			CurrentWeapon->StartGrenadeNew();
		}
	}
}

FGrenadeData AShooterCharacter::GetGrenadeConfig()
{
	return GrenadeConfig;
}