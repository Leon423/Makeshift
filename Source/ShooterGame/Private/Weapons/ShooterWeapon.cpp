// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "ShooterGame.h"
#include "Particles/ParticleSystemComponent.h"
//changed stuff
AShooterWeapon::AShooterWeapon(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	Mesh1P = ObjectInitializer.CreateDefaultSubobject<USkeletalMeshComponent>(this, TEXT("WeaponMesh1P"));
	Mesh1P->MeshComponentUpdateFlag = EMeshComponentUpdateFlag::OnlyTickPoseWhenRendered;
	Mesh1P->bChartDistanceFactor = false;
	Mesh1P->bReceivesDecals = false;
	Mesh1P->CastShadow = false;
	Mesh1P->SetCollisionObjectType(ECC_WorldDynamic);
	Mesh1P->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Mesh1P->SetCollisionResponseToAllChannels(ECR_Ignore);
	RootComponent = Mesh1P;

	Mesh3P = ObjectInitializer.CreateDefaultSubobject<USkeletalMeshComponent>(this, TEXT("WeaponMesh3P"));
	Mesh3P->MeshComponentUpdateFlag = EMeshComponentUpdateFlag::OnlyTickPoseWhenRendered;
	Mesh3P->bChartDistanceFactor = true;
	Mesh3P->bReceivesDecals = false;
	Mesh3P->CastShadow = true;
	Mesh3P->SetCollisionObjectType(ECC_WorldDynamic);
	Mesh3P->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Mesh3P->SetCollisionResponseToAllChannels(ECR_Ignore);
	Mesh3P->SetCollisionResponseToChannel(COLLISION_WEAPON, ECR_Block);
	Mesh3P->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	Mesh3P->SetCollisionResponseToChannel(COLLISION_PROJECTILE, ECR_Block);
	Mesh3P->AttachParent = Mesh1P;

	MeleeBoxNew = ObjectInitializer.CreateDefaultSubobject<UBoxComponent>(this, TEXT("WeaponMeleeBoxNew"));
	MeleeBoxNew->SetIsReplicated(true);
	MeleeBoxNew->AttachParent = Mesh1P;
	MeleeBoxNew->SetCollisionObjectType(ECC_WorldDynamic);
	MeleeBoxNew->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	MeleeBoxNew->SetCollisionResponseToAllChannels(ECR_Overlap);

	bLoopedMuzzleFX = false;
	bLoopedFireAnim = false;
	bPlayingFireAnim = false;
	bIsEquipped = false;
	bWantsToFire = false;
	bPendingReload = false;
	bPendingEquip = false;
	CurrentState = EWeaponState::Idle;

	CurrentAmmo = 0;
	CurrentAmmoInClip = 0;
	BurstCounter = 0;
	LastFireTime = 0.0f;
	MeleeCounter = 0;
	GrenadeCounter = 0;

	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickGroup = TG_PrePhysics;
	SetRemoteRoleForBackwardsCompat(ROLE_SimulatedProxy);
	bReplicates = true;
	bReplicateInstigator = true;
	bNetUseOwnerRelevancy = true;

	bMyCanFire = true;
	bMyCanMelee = true;
	bMyCanReload = true;

	MuzzleAttachPoint = MuzzleAttachPoint;
}

void AShooterWeapon::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	if (WeaponConfig.InitialClips > 0)
	{
		CurrentAmmoInClip = WeaponConfig.AmmoPerClip;
		CurrentAmmo = WeaponConfig.AmmoPerClip * WeaponConfig.InitialClips;
	}

	DetachMeshFromPawn();
}

void AShooterWeapon::Destroyed()
{
	Super::Destroyed();

	StopSimulatingWeaponFire();
	StopSimulatingWeaponMelee();
}

//////////////////////////////////////////////////////////////////////////
// Inventory

void AShooterWeapon::OnEquip()
{
	AttachMeshToPawn();

	bPendingEquip = true;
	DetermineWeaponState();

	float Duration = PlayWeaponAnimation(EquipAnim);
	if (Duration <= 0.0f)
	{
		// failsafe
		Duration = 0.5f;
	}
	EquipStartedTime = GetWorld()->GetTimeSeconds();
	EquipDuration = Duration;

	GetWorldTimerManager().SetTimer(this, &AShooterWeapon::OnEquipFinished, Duration, false);

	if (MyPawn && MyPawn->IsLocallyControlled())
	{
		PlayWeaponSound(EquipSound);
	}
}

void AShooterWeapon::OnEquipFinished()
{
	AttachMeshToPawn();

	bIsEquipped = true;
	bPendingEquip = false;

	// Determine the state so that the can reload checks will work
	DetermineWeaponState(); 
	
	if (MyPawn)
	{
		// try to reload empty clip
		if (MyPawn->IsLocallyControlled() &&
			CurrentAmmoInClip <= 0 &&
			CanReload())
		{
			StartReload();
		}
	}

	
}

void AShooterWeapon::OnUnEquip()
{
	DetachMeshFromPawn();
	bIsEquipped = false;
	StopFire();

	if (bPendingReload)
	{
		StopWeaponAnimation(ReloadAnim);
		bPendingReload = false;

		GetWorldTimerManager().ClearTimer(this, &AShooterWeapon::StopReload);
		GetWorldTimerManager().ClearTimer(this, &AShooterWeapon::ReloadWeapon);
	}

	if (bPendingEquip)
	{
		StopWeaponAnimation(EquipAnim);
		bPendingEquip = false;

		GetWorldTimerManager().ClearTimer(this, &AShooterWeapon::OnEquipFinished);
	}

	DetermineWeaponState();
}

void AShooterWeapon::OnEnterInventory(AShooterCharacter* NewOwner)
{
	SetOwningPawn(NewOwner);
}

void AShooterWeapon::OnLeaveInventory()
{
	if (Role == ROLE_Authority)
	{
		SetOwningPawn(NULL);
	}

	if (IsAttachedToPawn())
	{
		OnUnEquip();
	}
}

void AShooterWeapon::AttachMeshToPawn()
{
	if (MyPawn)
	{
		// Remove and hide both first and third person meshes
		DetachMeshFromPawn();

		// For locally controller players we attach both weapons and let the bOnlyOwnerSee, bOwnerNoSee flags deal with visibility.
		FName AttachPoint = MyPawn->GetWeaponAttachPoint();
		if( MyPawn->IsLocallyControlled() == true )
		{
			USkeletalMeshComponent* PawnMesh1p = MyPawn->GetSpecifcPawnMesh(true);
			USkeletalMeshComponent* PawnMesh3p = MyPawn->GetSpecifcPawnMesh(false);
			Mesh1P->SetHiddenInGame( false );
			Mesh3P->SetHiddenInGame( false );
			Mesh1P->AttachTo(PawnMesh1p, AttachPoint);
			Mesh3P->AttachTo(PawnMesh3p, AttachPoint);
		}
		else
		{
			USkeletalMeshComponent* UseWeaponMesh = GetWeaponMesh();
			USkeletalMeshComponent* UsePawnMesh = MyPawn->GetPawnMesh();
			UseWeaponMesh->AttachTo(UsePawnMesh, AttachPoint);	
			UseWeaponMesh->SetHiddenInGame( false );
		}
	}
}

