// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "ShooterWeapon.generated.h"

// CHANGED STUFF IN THIS
namespace EWeaponState
{
	enum Type
	{
		Idle,
		Firing,
		Reloading,
		Equipping,
		Meleeing,
		Charging,
		Grenading,
	};
}



// CHANGED STUFF IN THIS
USTRUCT()
struct FWeaponData
{
	GENERATED_USTRUCT_BODY()

	/** inifite ammo for reloads */
	UPROPERTY(EditDefaultsOnly, Category=Ammo)
	bool bInfiniteAmmo;

	/** infinite ammo in clip, no reload required */
	UPROPERTY(EditDefaultsOnly, Category=Ammo)
	bool bInfiniteClip;

	/** max ammo */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category=Ammo)
	int32 MaxAmmo;

	/** clip size */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = Ammo)
	int32 AmmoPerClip;

	/** initial clips */
	UPROPERTY(EditDefaultsOnly, Category=Ammo)
	int32 InitialClips;

	/** time between two consecutive shots */
	UPROPERTY(EditDefaultsOnly, Category=WeaponStat)
	float TimeBetweenShots;

	/** failsafe reload duration if weapon doesn't have any animation for it */
	UPROPERTY(EditDefaultsOnly, Category=WeaponStat)
	float NoAnimReloadDuration;

	/** defaults */
	FWeaponData()
	{
		bInfiniteAmmo = false;
		bInfiniteClip = false;
		MaxAmmo = 100;
		AmmoPerClip = 20;
		InitialClips = 4;
		TimeBetweenShots = 0.2f;
		NoAnimReloadDuration = 1.0f;
	}
};

USTRUCT()
struct FWeaponAnim
{
	GENERATED_USTRUCT_BODY()

	/** animation played on pawn (1st person view) */
	UPROPERTY(EditDefaultsOnly, Category=Animation)
	UAnimMontage* Pawn1P;

	/** animation played on pawn (3rd person view) */
	UPROPERTY(EditDefaultsOnly, Category=Animation)
	UAnimMontage* Pawn3P;
};

UCLASS(Abstract, Blueprintable)
class AShooterWeapon : public AActor
{
	GENERATED_UCLASS_BODY()

	/** perform initial setup */
	virtual void PostInitializeComponents() override;

	virtual void Destroyed() override;

	//////////////////////////////////////////////////////////////////////////
	// Ammo
	
	enum class EAmmoType
	{
		EBullet,
		ERocket,
		EMax,
	};

	/** [server] add ammo */
	void GiveAmmo(int AddAmount);

	/** consume a bullet */
	void UseAmmo();

	/** query ammo type */
	virtual EAmmoType GetAmmoType() const
	{
		return EAmmoType::EBullet;
	}

	//////////////////////////////////////////////////////////////////////////
	// Inventory

	/** weapon is being equipped by owner pawn */
	virtual void OnEquip();

	/** weapon is now equipped by owner pawn */
	virtual void OnEquipFinished();

	/** weapon is holstered by owner pawn */
	virtual void OnUnEquip();

	/** [server] weapon was added to pawn's inventory */
	virtual void OnEnterInventory(AShooterCharacter* NewOwner);

	/** [server] weapon was removed from pawn's inventory */
	virtual void OnLeaveInventory();

	/** check if it's currently equipped */
	bool IsEquipped() const;

	/** check if mesh is already attached */
	bool IsAttachedToPawn() const;


	//////////////////////////////////////////////////////////////////////////
	// Input

	/** [local + server] start weapon fire */
	virtual void StartFire();

	/** [local + server] stop weapon fire */
	virtual void StopFire();

	/** [all] start weapon reload */
	virtual void StartReload(bool bFromReplication = false);

	/** [local + server] interrupt weapon reload */
	virtual void StopReload();

	/** [server] performs actual reload */
	virtual void ReloadWeapon();

	/** trigger reload from server */
	UFUNCTION(reliable, client)
	void ClientStartReload();


	//////////////////////////////////////////////////////////////////////////
	// Control

	/** check if weapon can fire */
	bool CanFire() const;

	/** check if weapon can be reloaded */
	bool CanReload() const;


	//////////////////////////////////////////////////////////////////////////
	// Reading data

	/** get current weapon state */
	EWeaponState::Type GetCurrentState() const;