void AShooterWeapon::DetachMeshFromPawn()
{
	Mesh1P->DetachFromParent();
	Mesh1P->SetHiddenInGame(true);

	Mesh3P->DetachFromParent();
	Mesh3P->SetHiddenInGame(true);
}


//////////////////////////////////////////////////////////////////////////
// Input

void AShooterWeapon::StartFire()
{
	if (Role < ROLE_Authority)
	{
		ServerStartFire();
	}

	if (!bWantsToFire)
	{
		bWantsToFire = true;
		DetermineWeaponState();
	}
}

void AShooterWeapon::StopFire()
{
	if (Role < ROLE_Authority)
	{
		ServerStopFire();
	}

	if (bWantsToFire)
	{
		bWantsToFire = false;
		DetermineWeaponState();
	}
}

void AShooterWeapon::StartReload(bool bFromReplication)
{
	if (!bFromReplication && Role < ROLE_Authority)
	{
		ServerStartReload();
	}

	if (bFromReplication || CanReload())
	{
		bPendingReload = true;
		DetermineWeaponState();

		float AnimDuration = PlayWeaponAnimation(ReloadAnim);		
		if (AnimDuration <= 0.0f)
		{
			AnimDuration = WeaponConfig.NoAnimReloadDuration;
		}

		GetWorldTimerManager().SetTimer(this, &AShooterWeapon::StopReload, AnimDuration, false);
		if (Role == ROLE_Authority)
		{
			GetWorldTimerManager().SetTimer(this, &AShooterWeapon::ReloadWeapon, FMath::Max(0.1f, AnimDuration - 0.1f), false);
		}
		
		if (MyPawn && MyPawn->IsLocallyControlled())
		{
			PlayWeaponSound(ReloadSound);
		}
	}
}

void AShooterWeapon::StopReload()
{
	if (CurrentState == EWeaponState::Reloading)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Yellow, TEXT("StopReloadActuallyDidStuff"));
		bPendingReload = false;
		DetermineWeaponState();
		StopWeaponAnimation(ReloadAnim);
	}
}

bool AShooterWeapon::ServerStartFire_Validate()
{
	return true;
}

void AShooterWeapon::ServerStartFire_Implementation()
{
	StartFire();
}

bool AShooterWeapon::ServerStopFire_Validate()
{
	return true;
}

void AShooterWeapon::ServerStopFire_Implementation()
{
	StopFire();
}

bool AShooterWeapon::ServerStartReload_Validate()
{
	return true;
}

void AShooterWeapon::ServerStartReload_Implementation()
{
	StartReload();
}

bool AShooterWeapon::ServerStopReload_Validate()
{
	return true;
}

void AShooterWeapon::ServerStopReload_Implementation()
{
	StopReload();
}

void AShooterWeapon::ClientStartReload_Implementation()
{
	StartReload();
}

//////////////////////////////////////////////////////////////////////////
// Control

bool AShooterWeapon::CanFire() const
{
	bool bCanFire = MyPawn && MyPawn->CanFire();
	bool bStateOKToFire = ( ( CurrentState ==  EWeaponState::Idle ) || ( CurrentState == EWeaponState::Firing) );	
	return (( bCanFire == true ) && ( bStateOKToFire == true ) && ( bPendingReload == false ));
}

bool AShooterWeapon::CanReload() const
{
	bool bCanReload = (!MyPawn || MyPawn->CanReload());
	bool bGotAmmo = ( CurrentAmmoInClip < WeaponConfig.AmmoPerClip) && (CurrentAmmo - CurrentAmmoInClip > 0 || HasInfiniteClip());
	bool bStateOKToReload = ( ( CurrentState ==  EWeaponState::Idle ) || ( CurrentState == EWeaponState::Firing) );
	return ((bCanReload == true) && (bGotAmmo == true) && (bStateOKToReload == true) && (bIsOverheated == false));
}


//////////////////////////////////////////////////////////////////////////
// Weapon usage

void AShooterWeapon::GiveAmmo(int AddAmount)
{
	const int32 MissingAmmo = FMath::Max(0, WeaponConfig.MaxAmmo - CurrentAmmo);
	AddAmount = FMath::Min(AddAmount, MissingAmmo);
	CurrentAmmo += AddAmount;

	AShooterAIController* BotAI = MyPawn ? Cast<AShooterAIController>(MyPawn->GetController()) : NULL;
	if (BotAI)
	{
		BotAI->CheckAmmo(this);
	}
	
	// start reload if clip was empty
	if (GetCurrentAmmoInClip() <= 0 &&
		CanReload() &&
		MyPawn->GetWeapon() == this)
	{
		ClientStartReload();
	}
}

void AShooterWeapon::UseAmmo()
{
	if (bControlsProjectileExplosion && ActiveProjectile != NULL)
	{
		return;
	}

	if (!HasInfiniteAmmo())
	{
		CurrentAmmoInClip--;
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
	else if(PlayerController)
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

// CHANGED STUFF IN HERE
void AShooterWeapon::HandleFiring()
{
	if ((CurrentAmmoInClip > 0 || HasInfiniteClip() || HasInfiniteAmmo()) && CanFire())
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Yellow, TEXT("We Are In HanleFiring and can fire"));
		if (GetNetMode() != NM_DedicatedServer)
		{
			SimulateWeaponFire();
		}

		if (MyPawn && MyPawn->IsLocallyControlled())
		{
			FireWeapon();

			if (bControlsProjectileExplosion)
			{

			}
			else
			{
				UseAmmo();
			}


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
			GetWorldTimerManager().SetTimer(this, &AShooterWeapon::HandleFiring, WeaponConfig.TimeBetweenShots, false);
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

bool AShooterWeapon::ServerHandleFiring_Validate()
{
	return true;
}

void AShooterWeapon::ServerHandleFiring_Implementation()
{
	const bool bShouldUpdateAmmo = (CurrentAmmoInClip > 0 && CanFire());

	HandleFiring();

	if (bShouldUpdateAmmo)
	{
		// update ammo
		if (bControlsProjectileExplosion)
		{

		}
		else
		{
			UseAmmo();
		}
		

		// update firing FX on remote clients
		BurstCounter++;
	}
}

void AShooterWeapon::ReloadWeapon()
{
	int32 ClipDelta = FMath::Min(WeaponConfig.AmmoPerClip - CurrentAmmoInClip, CurrentAmmo - CurrentAmmoInClip);

	if (HasInfiniteClip())
	{
		ClipDelta = WeaponConfig.AmmoPerClip - CurrentAmmoInClip;
	}

	if (ClipDelta > 0)
	{
		CurrentAmmoInClip += ClipDelta;
	}

	if (HasInfiniteClip())
	{
		CurrentAmmo = FMath::Max(CurrentAmmoInClip, CurrentAmmo);
	}
}

// CHANGED STUFF
void AShooterWeapon::SetWeaponState(EWeaponState::Type NewState)
{
	const EWeaponState::Type PrevState = CurrentState;

	if (PrevState == EWeaponState::Firing && NewState != EWeaponState::Firing)
	{
		OnBurstFinished();
	}

	CurrentState = NewState;

	if (PrevState != EWeaponState::Firing && NewState == EWeaponState::Firing)
	{
		OnBurstStarted();
	}

	if (NewState == EWeaponState::Meleeing)
	{
		//PerformMelee();
		OnMeleeStarted();
	}

	if (NewState == EWeaponState::Grenading)
	{
		OnGrenadeStarted();
	}
}

// CHANGED STUFF IN THIS
void AShooterWeapon::DetermineWeaponState()
{
	EWeaponState::Type NewState = EWeaponState::Idle;

	if (bIsEquipped)
	{
		if (bPendingReload)
		{
			if (CanReload() == false)
			{
				NewState = CurrentState;
			}
			else
			{
				NewState = EWeaponState::Reloading;
			}
		}
		else if ((bPendingReload == false) && (bWantsToFire == true) && (CanFire() == true))
		{
			NewState = EWeaponState::Firing;
		}

		/*if (bWantsToMelee == true && bCanMelee == true)
		{
			NewState = EWeaponState::Meleeing;
		}*/

		if ((bWantsToGrenadeNew == true) && (CanGrenade() == true))
		{
			NewState = EWeaponState::Grenading;
		}

		if ((bWantsToMeleeNew == true) && (CanMelee() == true))
		{
			NewState = EWeaponState::Meleeing;
		}
	}
	else if (bPendingEquip)
	{
		NewState = EWeaponState::Equipping;
	}

	SetWeaponState(NewState);
}

void AShooterWeapon::OnBurstStarted()
{
	// start firing, can be delayed to satisfy TimeBetweenShots
	const float GameTime = GetWorld()->GetTimeSeconds();
	if (LastFireTime > 0 && WeaponConfig.TimeBetweenShots > 0.0f &&
		LastFireTime + WeaponConfig.TimeBetweenShots > GameTime)
	{
		GetWorldTimerManager().SetTimer(this, &AShooterWeapon::HandleFiring, LastFireTime + WeaponConfig.TimeBetweenShots - GameTime, false);
	}
	else
	{
		HandleFiring();
	}
}

// CHANGED STUFF IN THIS
void AShooterWeapon::OnBurstFinished()
{
	// stop firing FX on remote clients
	BurstCounter = 0;

	// stop firing FX locally, unless it's a dedicated server
	if (GetNetMode() != NM_DedicatedServer)
	{
		StopSimulatingWeaponFire();
	}
	
	GetWorldTimerManager().ClearTimer(this, &AShooterWeapon::HandleFiring);
	bRefiring = false;
	GetWorldTimerManager().ClearTimer(this, &AShooterWeapon::ActivateRecoil);
	bShouldRecoil = false;
}


//////////////////////////////////////////////////////////////////////////
// Weapon usage helpers

UAudioComponent* AShooterWeapon::PlayWeaponSound(USoundCue* Sound)
{
	UAudioComponent* AC = NULL;
	if (Sound && MyPawn)
	{
		AC = UGameplayStatics::PlaySoundAttached(Sound, MyPawn->GetRootComponent());
	}

	return AC;
}

float AShooterWeapon::PlayWeaponAnimation(const FWeaponAnim& Animation)
{
	float Duration = 0.0f;
	if (MyPawn)
	{
		UAnimMontage* UseAnim = MyPawn->IsFirstPerson() ? Animation.Pawn1P : Animation.Pawn3P;
		if (UseAnim)
		{
			Duration = MyPawn->PlayAnimMontage(UseAnim);
		}
	}

	return Duration;
}

void AShooterWeapon::StopWeaponAnimation(const FWeaponAnim& Animation)
{
	if (MyPawn)
	{
		UAnimMontage* UseAnim = MyPawn->IsFirstPerson() ? Animation.Pawn1P : Animation.Pawn3P;
		if (UseAnim)
		{
			MyPawn->StopAnimMontage(UseAnim);
		}
	}
}

FVector AShooterWeapon::GetCameraAim() const
{
	AShooterPlayerController* const PlayerController = Instigator ? Cast<AShooterPlayerController>(Instigator->Controller) : NULL;
	FVector FinalAim = FVector::ZeroVector;

	if (PlayerController)
	{
		FVector CamLoc;
		FRotator CamRot;
		PlayerController->GetPlayerViewPoint(CamLoc, CamRot);
		FinalAim = CamRot.Vector();
	}
	else if (Instigator)
	{
		FinalAim = Instigator->GetBaseAimRotation().Vector();		
	}

	return FinalAim;
}

FVector AShooterWeapon::GetAdjustedAim() const
{
	AShooterPlayerController* const PlayerController = Instigator ? Cast<AShooterPlayerController>(Instigator->Controller) : NULL;
	FVector FinalAim = FVector::ZeroVector;
	// If we have a player controller use it for the aim
	if (PlayerController)
	{
		FVector CamLoc;
		FRotator CamRot;
		PlayerController->GetPlayerViewPoint(CamLoc, CamRot);
		FinalAim = CamRot.Vector();
	}
	else if (Instigator)
	{
		// Now see if we have an AI controller - we will want to get the aim from there if we do
		AShooterAIController* AIController = MyPawn ? Cast<AShooterAIController>(MyPawn->Controller) : NULL;
		if(AIController != NULL )
		{
			FinalAim = AIController->GetControlRotation().Vector();
		}
		else
		{			
			FinalAim = Instigator->GetBaseAimRotation().Vector();
		}
	}

	return FinalAim;
}

FVector AShooterWeapon::GetCameraDamageStartLocation(const FVector& AimDir) const
{
	AShooterPlayerController* PC = MyPawn ? Cast<AShooterPlayerController>(MyPawn->Controller) : NULL;
	AShooterAIController* AIPC = MyPawn ? Cast<AShooterAIController>(MyPawn->Controller) : NULL;
	FVector OutStartTrace = FVector::ZeroVector;

	if (PC)
	{
		// use player's camera
		FRotator UnusedRot;
		PC->GetPlayerViewPoint(OutStartTrace, UnusedRot);

		// Adjust trace so there is nothing blocking the ray between the camera and the pawn, and calculate distance from adjusted start
		OutStartTrace = OutStartTrace + AimDir * ((Instigator->GetActorLocation() - OutStartTrace) | AimDir);
	}
	else if (AIPC)
	{
		OutStartTrace = GetMuzzleLocation();
	}

	return OutStartTrace;
}

FVector AShooterWeapon::GetMuzzleLocation() const
{
	USkeletalMeshComponent* UseMesh = GetWeaponMesh();
	return UseMesh->GetSocketLocation(MuzzleAttachPoint);
}

FVector AShooterWeapon::GetMuzzleDirection() const
{
	USkeletalMeshComponent* UseMesh = GetWeaponMesh();
	return UseMesh->GetSocketRotation(MuzzleAttachPoint).Vector();
}

// CHANGED STUFF IN THIS
FHitResult AShooterWeapon::WeaponTrace(const FVector& StartTrace, const FVector& EndTrace) const
{
	/*
	static FName WeaponFireTag = FName(TEXT("WeaponTrace"));

	// Perform trace to retrieve hit info
	FCollisionQueryParams TraceParams(WeaponFireTag, true, Instigator);
	TraceParams.bTraceAsyncScene = true;
	TraceParams.bReturnPhysicalMaterial = true;

	FHitResult Hit(ForceInit);
	GetWorld()->LineTraceSingle(Hit, StartTrace, EndTrace, COLLISION_WEAPON, TraceParams);

	return Hit;
	*/

	static FName WeaponFireTag = FName(TEXT("WeaponTrace"));

	// Perform trace to retrieve hit info
	FCollisionQueryParams TraceParams(WeaponFireTag, true, Instigator);
	TraceParams.bTraceAsyncScene = true;
	TraceParams.bReturnPhysicalMaterial = true;
	//TraceParams.TraceTag = WeaponFireTag;

	FHitResult Hit(ForceInit);

	if (GetWorld()->LineTraceSingle(Hit, StartTrace, EndTrace, COLLISION_WEAPON, TraceParams))
	{


		FVector NewEnd;

		AActor* ActorThatWasHit;

		bool bHitShooterCharacter = false;

		ActorThatWasHit = Hit.GetActor();

		//	GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Green, TEXT("TraceExtemt:"));
		//GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Green, TraceExtent.ToString());

		if (Hit.GetActor())
		{
			// if the hit actor was a player, we dont need to do anything else.
			if (Cast<AShooterCharacter>(Hit.GetActor()))
			{
				return Hit;

			}

			//otherwise we need to do a box trace that only searches for players.
			else
			{
				TArray<FHitResult> ListOfHits;
				FCollisionShape shape = FCollisionShape::MakeBox(TraceExtent);
				FCollisionResponseParams ResponseParam(ECollisionResponse::ECR_Ignore);
				ResponseParam.CollisionResponse.Pawn = 1;
				
				if (GetWorld()->SweepMulti(ListOfHits, StartTrace, EndTrace, FQuat(), COLLISION_WEAPON, shape, TraceParams, ResponseParam))
				{



					//if we hit something  we do some stuff to get where the new line trace should trace to.
					if (ListOfHits.Num() > 0)
					{
						int32 numbertoprint;
						numbertoprint = ListOfHits.Num();
						//GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::FromInt(numbertoprint));

						for (int32 i = 0; i < ListOfHits.Num(); i++)
						{
							//GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::FromInt(i + 1));
							if (ListOfHits[i].GetActor())
							{
								if (Cast<AShooterCharacter>(ListOfHits[i].GetActor()))
								{
									//GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Green, TEXT("I HIT A SHOOTERCHARACTER"));

									//FHitResult NewHitResult(ForceInit);
									//GetWorld()->LineTraceSingle(NewHitResult, StartTrace, ListOfHits[i].ImpactPoint, COLLISION_WEAPON, TraceParams);

									//return NewHitResult;

									NewEnd = ListOfHits[i].ImpactPoint;

									FVector Test;

									const int32 RandomSeed = FMath::Rand();
									FRandomStream WeaponRandomStream(RandomSeed);

									Test = ListOfHits[i].ImpactPoint - StartTrace;

									FVector Cone = WeaponRandomStream.VRandCone(Test, 0, 0);

									NewEnd = StartTrace + Cone * 10000;

									ActorThatWasHit = ListOfHits[i].GetActor();
									bHitShooterCharacter = true;

									//GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, NewEnd.ToString());

									break;

									//return ListOfHits[i];
								}

							}

						}
					}
				}
			}
		}
		// we didnt hit anything so we do the same as above
		else
		{
			//GEngine->AddOnScreenDebugMessage(-1, 10.0f, FColor::Cyan, TEXT("Hit a BSP"));

			TArray<FHitResult> ListOfHits;
			FCollisionShape shape = FCollisionShape::MakeBox(TraceExtent);
			FCollisionResponseParams ResponseParam(ECollisionResponse::ECR_Ignore);
			ResponseParam.CollisionResponse.Pawn = 1;
			
			if (GetWorld()->SweepMulti(ListOfHits, StartTrace, EndTrace, FQuat(), COLLISION_WEAPON, shape, TraceParams, ResponseParam))
			{

				if (ListOfHits.Num() > 0)
				{
					int32 numbertoprint;
					numbertoprint = ListOfHits.Num();
					//GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::FromInt(numbertoprint));

					for (int32 i = 0; i < ListOfHits.Num(); i++)
					{
						//GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::FromInt(i + 1));
						if (ListOfHits[i].GetActor())
						{
							if (Cast<AShooterCharacter>(ListOfHits[i].GetActor()))
							{
								//	GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Green, TEXT("I HIT A SHOOTERCHARACTER"));

								//FHitResult NewHitResult(ForceInit);
								//GetWorld()->LineTraceSingle(NewHitResult, StartTrace, ListOfHits[i].ImpactPoint, COLLISION_WEAPON, TraceParams);

								//return NewHitResult;

								NewEnd = ListOfHits[i].ImpactPoint;

								FVector Test;

								const int32 RandomSeed = FMath::Rand();
								FRandomStream WeaponRandomStream(RandomSeed);

								Test = ListOfHits[i].ImpactPoint - StartTrace;

								FVector Cone = WeaponRandomStream.VRandCone(Test, 0, 0);

								NewEnd = StartTrace + Cone * 10000;

								ActorThatWasHit = ListOfHits[i].GetActor();
								bHitShooterCharacter = true;

								//	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, NewEnd.ToString());

								break;

								//return ListOfHits[i];
							}

						}

					}
				}
			}
		}

		if (bDrawWeaponTrace)
		{
			GetWorld()->DebugDrawTraceTag = WeaponFireTag;
		}
		else
		{
			GetWorld()->DebugDrawTraceTag = FName(TEXT(""));
		}

		// do a normal line trace for where we hit the player to ensure that it wasnt through geometry.
		if (ActorThatWasHit && bHitShooterCharacter)
		{
			FHitResult FinalHitResult(ForceInit);
			//	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, NewEnd.ToString());
			
			if (GetWorld()->LineTraceSingle(FinalHitResult, StartTrace, NewEnd, COLLISION_WEAPON, TraceParams))
			{
				return FinalHitResult;
			}
			

			
		}

		FString stringtoprint;

		Hit.BoneName.ToString(stringtoprint);
		//	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, stringtoprint);

		//GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("Charge amount, server: %f"), CurrentChargeAmount));

		return Hit;
	}
	else
	{
		//GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red,  TEXT("CRASH ALERT!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"));
		FHitResult TestHitResult(ForceInit);
		return TestHitResult;
	}
}

void AShooterWeapon::SetOwningPawn(AShooterCharacter* NewOwner)
{
	if (MyPawn != NewOwner)
	{
		Instigator = NewOwner;
		MyPawn = NewOwner;
		// net owner for RPC calls
		SetOwner(NewOwner);
	}	
}

//////////////////////////////////////////////////////////////////////////
// Replication & effects

void AShooterWeapon::OnRep_MyPawn()
{
	if (MyPawn)
	{
		OnEnterInventory(MyPawn);
	}
	else
	{
		OnLeaveInventory();
	}
}

void AShooterWeapon::OnRep_BurstCounter()
{
	if (BurstCounter > 0)
	{
		SimulateWeaponFire();
	}
	else
	{
		StopSimulatingWeaponFire();
	}
}

void AShooterWeapon::OnRep_Reload()
{
	if (bPendingReload)
	{
		StartReload(true);
	}
	else
	{
		StopReload();
	}
}

void AShooterWeapon::SimulateWeaponFire()
{
	if (bControlsProjectileExplosion && ActiveProjectile != NULL)
	{
		return;
	}

	if (Role == ROLE_Authority && CurrentState != EWeaponState::Firing)
	{
		return;
	}

	if (MuzzleFX)
	{
		USkeletalMeshComponent* UseWeaponMesh = GetWeaponMesh();
		if (!bLoopedMuzzleFX || MuzzlePSC == NULL)
		{
			// Split screen requires we create 2 effects. One that we see and one that the other player sees.
			if( (MyPawn != NULL ) && ( MyPawn->IsLocallyControlled() == true ) )
			{
				AController* PlayerCon = MyPawn->GetController();				
				if( PlayerCon != NULL )
				{
					Mesh1P->GetSocketLocation(MuzzleAttachPoint);
					MuzzlePSC = UGameplayStatics::SpawnEmitterAttached(MuzzleFX, Mesh1P, MuzzleAttachPoint);
					MuzzlePSC->bOwnerNoSee = false;
					MuzzlePSC->bOnlyOwnerSee = true;

					Mesh3P->GetSocketLocation(MuzzleAttachPoint);
					MuzzlePSCSecondary = UGameplayStatics::SpawnEmitterAttached(MuzzleFX, Mesh3P, MuzzleAttachPoint);
					MuzzlePSCSecondary->bOwnerNoSee = true;
					MuzzlePSCSecondary->bOnlyOwnerSee = false;				
				}				
			}
			else
			{
				MuzzlePSC = UGameplayStatics::SpawnEmitterAttached(MuzzleFX, UseWeaponMesh, MuzzleAttachPoint);
			}
		}
	}

	if (!bLoopedFireAnim || !bPlayingFireAnim)
	{
		PlayWeaponAnimation(FireAnim);
		bPlayingFireAnim = true;
	}

	if (bLoopedFireSound)
	{
		if (FireAC == NULL)
		{
			FireAC = PlayWeaponSound(FireLoopSound);
		}
	}
	else
	{
		PlayWeaponSound(FireSound);
	}

	AShooterPlayerController* PC = (MyPawn != NULL) ? Cast<AShooterPlayerController>(MyPawn->Controller) : NULL;
	if (PC != NULL && PC->IsLocalController())
	{
		if (FireCameraShake != NULL)
		{
			PC->ClientPlayCameraShake(FireCameraShake, 1);
		}
		if (FireForceFeedback != NULL)
		{
			PC->ClientPlayForceFeedback(FireForceFeedback, false, "Weapon");
		}
	}
}

void AShooterWeapon::StopSimulatingWeaponFire()
{
	if (bLoopedMuzzleFX )
	{
		if( MuzzlePSC != NULL )
		{
			MuzzlePSC->DeactivateSystem();
			MuzzlePSC = NULL;
		}
		if( MuzzlePSCSecondary != NULL )
		{
			MuzzlePSCSecondary->DeactivateSystem();
			MuzzlePSCSecondary = NULL;
		}
	}

	if (bLoopedFireAnim && bPlayingFireAnim)
	{
		StopWeaponAnimation(FireAnim);
		bPlayingFireAnim = false;
	}

	if (FireAC)
	{
		FireAC->FadeOut(0.1f, 0.0f);
		FireAC = NULL;

		PlayWeaponSound(FireFinishSound);
	}
}

void AShooterWeapon::GetLifetimeReplicatedProps( TArray< FLifetimeProperty > & OutLifetimeProps ) const
{
	Super::GetLifetimeReplicatedProps( OutLifetimeProps );

	DOREPLIFETIME( AShooterWeapon, MyPawn );

	DOREPLIFETIME_CONDITION( AShooterWeapon, CurrentAmmo,		COND_OwnerOnly );
	DOREPLIFETIME_CONDITION( AShooterWeapon, CurrentAmmoInClip, COND_OwnerOnly );

	DOREPLIFETIME_CONDITION( AShooterWeapon, BurstCounter,		COND_SkipOwner );
	DOREPLIFETIME_CONDITION( AShooterWeapon, bPendingReload,	COND_SkipOwner );

	DOREPLIFETIME_CONDITION(AShooterWeapon, MeleeCounter, COND_SkipOwner);
	DOREPLIFETIME_CONDITION(AShooterWeapon, GrenadeCounter, COND_SkipOwner);

	DOREPLIFETIME(AShooterWeapon, bIsOverheated);

	DOREPLIFETIME(AShooterWeapon, ActiveProjectile);
}

USkeletalMeshComponent* AShooterWeapon::GetWeaponMesh() const
{
	return (MyPawn != NULL && MyPawn->IsFirstPerson()) ? Mesh1P : Mesh3P;
}

class AShooterCharacter* AShooterWeapon::GetPawnOwner() const
{
	return MyPawn;
}

bool AShooterWeapon::IsEquipped() const
{
	return bIsEquipped;
}

bool AShooterWeapon::IsAttachedToPawn() const
{
	return bIsEquipped || bPendingEquip;
}

EWeaponState::Type AShooterWeapon::GetCurrentState() const
{
	return CurrentState;
}

int32 AShooterWeapon::GetCurrentAmmo() const
{
	return CurrentAmmo;
}

int32 AShooterWeapon::GetCurrentAmmoInClip() const
{
	return CurrentAmmoInClip;
}

int32 AShooterWeapon::GetAmmoPerClip() const
{
	return WeaponConfig.AmmoPerClip;
}

int32 AShooterWeapon::GetMaxAmmo() const
{
	return WeaponConfig.MaxAmmo;
}

bool AShooterWeapon::HasInfiniteAmmo() const
{
	const AShooterPlayerController* MyPC = (MyPawn != NULL) ? Cast<const AShooterPlayerController>(MyPawn->Controller) : NULL;
	return WeaponConfig.bInfiniteAmmo || (MyPC && MyPC->HasInfiniteAmmo());
}

bool AShooterWeapon::HasInfiniteClip() const
{
	const AShooterPlayerController* MyPC = (MyPawn != NULL) ? Cast<const AShooterPlayerController>(MyPawn->Controller) : NULL;
	return WeaponConfig.bInfiniteClip || (MyPC && MyPC->HasInfiniteClip());
}

float AShooterWeapon::GetEquipStartedTime() const
{
	return EquipStartedTime;
}

float AShooterWeapon::GetEquipDuration() const
{
	return EquipDuration;
}

///////////////////////// ADDED BELOW THIS:::::::::::::

void AShooterWeapon::PickedUp(int32 Clips)
{
	/*WeaponConfig.InitialClips = Clips;

	CurrentAmmo = WeaponConfig.AmmoPerClip * WeaponConfig.InitialClips;
	*/
	WeaponConfig.InitialClips = 0;

	/*if (Clips < WeaponConfig.AmmoPerClip)
	{
	GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Yellow, TEXT("first if"));
	CurrentAmmoInClip = Clips;
	CurrentAmmo = Clips;
	//CurrentAmmo = WeaponConfig.AmmoPerClip;
	//CurrentAmmo = CurrentAmmo - (CurrentAmmo - Clips);
	}
	else if (Clips >= (WeaponConfig.MaxAmmo - CurrentAmmo))
	{
	GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Yellow, TEXT("second if"));
	CurrentAmmo = WeaponConfig.MaxAmmo;
	}
	else
	{
	GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Yellow, TEXT("third if"));
	CurrentAmmo = Clips;
	}*/

	// this should only get called when picking up a weapon, not ammo for a weapon
	if (Clips < WeaponConfig.AmmoPerClip)
	{
		CurrentAmmoInClip = Clips;
		CurrentAmmo = Clips;
	}
	else
	{
		CurrentAmmo = Clips;
	}
}

////////// GRENADES

void AShooterWeapon::ThrowGrenade(FGrenadeData GrenadeData)
{
	if (Cast<AShooterCharacter>(Instigator))
	{
			FVector ShootDir = GetAdjustedAim();
			FVector Origin = GetMuzzleLocation();

			// trace from camera to check what's under crosshair
			const float ProjectileAdjustRange = 10000.0f;
			const FVector StartTrace = GetCameraDamageStartLocation(ShootDir);
			const FVector EndTrace = StartTrace + ShootDir * ProjectileAdjustRange;
			FHitResult Impact = WeaponTrace(StartTrace, EndTrace);

			// and adjust directions to hit that actor
			if (Impact.bBlockingHit)
			{
				const FVector AdjustedDir = (Impact.ImpactPoint - Origin).SafeNormal();
				bool bWeaponPenetration = false;

				const float DirectionDot = FVector::DotProduct(AdjustedDir, ShootDir);
				if (DirectionDot < 0.0f)
				{
					// shoot backwards = weapon is penetrating
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
					ShootDir = AdjustedDir;
				}
			}

			Origin += Cast<AShooterCharacter>(Instigator)->GrenadeOffset;
			//PlayWeaponAnimation(GrenadeAnim);
			ServerGrenadeThrow(Origin, ShootDir, GrenadeData);
	}
}

bool AShooterWeapon::ServerGrenadeThrow_Validate(FVector Origin, FVector_NetQuantizeNormal ShootDir, FGrenadeData GrenadeData)
{
	return true;
}

void AShooterWeapon::ServerGrenadeThrow_Implementation(FVector Origin, FVector_NetQuantizeNormal ShootDir, FGrenadeData GrenadeData)
{
	FTransform SpawnTM(ShootDir.Rotation(), Origin);
	AShooterProjectile* Grenade = Cast<AShooterProjectile>(UGameplayStatics::BeginSpawningActorFromClass(this, GrenadeData.GrenadeClass, SpawnTM));
	
	if (Grenade)
	{
		Grenade->Instigator = Instigator;
		Grenade->SetOwner(this);
		Grenade->InitVelocity(ShootDir);

		UGameplayStatics::FinishSpawningActor(Grenade, SpawnTM);
	}
}

void AShooterWeapon::AddAmmoFromPickup(int32 Bullets)
{
	GiveAmmo(Bullets);
}

void AShooterWeapon::ActivateRecoil()
{
	bShouldRecoil = true;
}

FHitResult AShooterWeapon::MyLineTrace(const FVector& Start, const FVector& End, ECollisionChannel TraceChannel, const struct FCollisionQueryParams& Params) const
{
	FHitResult HitResult(ForceInit);

	GetWorld()->LineTraceSingle(HitResult, Start, End, TraceChannel, Params);

	//GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Cyan, TEXT("MYLINETRACE IS GETTINC ACALLLED"));

	return HitResult;
}

void AShooterWeapon::StartGrenadeAnim()
{
	//GEngine->AddOnScreenDebugMessage(-1, 10.0f, FColor::Yellow, TEXT("START GRENADE ANIM!!!!!!!!"));
	PlayWeaponAnimation(GrenadeAnim);
}


////////////// NEW GRENADE STUFF

void AShooterWeapon::StartGrenadeNew()
{
	if (Role < ROLE_Authority)
	{
		ServerStartGrenadeNew();
	}

	if (!bWantsToGrenadeNew)
	{
		bWantsToGrenadeNew = true;
		DetermineWeaponState();
	}
	else
	{

	}
}

bool AShooterWeapon::ServerStartGrenadeNew_Validate()
{
	return true;
}

void AShooterWeapon::ServerStartGrenadeNew_Implementation()
{
	StartGrenadeNew();
}

void AShooterWeapon::StopGrenadeNew()
{
	if (Role < ROLE_Authority)
	{
		ServerStopGrenadeNew();
	}

	if (bWantsToGrenadeNew)
	{
		GrenadeCounter = 0;

		if (GetNetMode() != NM_DedicatedServer)
		{
			StopSimulatingWeaponGrenade();
		}

		bWantsToGrenadeNew = false;

		DetermineWeaponState();
	}
}

bool AShooterWeapon::ServerStopGrenadeNew_Validate()
{
	return true;
}

void AShooterWeapon::ServerStopGrenadeNew_Implementation()
{
	StopGrenadeNew();
}

bool AShooterWeapon::CanGrenade() const
{
	bool bCanGrenade = MyPawn && MyPawn->CanFire(); // uses pawn canfire because its just an alive check
	bool bStateOkToGrenade = ((CurrentState == EWeaponState::Idle) || (CurrentState == EWeaponState::Firing) || (CurrentState == EWeaponState::Reloading) || (CurrentState == EWeaponState::Grenading));

	return ((bCanGrenade == true) && (bStateOkToGrenade == true) && (bPendingReload == false));
}

void AShooterWeapon::OnGrenadeStarted()
{
	const float GameTime = GetWorld()->GetTimeSeconds();
	if (LastGrenadeTimeNew > 0 && TimeBetweenGrenadesNew > 0.0f &&
		LastGrenadeTimeNew + TimeBetweenGrenadesNew > GameTime)
	{
		GetWorldTimerManager().SetTimer(this, &AShooterWeapon::StopGrenadeNew, LastGrenadeTimeNew + TimeBetweenGrenadesNew - GameTime, false);
	}
	else
	{
		HandleGrenadingNew();
	}
}

void AShooterWeapon::HandleGrenadingNew()
{
	if (CanGrenade())
	{
		if (GetNetMode() != NM_DedicatedServer)
		{
			SimulateWeaponGrenade();
		}

		if (MyPawn && MyPawn->IsLocallyControlled())
		{
			//WeaponGrenade();
			GetWorldTimerManager().SetTimer(this, &AShooterWeapon::WeaponGrenade, GrenadeSpawnDelay, false);
			GrenadeCounter++;
		}
	}

	if (MyPawn && MyPawn->IsLocallyControlled())
	{
		if (Role < ROLE_Authority)
		{
			ServerHandleGrenadingNew();
		}
	}

	LastGrenadeTimeNew = GetWorld()->GetTimeSeconds();
}

bool AShooterWeapon::ServerHandleGrenadingNew_Validate()
{
	return true;
}

void AShooterWeapon::ServerHandleGrenadingNew_Implementation()
{
	HandleGrenadingNew();
}

void AShooterWeapon::WeaponGrenade()
{
	if (Cast<AShooterCharacter>(Instigator))
	{
		FVector ShootDir = GetAdjustedAim();
		FVector Origin = GetMuzzleLocation();

		// trace from camera to check what's under crosshair
		const float ProjectileAdjustRange = 10000.0f;
		const FVector StartTrace = GetCameraDamageStartLocation(ShootDir);
		const FVector EndTrace = StartTrace + ShootDir * ProjectileAdjustRange;
		FHitResult Impact = WeaponTrace(StartTrace, EndTrace);

		// and adjust directions to hit that actor
		if (Impact.bBlockingHit)
		{
			const FVector AdjustedDir = (Impact.ImpactPoint - Origin).SafeNormal();
			bool bWeaponPenetration = false;

			const float DirectionDot = FVector::DotProduct(AdjustedDir, ShootDir);
			if (DirectionDot < 0.0f)
			{
				// shoot backwards = weapon is penetrating
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
				ShootDir = AdjustedDir;
			}
		}

		Origin += Cast<AShooterCharacter>(Instigator)->GrenadeOffset;
		//PlayWeaponAnimation(GrenadeAnim);
		ServerWeaponGrenade(Origin, ShootDir);
	}

	GetWorldTimerManager().SetTimer(this, &AShooterWeapon::StopGrenadeNew, TimeBetweenGrenadesNew, false);
}

bool AShooterWeapon::ServerWeaponGrenade_Validate(FVector Origin, FVector_NetQuantizeNormal ShootDir)
{
	return true;
}

void AShooterWeapon::ServerWeaponGrenade_Implementation(FVector Origin, FVector_NetQuantizeNormal ShootDir)
{
	FTransform SpawnTM(ShootDir.Rotation(), Origin);

	FGrenadeData GrenadeData = Cast<AShooterCharacter>(MyPawn)->GetGrenadeConfig();
	
	if (GrenadeData.GrenadeClass != NULL)
	{

		AShooterProjectile* Grenade = Cast<AShooterProjectile>(UGameplayStatics::BeginSpawningActorFromClass(this, GrenadeData.GrenadeClass, SpawnTM));

		if (Grenade)
		{
			Grenade->Instigator = Instigator;
			Grenade->SetOwner(this);
			Grenade->InitVelocity(ShootDir);

			UGameplayStatics::FinishSpawningActor(Grenade, SpawnTM);
		}
	}
}

void AShooterWeapon::SimulateWeaponGrenade()
{
	//GEngine->AddOnScreenDebugMessage(-1, 10.0f, FColor::Yellow, TEXT("SIMULATEWEAPONGRENADE!!!"));
	if (Role == ROLE_Authority && CurrentState != EWeaponState::Grenading)
	{
		return;
	}


	if (!bPlayingGrenadeAnim)
	{
		PlayWeaponAnimation(GrenadeAnim);
		bPlayingGrenadeAnim = true;
	}

	/*if (bLoopedFireSound)
	{
	if (FireAC == NULL)
	{
	FireAC = PlayWeaponSound(FireLoopSound);
	}
	}
	else
	{
	PlayWeaponSound(FireSound);
	}
	*/
	/*AShooterPlayerController* PC = (MyPawn != NULL) ? Cast<AShooterPlayerController>(MyPawn->Controller) : NULL;
	if (PC != NULL && PC->IsLocalController())
	{
	if (FireCameraShake != NULL)
	{
	PC->ClientPlayCameraShake(FireCameraShake, 1);
	}
	if (FireForceFeedback != NULL)
	{
	PC->ClientPlayForceFeedback(FireForceFeedback, false, "Weapon");
	}
	}
	*/
}

void AShooterWeapon::StopSimulatingWeaponGrenade()
{

	if (bPlayingGrenadeAnim)
	{
		StopWeaponAnimation(GrenadeAnim);
		bPlayingGrenadeAnim = false;
	}

	/*if (FireAC)
	{
	FireAC->FadeOut(0.1f, 0.0f);
	FireAC = NULL;

	PlayWeaponSound(FireFinishSound);
	}
	*/
}

void AShooterWeapon::OnRep_GrenadeCounter()
{
	if (GrenadeCounter > 0)
	{
		SimulateWeaponGrenade();
	}
	else
	{
		StopSimulatingWeaponGrenade();
	}
}
////////////// New Melee Stuff Testing
void AShooterWeapon::StartMeleeNew()
{
	if (Role < ROLE_Authority)
	{
		ServerStartMeleeNew();
	}

	if (!bWantsToMeleeNew)
	{
		bWantsToMeleeNew = true;
		DetermineWeaponState();
	}
	else
	{
	}
}

bool AShooterWeapon::ServerStartMeleeNew_Validate()
{
	return true;
}

void AShooterWeapon::ServerStartMeleeNew_Implementation()
{
	StartMeleeNew();
}

void AShooterWeapon::StopMeleeNew()
{
	if (Role < ROLE_Authority)
	{
		ServerStopMeleeNew();
	}

	if (bWantsToMeleeNew)
	{
		MeleeCounter = 0;

		if (GetNetMode() != NM_DedicatedServer)
		{
			StopSimulatingWeaponMelee();
		}

		bWantsToMeleeNew = false;
		//GetPawnOwner()->bWantsToMeleeNew = false;
		DetermineWeaponState();
	}
}

bool AShooterWeapon::ServerStopMeleeNew_Validate()
{
	return true;
}

void AShooterWeapon::ServerStopMeleeNew_Implementation()
{
	StopMeleeNew();
}

bool AShooterWeapon::CanMelee() const
{
	bool bCanMelee = MyPawn && MyPawn->CanFire(); // uses CanFire() because it is just a check if the pawn is alive
	bool bStateOKToMelee = ((CurrentState == EWeaponState::Idle) || (CurrentState == EWeaponState::Firing) || (CurrentState == EWeaponState::Reloading) || (CurrentState == EWeaponState::Meleeing));

	return ((bCanMelee == true) && (bStateOKToMelee == true) && (bPendingReload == false));
}

void AShooterWeapon::OnMeleeStarted()
{
	// start firing, can be delayed to satisfy TimeBetweenShots
	const float GameTime = GetWorld()->GetTimeSeconds();
	if (LastMeleeTimeNew > 0 &&	TimeBetweenMeleesNew > 0.0f &&
		LastMeleeTimeNew + TimeBetweenMeleesNew > GameTime)
	{
		//GetWorldTimerManager().SetTimer(this, &AShooterWeapon::HandleFiring, LastMeleeTimeNew +  - GameTime, false);
		GetWorldTimerManager().SetTimer(this, &AShooterWeapon::StopMeleeNew, LastMeleeTimeNew + TimeBetweenMeleesNew - GameTime, false);
	}
	else
	{
		HandleMeleeingNew();
	}
}

void AShooterWeapon::HandleMeleeingNew()
{
	if ((CanMelee()))
	{
		if (GetNetMode() != NM_DedicatedServer)
		{
			//SimulateWeaponFire();
			SimulateWeaponMelee();
		}

		if (MyPawn && MyPawn->IsLocallyControlled())
		{
			/*FireWeapon();

			UseAmmo();


			// update firing FX on remote clients if function was called on server
			BurstCounter++;
			*/

			WeaponMelee();

			MeleeCounter++;
		}
	}

	if (MyPawn && MyPawn->IsLocallyControlled())
	{
		// local client will notify server
		if (Role < ROLE_Authority)
		{
			ServerHandleMeleeingNew();
		}
	}

	LastMeleeTimeNew = GetWorld()->GetTimeSeconds();
}

bool AShooterWeapon::ServerHandleMeleeingNew_Validate()
{
	return true;
}

void AShooterWeapon::ServerHandleMeleeingNew_Implementation()
{

	HandleMeleeingNew();

}

FVector AShooterWeapon::GetShootDir()
{
	const int32 RandomSeed = FMath::Rand();
	FRandomStream WeaponRandomStream(RandomSeed);
	const float CurrentSpread = 0;
	const float ConeHalfAngle = 0;

	const FVector AimDir = GetAdjustedAim();
	const FVector ShootDir = WeaponRandomStream.VRandCone(AimDir, ConeHalfAngle, ConeHalfAngle);

	return ShootDir;
}

void AShooterWeapon::WeaponMelee()
{
	/*
	GEngine->AddOnScreenDebugMessage(-1, 10.0f, FColor::Red, TEXT("WeaponMelee!!!"));
	
	if (Role < ROLE_Authority)
	{
		GEngine->AddOnScreenDebugMessage(-1, 10.0f, FColor::Red, TEXT("Client: Weapon Melee!!!"));
		ServerWeaponMelee();
	}

	TArray<AActor*> ActorsOverlapping;
	MeleeBoxNew->GetOverlappingActors(ActorsOverlapping);

	if (ActorsOverlapping.Num() <= 0)
	{
		GEngine->AddOnScreenDebugMessage(-1, 10.0f, FColor::Red, TEXT("THE ARRAY IS LESS THAN OR EQUAL TO 0"));
	}

	for (int32 i = 0; i < ActorsOverlapping.Num(); i++)
	{
		GEngine->AddOnScreenDebugMessage(-1, 10.0f, FColor::Blue, TEXT("WE HAVE ENTERED THE FOR LOOP"));
		//Cast<AShooterCharacter>(Impact.GetActor())->EndZoom();

		if (ActorsOverlapping[i] == MyPawn)
		{
			GEngine->AddOnScreenDebugMessage(-1, 10.0f, FColor::Blue, TEXT("THIS IS US, GO TO NEXT OVERLAPPING ACTOR"));
			// WE ARE OVERLAPPING SELF, WHAT KINDA MORON WOULD HIT HIMSELF!!!!!
		}
		else if (Cast<AShooterCharacter>(ActorsOverlapping[i]))
		{
			// its a shootercharacter

			GEngine->AddOnScreenDebugMessage(-1, 10.0f, FColor::Blue, TEXT("Cast is good, we should be doing damage"));

			AShooterCharacter* MeleeTarget = Cast<AShooterCharacter>(ActorsOverlapping[i]);

			FPointDamageEvent PointDmg;
			PointDmg.DamageTypeClass = MeleeDamageType;
			PointDmg.ShotDirection = GetShootDir();

			static FName WeaponFireTag = FName(TEXT("WeaponTrace"));
			FCollisionQueryParams TraceParams(WeaponFireTag, true, Instigator);
			TraceParams.bTraceAsyncScene = true;
			TraceParams.bReturnPhysicalMaterial = true;

			const FVector StartTrace = GetActorLocation();
			const FVector EndTrace = MeleeTarget->GetActorLocation();

			GetWorld()->LineTraceSingle(PointDmg.HitInfo, StartTrace, EndTrace, COLLISION_WEAPON, TraceParams);

			PointDmg.Damage = MeleeTarget->CalculateDamageToUse(MeleeDamage, PointDmg, MyPawn->Controller, this, 0.0f, MeleeShieldDamage);

			MeleeTarget->TakeDamage(PointDmg.Damage, PointDmg, MyPawn->Controller, this);

			break;


		}
		else
		{
			// isnt a shooter character, do nothing

			GEngine->AddOnScreenDebugMessage(-1, 10.0f, FColor::Blue, TEXT("IS NOT A SHOOTER CHARACTER SAD FACE"));
		}
	}

	GetWorldTimerManager().SetTimer(this, &AShooterWeapon::StopMeleeNew, TimeBetweenMeleesNew, false);

	*/

	TArray<AActor*> ActorsOverlapping;
	MeleeBoxNew->GetOverlappingActors(ActorsOverlapping);

	if (ActorsOverlapping.Num() <= 0)
	{

	}

	for (int32 i = 0; i < ActorsOverlapping.Num(); i++)
	{

		//Cast<AShooterCharacter>(Impact.GetActor())->EndZoom();

		if (ActorsOverlapping[i] == MyPawn)
		{

			// WE ARE OVERLAPPING SELF, WHAT KINDA MORON WOULD HIT HIMSELF!!!!!
		}
		else if (Cast<AShooterCharacter>(ActorsOverlapping[i]))
		{
			// its a shootercharacter



			ServerWeaponMelee(Cast<AShooterCharacter>(ActorsOverlapping[i]));

			break;


		}
		else
		{
			// isnt a shooter character, do nothing

	
		}
	}

	GetWorldTimerManager().SetTimer(this, &AShooterWeapon::StopMeleeNew, TimeBetweenMeleesNew, false);
}

bool AShooterWeapon::ServerWeaponMelee_Validate(AShooterCharacter* TargetChar)
{
	return true;
}

void AShooterWeapon::ServerWeaponMelee_Implementation(AShooterCharacter* TargetChar)
{	
	// the damagy stuff happens here.

	//WeaponMelee();



	FPointDamageEvent PointDmg;
	PointDmg.DamageTypeClass = MeleeDamageType;
	PointDmg.ShotDirection = GetShootDir();

	static FName WeaponFireTag = FName(TEXT("WeaponTrace"));
	FCollisionQueryParams TraceParams(WeaponFireTag, true, Instigator);
	TraceParams.bTraceAsyncScene = true;
	TraceParams.bReturnPhysicalMaterial = true;

	const FVector StartTrace = GetActorLocation();
	const FVector EndTrace = TargetChar->GetActorLocation();

	GetWorld()->LineTraceSingle(PointDmg.HitInfo, StartTrace, EndTrace, COLLISION_WEAPON, TraceParams);

	PointDmg.Damage = TargetChar->CalculateDamageToUse(MeleeDamage, PointDmg, MyPawn->Controller, this, 0.0f, MeleeShieldDamage);

	TargetChar->TakeDamage(PointDmg.Damage, PointDmg, MyPawn->Controller, this);

}

void AShooterWeapon::SimulateWeaponMelee()
{
	//GEngine->AddOnScreenDebugMessage(-1, 10.0f, FColor::Yellow, TEXT("SIMULATEWEAPONMELEE!!!"));
	if (Role == ROLE_Authority && CurrentState != EWeaponState::Meleeing)
	{
		return;
	}


	if (!bPlayingMeleeAnim)
	{
		PlayWeaponAnimation(MeleeAnim);
		bPlayingMeleeAnim = true;
	}

	/*if (bLoopedFireSound)
	{
		if (FireAC == NULL)
		{
			FireAC = PlayWeaponSound(FireLoopSound);
		}
	}
	else
	{
		PlayWeaponSound(FireSound);
	}
	*/
	/*AShooterPlayerController* PC = (MyPawn != NULL) ? Cast<AShooterPlayerController>(MyPawn->Controller) : NULL;
	if (PC != NULL && PC->IsLocalController())
	{
		if (FireCameraShake != NULL)
		{
			PC->ClientPlayCameraShake(FireCameraShake, 1);
		}
		if (FireForceFeedback != NULL)
		{
			PC->ClientPlayForceFeedback(FireForceFeedback, false, "Weapon");
		}
	}
	*/
}

void AShooterWeapon::StopSimulatingWeaponMelee()
{

	if (bPlayingMeleeAnim)
	{
		StopWeaponAnimation(MeleeAnim);
		bPlayingMeleeAnim = false;
	}

	/*if (FireAC)
	{
		FireAC->FadeOut(0.1f, 0.0f);
		FireAC = NULL;

		PlayWeaponSound(FireFinishSound);
	}
	*/
}

void AShooterWeapon::OnRep_MeleeCounter()
{
	if (MeleeCounter > 0)
	{
		SimulateWeaponMelee();
	}
	else
	{
		StopSimulatingWeaponMelee();
	}
}