	/** get current ammo amount (total) */
	int32 GetCurrentAmmo() const;

	/** get current ammo amount (clip) */
	int32 GetCurrentAmmoInClip() const;

	/** get clip size */
	int32 GetAmmoPerClip() const;

	/** get max ammo amount */
	int32 GetMaxAmmo() const;

	/** get weapon mesh (needs pawn owner to determine variant) */
	USkeletalMeshComponent* GetWeaponMesh() const;

	/** get pawn owner */
	UFUNCTION(BlueprintCallable, Category="Game|Weapon")
	class AShooterCharacter* GetPawnOwner() const;

	/** icon displayed on the HUD when weapon is equipped as primary */
	UPROPERTY(EditDefaultsOnly, Category=HUD)
	FCanvasIcon PrimaryIcon;

	/** icon displayed on the HUD when weapon is secondary */
	UPROPERTY(EditDefaultsOnly, Category=HUD)
	FCanvasIcon SecondaryIcon;

	/** bullet icon used to draw current clip (left side) */
	UPROPERTY(EditDefaultsOnly, Category=HUD)
	FCanvasIcon PrimaryClipIcon;

	/** bullet icon used to draw secondary clip (left side) */
	UPROPERTY(EditDefaultsOnly, Category=HUD)
	FCanvasIcon SecondaryClipIcon;

	/** how many icons to draw per clip */
	UPROPERTY(EditDefaultsOnly, Category=HUD)
	float AmmoIconsCount;

	/** defines spacing between primary ammo icons (left side) */
	UPROPERTY(EditDefaultsOnly, Category=HUD)
	int32 PrimaryClipIconOffset;

	/** defines spacing between secondary ammo icons (left side) */
	UPROPERTY(EditDefaultsOnly, Category=HUD)
	int32 SecondaryClipIconOffset;

	/** crosshair parts icons (left, top, right, bottom and center) */
	UPROPERTY(EditDefaultsOnly, Category=HUD)
	FCanvasIcon Crosshair[5];

	/** crosshair parts icons when targeting (left, top, right, bottom and center) */
	UPROPERTY(EditDefaultsOnly, Category=HUD)
	FCanvasIcon AimingCrosshair[5];

	/** only use red colored center part of aiming crosshair */
	UPROPERTY(EditDefaultsOnly, Category=HUD)
	bool UseLaserDot;

	/** false = default crosshair */
	UPROPERTY(EditDefaultsOnly, Category=HUD)
	bool UseCustomCrosshair;

	/** false = use custom one if set, otherwise default crosshair */
	UPROPERTY(EditDefaultsOnly, Category=HUD)
	bool UseCustomAimingCrosshair;

	/** true - crosshair will not be shown unless aiming with the weapon */
	UPROPERTY(EditDefaultsOnly, Category=HUD)
	bool bHideCrosshairWhileNotAiming;

	/** check if weapon has infinite ammo (include owner's cheats) */
	bool HasInfiniteAmmo() const;

	/** check if weapon has infinite clip (include owner's cheats) */
	bool HasInfiniteClip() const;

	/** set the weapon's owning pawn */
	void SetOwningPawn(AShooterCharacter* AShooterCharacter);

	/** gets last time when this weapon was switched to */
	float GetEquipStartedTime() const;

	/** gets the duration of equipping weapon*/
	float GetEquipDuration() const;

protected:

	/** pawn owner */
	UPROPERTY(Transient, ReplicatedUsing=OnRep_MyPawn)
	class AShooterCharacter* MyPawn;

	/** weapon data */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category=Config)
	FWeaponData WeaponConfig;

private:
	/** weapon mesh: 1st person view */
	UPROPERTY(VisibleDefaultsOnly, Category=Mesh)
	USkeletalMeshComponent* Mesh1P;

	/** weapon mesh: 3rd person view */
	UPROPERTY(VisibleDefaultsOnly, Category=Mesh)
	USkeletalMeshComponent* Mesh3P;
protected:

	/** firing audio (bLoopedFireSound set) */
	UPROPERTY(Transient)
	UAudioComponent* FireAC;

	/** name of bone/socket for muzzle in weapon mesh */
	UPROPERTY(EditDefaultsOnly, Category=Effects)
	FName MuzzleAttachPoint;

	/** FX for muzzle flash */
	UPROPERTY(EditDefaultsOnly, Category=Effects)
	UParticleSystem* MuzzleFX;

	/** spawned component for muzzle FX */
	UPROPERTY(Transient)
	UParticleSystemComponent* MuzzlePSC;

	/** spawned component for second muzzle FX (Needed for split screen) */
	UPROPERTY(Transient)
	UParticleSystemComponent* MuzzlePSCSecondary;

	/** camera shake on firing */
	UPROPERTY(EditDefaultsOnly, Category=Effects)
	TSubclassOf<UCameraShake> FireCameraShake;

	/** force feedback effect to play when the weapon is fired */
	UPROPERTY(EditDefaultsOnly, Category=Effects)
	UForceFeedbackEffect *FireForceFeedback;

	/** single fire sound (bLoopedFireSound not set) */
	UPROPERTY(EditDefaultsOnly, Category=Sound)
	USoundCue* FireSound;

	/** looped fire sound (bLoopedFireSound set) */
	UPROPERTY(EditDefaultsOnly, Category=Sound)
	USoundCue* FireLoopSound;

	/** finished burst sound (bLoopedFireSound set) */
	UPROPERTY(EditDefaultsOnly, Category=Sound)
	USoundCue* FireFinishSound;

	/** out of ammo sound */
	UPROPERTY(EditDefaultsOnly, Category=Sound)
	USoundCue* OutOfAmmoSound;

	/** reload sound */
	UPROPERTY(EditDefaultsOnly, Category=Sound)
	USoundCue* ReloadSound;

	/** reload animations */
	UPROPERTY(EditDefaultsOnly, Category=Animation)
	FWeaponAnim ReloadAnim;

	/** equip sound */
	UPROPERTY(EditDefaultsOnly, Category=Sound)
	USoundCue* EquipSound;

	/** equip animations */
	UPROPERTY(EditDefaultsOnly, Category=Animation)
	FWeaponAnim EquipAnim;

	/** fire animations */
	UPROPERTY(EditDefaultsOnly, Category=Animation)
	FWeaponAnim FireAnim;

	/** is muzzle FX looped? */
	UPROPERTY(EditDefaultsOnly, Category=Effects)
	uint32 bLoopedMuzzleFX : 1;

	/** is fire sound looped? */
	UPROPERTY(EditDefaultsOnly, Category=Sound)
	uint32 bLoopedFireSound : 1;

	/** is fire animation looped? */
	UPROPERTY(EditDefaultsOnly, Category=Animation)
	uint32 bLoopedFireAnim : 1;

	/** is fire animation playing? */
	uint32 bPlayingFireAnim : 1;

	/** is weapon currently equipped? */
	uint32 bIsEquipped : 1;

	/** is weapon fire active? */
	uint32 bWantsToFire : 1;

	/** is reload animation playing? */
	UPROPERTY(Transient, ReplicatedUsing=OnRep_Reload)
	uint32 bPendingReload : 1;

	/** is equip animation playing? */
	uint32 bPendingEquip : 1;

	/** weapon is refiring */
	uint32 bRefiring;

	/** current weapon state */
	EWeaponState::Type CurrentState;

	/** time of last successful weapon fire */
	float LastFireTime;

	/** last time when this weapon was switched to */
	float EquipStartedTime;

	/** how much time weapon needs to be equipped */
	float EquipDuration;

	/** current total ammo */
	UPROPERTY(BlueprintReadWrite, Transient, Replicated)
	int32 CurrentAmmo;

	/** current ammo - inside clip */
	UPROPERTY(BlueprintReadWrite, Transient, Replicated)
	int32 CurrentAmmoInClip;

	/** burst counter, used for replicating fire events to remote clients */
	UPROPERTY(Transient, ReplicatedUsing=OnRep_BurstCounter)
	int32 BurstCounter;

	//////////////////////////////////////////////////////////////////////////
	// Input - server side

	UFUNCTION(reliable, server, WithValidation)
	void ServerStartFire();

	UFUNCTION(reliable, server, WithValidation)
	void ServerStopFire();

	UFUNCTION(reliable, server, WithValidation)
	void ServerStartReload();

	UFUNCTION(reliable, server, WithValidation)
	void ServerStopReload();


	//////////////////////////////////////////////////////////////////////////
	// Replication & effects

	UFUNCTION()
	void OnRep_MyPawn();

	UFUNCTION()
	void OnRep_BurstCounter();

	UFUNCTION()
	void OnRep_Reload();

	/** Called in network play to do the cosmetic fx for firing */
	virtual void SimulateWeaponFire();

	/** Called in network play to stop cosmetic fx (e.g. for a looping shot). */
	virtual void StopSimulatingWeaponFire();


	//////////////////////////////////////////////////////////////////////////
	// Weapon usage

	/** [local] weapon specific fire implementation */
	virtual void FireWeapon() PURE_VIRTUAL(AShooterWeapon::FireWeapon,);

	/** [server] fire & update ammo */
	UFUNCTION(reliable, server, WithValidation)
	void ServerHandleFiring();

	/** [local + server] handle weapon fire */
	void HandleFiring();

	/** [local + server] firing started */
	virtual void OnBurstStarted();

	/** [local + server] firing finished */
	virtual void OnBurstFinished();

	/** update weapon state */
	void SetWeaponState(EWeaponState::Type NewState);

	/** determine current weapon state */
	void DetermineWeaponState();


	//////////////////////////////////////////////////////////////////////////
	// Inventory

	/** attaches weapon mesh to pawn's mesh */
	void AttachMeshToPawn();

	/** detaches weapon mesh from pawn */
	void DetachMeshFromPawn();


	//////////////////////////////////////////////////////////////////////////
	// Weapon usage helpers

	/** play weapon sounds */
	UAudioComponent* PlayWeaponSound(USoundCue* Sound);

	/** play weapon animations */
	float PlayWeaponAnimation(const FWeaponAnim& Animation);

	/** stop playing weapon animations */
	void StopWeaponAnimation(const FWeaponAnim& Animation);

	/** Get the aim of the weapon, allowing for adjustments to be made by the weapon */
	virtual FVector GetAdjustedAim() const;

	/** Get the aim of the camera */
	FVector GetCameraAim() const;

	/** get the originating location for camera damage */
	FVector GetCameraDamageStartLocation(const FVector& AimDir) const;

	/** get the muzzle location of the weapon */
	FVector GetMuzzleLocation() const;

	/** get direction of weapon's muzzle */
	FVector GetMuzzleDirection() const;

	/** find hit */
	FHitResult WeaponTrace(const FVector& TraceFrom, const FVector& TraceTo) const;

protected:
	/** Returns Mesh1P subobject **/
	FORCEINLINE USkeletalMeshComponent* GetMesh1P() const { return Mesh1P; }
	/** Returns Mesh3P subobject **/
	FORCEINLINE USkeletalMeshComponent* GetMesh3P() const { return Mesh3P; }

	///////////////// ADDED EVERYTHING BELOW:::::

	///////////////////////////////////////////////////////////////////
	// ZOOMING
public:
	/** The maximum amount of times one can zoom before unzooming */
	UPROPERTY(EditDefaultsOnly, Category = Zooming)
		float MaxZoomLevel;
	/** The current zoom level, as long as this isnt equal to MaxZoomLevel you can zoom. Leave this at ZERO in blueprint */
	UPROPERTY(EditDefaultsOnly, Replicated, Category = Zooming)
		float CurrentZoomLevel;
	/** This array holds the FOV that each zoom level will set. The length of the array should be equal to MaxZoomLevel. Example: If you have MaxZoomLevel as 2, this will have two elements (numbered 0 and 1) */
	UPROPERTY(EditDefaultsOnly, Category = Zooming)
		TArray<float> ZoomLevelFOV;
	/** Whether or not this weapon can zoom */
	UPROPERTY(EditDefaultsOnly, Category = Zooming)
		bool bZoomingAllowed;

	///////////////////////////////////////////////////////////////////
	// SINGLE SHOT

	/** Whether this weapon is automatic or not */
	UPROPERTY(EditDefaultsOnly, Category = Firing)
		bool bIsAutomatic;

	/////////////////////////////////////////
	// GRENADES

	UFUNCTION()
		void ThrowGrenade(FGrenadeData GrenadeData);
	UFUNCTION(reliable, server, WithValidation)
		void ServerGrenadeThrow(FVector Origin, FVector_NetQuantizeNormal ShootDir, FGrenadeData GrenadeData);

	////////////////////////////////////////
	// Pickup Integration

	UFUNCTION(Blueprintcallable, Category = Weapon)
		void PickedUp(int32 Clips);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Weapon)
		TEnumAsByte<EWeaponClassification::Type> WeaponClassification;

	UFUNCTION(Blueprintcallable, Category = Weapon)
		void AddAmmoFromPickup(int32 Bullets);

	UPROPERTY(EditDefaultsOnly, Category = Weapon)
		float HeadshotDamage;
	
	UPROPERTY(EditDefaultsOnly, Category = Weapon)
		float ShieldDamage;

	// test melee stuff
public:
	UFUNCTION()
		FVector GetShootDir();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Weapon)
		TSubclassOf<class AShooter_Pickup> PickupBP;

	// recoil stuff
	/** The amount of recoil added per shot. Setting to 0 will cause no recoil in that axis */
	UPROPERTY(EditDefaultsOnly, Category = Recoil)
		FRotator RecoilPerShot;
	/** the time elapsed before recoil resets */
	UPROPERTY(EditDefaultsOnly, Category = Recoil)
		float RecoilResetTime;
	UPROPERTY(EditDefaultsOnly, Category = Recoil)
		float ZoomRecoilMod;
	UPROPERTY(VisibleAnywhere, Category = Recoil)
		bool bShouldRecoil;
	UPROPERTY(EditDefaultsOnly, Category = Recoil)
		float TimeBeforeRecoil;
	void ActivateRecoil();

	// Weapon Trace uses this for box trace

	/** Sets the size of the box trace being done, setting to ZERO in all aspects uses the normal line trace. */
	UPROPERTY(EditDefaultsOnly, Category = Weapon)
		FVector TraceExtent;

	UPROPERTY(EditAnywhere, Category = Debug)
		bool bDrawWeaponTrace;

	FHitResult MyLineTrace(const FVector& Start, const FVector& End, ECollisionChannel TraceChannel, const struct FCollisionQueryParams& Params) const;

	UPROPERTY(BlueprintReadWrite, Category = MyUtilities)
		bool bMyCanFire;
	UPROPERTY(BlueprintReadWrite, Category = MyUtilities)
		bool bMyCanMelee;
	UPROPERTY(BlueprintReadWrite, Category = MyUtilities)
		bool bMyCanReload;

	UPROPERTY(EditDefaultsOnly, Category = Animation)
		FWeaponAnim GrenadeAnim;

	void StartGrenadeAnim();

	UPROPERTY(EditDefaultsOnly, Category = Animation)
		FWeaponAnim MeleeAnim;

	//overheat stuff, the functions are made in instant weapon and charge weapon since i originally had them there and they function differently anyway
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Overheat)
		bool bIsOverheat;
	/** the heat produced with each shot */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Overheat)
		float HeatIncrease;
	/** the rate at which heat decreases. NOTE: this sets the timer for the next decrease.
	1.0 is one second. each time the current heat is decreased by 10 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Overheat)
		float CooldownRate;
	UPROPERTY(VisibleAnywhere, replicated, BlueprintReadOnly, Category = Overheat)
		float CurrentHeat;
	UPROPERTY(VisibleAnywhere, replicated, BlueprintReadOnly, Category = Overheat)
		bool bIsOverheated;

	//////////////////// PLASMA STUN
	UPROPERTY(EditDefaultsOnly, category = Stun)
		bool bHasPlasmaStun;
	/** The percentage of movement taken away per shot. Example: If a character moves at 100 speed and stun amount is .5. After being hit with one bullet he will move at 50. A second bullet will make him move at 25 and so on. */
	UPROPERTY(EditDefaultsOnly, category = Stun)
		float StunAmount;
	/** The amount of time before movement speed resets.*/
	UPROPERTY(EditDefaultsOnly, category = Stun)
		float StunTime;





	///////// New Melee Stuff Testing
	/** [local + server] start weapon melee */
	virtual void StartMeleeNew();

	UFUNCTION(reliable, server, WithValidation)
		void ServerStartMeleeNew();

	/** [local + server] start weapon melee */
	virtual void StopMeleeNew();

	UFUNCTION(reliable, server, WithValidation)
		void ServerStopMeleeNew();

	/** is weapon melee active? */
	uint32 bWantsToMeleeNew : 1;

	/** check if weapon can melee */
	bool CanMelee() const;

	/** [local + server] melee started */
	virtual void OnMeleeStarted();

	/** time of last successful weapon melee */
	float LastMeleeTimeNew;

	/** time between two consecutive melee */
	UPROPERTY(EditDefaultsOnly, Category = WeaponStat)
		float TimeBetweenMeleesNew;

	/** [server] fire & update ammo */
	UFUNCTION(reliable, server, WithValidation)
		void ServerHandleMeleeingNew();

	/** [local + server] handle weapon fire */
	void HandleMeleeingNew();

	/** [local] weapon specific fire implementation */
	virtual void WeaponMelee();

	/** Called in network play to do the cosmetic fx for melee */
	virtual void SimulateWeaponMelee();

	/** Called in network play to stop cosmetic fx (e.g. for a looping shot). */
	virtual void StopSimulatingWeaponMelee();

	/** is fire animation playing? */
	uint32 bPlayingMeleeAnim : 1;

	UFUNCTION(reliable, server, WithValidation)
		void ServerWeaponMelee(AShooterCharacter* TargetChar);

	/** melee counter, used for replicating events events to remote clients */
	UPROPERTY(Transient, ReplicatedUsing = OnRep_MeleeCounter)
		int32 MeleeCounter;

	UFUNCTION()
		void OnRep_MeleeCounter();

	

	UPROPERTY(EditDefaultsOnly, Category = MeleeBoxStuff)
		class UBoxComponent* MeleeBoxNew;

	UPROPERTY(EditDefaultsOnly, Category = Melee)
		float MeleeDamage;
	UPROPERTY(EditDefaultsOnly, Category = Melee)
		float MeleeShieldDamage;

	UPROPERTY(EditDefaultsOnly, Category = Melee)
		TSubclassOf<UDamageType> MeleeDamageType;

	/** grenade counter, used for replicating events events to remote clients */
	UPROPERTY(Transient, ReplicatedUsing = OnRep_GrenadeCounter)
		int32 GrenadeCounter;

	UFUNCTION()
		void OnRep_GrenadeCounter();

	/** [local + server] start weapon grenade */
	virtual void StartGrenadeNew();

	UFUNCTION(reliable, server, WithValidation)
		void ServerStartGrenadeNew();

	/** [local + server] start weapon melee */
	virtual void StopGrenadeNew();

	UFUNCTION(reliable, server, WithValidation)
		void ServerStopGrenadeNew();

	/** is weapon melee active? */
	uint32 bWantsToGrenadeNew : 1;

	/** check if weapon can melee */
	bool CanGrenade() const;

	/** [local + server] melee started */
	virtual void OnGrenadeStarted();

	/** time of last successful weapon melee */
	float LastGrenadeTimeNew;

	/** time between two consecutive melee */
	UPROPERTY(EditDefaultsOnly, Category = WeaponStat)
		float TimeBetweenGrenadesNew;

	/** [server] fire & update ammo */
	UFUNCTION(reliable, server, WithValidation)
		void ServerHandleGrenadingNew();

	/** [local + server] handle weapon fire */
	void HandleGrenadingNew();

	/** [local] weapon specific fire implementation */
	virtual void WeaponGrenade();

	/** Called in network play to do the cosmetic fx for melee */
	virtual void SimulateWeaponGrenade();

	/** Called in network play to stop cosmetic fx (e.g. for a looping shot). */
	virtual void StopSimulatingWeaponGrenade();

	/** is fire animation playing? */
	uint32 bPlayingGrenadeAnim : 1;

	UFUNCTION(reliable, server, WithValidation)
		void ServerWeaponGrenade(FVector Origin, FVector_NetQuantizeNormal ShootDir);

	UPROPERTY(EditDefaultsOnly, Category = WeaponStat)
		float GrenadeSpawnDelay;

	/////////////////////////////// Pro Pipe Stuff

	/** If the projectile should explode on trigger release check this! */
	UPROPERTY(EditDefaultsOnly, Category = ProPipe)
		bool bControlsProjectileExplosion;

	// believe i need to make this replicated!
	UPROPERTY(replicated)
	AShooterProjectile* ActiveProjectile;
};

