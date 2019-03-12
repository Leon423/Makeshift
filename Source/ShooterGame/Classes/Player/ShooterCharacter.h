// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "ShooterTypes.h"
#include "ShooterCharacter.generated.h"

// ADDED THIS
USTRUCT()
struct FGrenadeData
{
	GENERATED_USTRUCT_BODY()

		/** Grenade Projectile Class */
		UPROPERTY(EditDefaultsOnly, Category = Grenades)
		TSubclassOf<class AShooterProjectile> GrenadeClass;

	/** Damage of Grenade at Impact Point */
	UPROPERTY(EditDefaultsOnly, Category = Grenades)
		int32 GrenadeExplosionDamage;

	/** Radius of Damage */
	UPROPERTY(EditDefaultsOnly, Category = Grenades)
		float GrenadeExplosionRadius;

	/** type of damage */
	UPROPERTY(EditDefaultsOnly, Category = Grenades)
		TSubclassOf<UDamageType> GrenadeDamageType;

	/** defaults */

	FGrenadeData()
	{
		GrenadeClass = NULL;
		GrenadeExplosionDamage = 100;
		GrenadeExplosionRadius = 300.0f;
		GrenadeDamageType = UDamageType::StaticClass();
	}
};

UCLASS(Abstract)
class AShooterCharacter : public ACharacter
{
	GENERATED_UCLASS_BODY()

	/** spawn inventory, setup initial variables */
	virtual void PostInitializeComponents() override;

	/** Update the character. (Running, health etc). */
	virtual void Tick(float DeltaSeconds) override;

	/** cleanup inventory */
	virtual void Destroyed() override;

	/** update mesh for first person view */
	virtual void PawnClientRestart() override;

	/** [server] perform PlayerState related setup */
	virtual void PossessedBy(class AController* C) override;

	/** [client] perform PlayerState related setup */
	virtual void OnRep_PlayerState() override;

	/** 
	 * Add camera pitch to first person mesh.
	 *
	 *	@param	CameraLocation	Location of the Camera.
	 *	@param	CameraRotation	Rotation of the Camera.
	 */
	void OnCameraUpdate(const FVector& CameraLocation, const FRotator& CameraRotation);

	/** get aim offsets */
	UFUNCTION(BlueprintCallable, Category="Game|Weapon")
	FRotator GetAimOffsets() const;

	/** 
	 * Check if pawn is enemy if given controller.
	 *
	 * @param	TestPC	Controller to check against.
	 */
	bool IsEnemyFor(AController* TestPC) const;

	//////////////////////////////////////////////////////////////////////////
	// Inventory

	/** 
	 * [server] add weapon to inventory
	 *
	 * @param Weapon	Weapon to add.
	 */
	void AddWeapon(class AShooterWeapon* Weapon);

	/** 
	 * [server] remove weapon from inventory 
	 *
	 * @param Weapon	Weapon to remove.
	 */	
	void RemoveWeapon(class AShooterWeapon* Weapon);

	/** 
	 * Find in inventory
	 *
	 * @param WeaponClass	Class of weapon to find.
	 */
	class AShooterWeapon* FindWeapon(TSubclassOf<class AShooterWeapon> WeaponClass);

	/** 
	 * [server + local] equips weapon from inventory 
	 *
	 * @param Weapon	Weapon to equip
	 */
	void EquipWeapon(class AShooterWeapon* Weapon);

	//////////////////////////////////////////////////////////////////////////
	// Weapon usage

	/** [local] starts weapon fire */
	void StartWeaponFire();

	/** [local] stops weapon fire */
	void StopWeaponFire();

	/** check if pawn can fire weapon */
	bool CanFire() const;

	/** check if pawn can reload weapon */
	bool CanReload() const;

	/** [server + local] change targeting state */
	void SetTargeting(bool bNewTargeting);

	//////////////////////////////////////////////////////////////////////////
	// Movement

	/** [server + local] change running state */
	void SetRunning(bool bNewRunning, bool bToggle);
	
	//////////////////////////////////////////////////////////////////////////
	// Animations
	
	/** play anim montage */
	virtual float PlayAnimMontage(class UAnimMontage* AnimMontage, float InPlayRate = 1.f, FName StartSectionName = NAME_None) override;

	/** stop playing montage */
	virtual void StopAnimMontage(class UAnimMontage* AnimMontage) override;

	/** stop playing all montages */
	void StopAllAnimMontages();

	//////////////////////////////////////////////////////////////////////////
	// Input handlers

	/** setup pawn specific input handlers */
	virtual void SetupPlayerInputComponent(class UInputComponent* InputComponent) override;

	/** 
	 * Move forward/back
	 *
	 * @param Val Movment input to apply
	 */
	void MoveForward(float Val);

	/** 
	 * Strafe right/left 
	 *
	 * @param Val Movment input to apply
	 */	
	void MoveRight(float Val);

	/** 
	 * Move Up/Down in allowed movement modes. 
	 *
	 * @param Val Movment input to apply
	 */	
	void MoveUp(float Val);

	/* Frame rate independent turn */
	void TurnAtRate(float Val);

	/* Frame rate independent lookup */
	void LookUpAtRate(float Val);

	/** player pressed start fire action */
	void OnStartFire();

	/** player released start fire action */
	void OnStopFire();

	/** player pressed targeting action */
	void OnStartTargeting();

	/** player released targeting action */
	void OnStopTargeting();

	/** player pressed next weapon action */
	void OnNextWeapon();

	/** player pressed prev weapon action */
	void OnPrevWeapon();

	/** player pressed reload action */
	void OnReload();

	/** player pressed jump action */
	void OnStartJump();

	/** player released jump action */
	void OnStopJump();

	/** player pressed run action */
	void OnStartRunning();

	/** player pressed toggled run action */
	void OnStartRunningToggle();

	/** player released run action */
	void OnStopRunning();

	//////////////////////////////////////////////////////////////////////////
	// Reading data

	/** get mesh component */
	USkeletalMeshComponent* GetPawnMesh() const;

	/** get currently equipped weapon */
	UFUNCTION(BlueprintCallable, Category="Game|Weapon")
	class AShooterWeapon* GetWeapon() const;

	/** get weapon attach point */
	FName GetWeaponAttachPoint() const;

	/** get total number of inventory items */
	int32 GetInventoryCount() const;

	/** 
	 * get weapon from inventory at index. Index validity is not checked.
	 *
	 * @param Index Inventory index
	 */
	class AShooterWeapon* GetInventoryWeapon(int32 index) const;

	/** get weapon taget modifier speed	*/
	UFUNCTION(BlueprintCallable, Category="Game|Weapon")
	float GetTargetingSpeedModifier() const;

	/** get targeting state */
	UFUNCTION(BlueprintCallable, Category="Game|Weapon")
	bool IsTargeting() const;

	/** get firing state */
	UFUNCTION(BlueprintCallable, Category="Game|Weapon")
	bool IsFiring() const;

	/** get the modifier value for running speed */
	UFUNCTION(BlueprintCallable, Category=Pawn)
	float GetRunningSpeedModifier() const;

	/** get running state */
	UFUNCTION(BlueprintCallable, Category=Pawn)
	bool IsRunning() const;

	/** get camera view type */
	UFUNCTION(BlueprintCallable, Category=Mesh)
	virtual bool IsFirstPerson() const;

	/** get max health */
	int32 GetMaxHealth() const;

	/** check if pawn is still alive */
	bool IsAlive() const;

	/** returns percentage of health when low health effects should start */
	float GetLowHealthPercentage() const;

	/*
 	 * Get either first or third person mesh. 
	 *
 	 * @param	WantFirstPerson		If true returns the first peron mesh, else returns the third
	 */
	USkeletalMeshComponent* GetSpecifcPawnMesh( bool WantFirstPerson ) const;

	/** Update the team color of all player meshes. */
	void UpdateTeamColorsAllMIDs();
private:

	/** pawn mesh: 1st person view */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=Mesh, META=(AllowPrivateAccess="true"))
	USkeletalMeshComponent* Mesh1P;
protected:

	/** socket or bone name for attaching weapon mesh */
	UPROPERTY(EditDefaultsOnly, Category=Inventory)
	FName WeaponAttachPoint;

	/** default inventory list */
	UPROPERTY(EditDefaultsOnly, Category=Inventory)
	TArray<TSubclassOf<class AShooterWeapon> > DefaultInventoryClasses;

	/** weapons in inventory */
	UPROPERTY(Transient, Replicated)
	TArray<class AShooterWeapon*> Inventory;

	/** currently equipped weapon */
	UPROPERTY(Transient, ReplicatedUsing=OnRep_CurrentWeapon)
	class AShooterWeapon* CurrentWeapon;

	/** Replicate where this pawn was last hit and damaged */
	UPROPERTY(Transient, ReplicatedUsing=OnRep_LastTakeHitInfo)
	struct FTakeHitInfo LastTakeHitInfo;

	/** Time at which point the last take hit info for the actor times out and won't be replicated; Used to stop join-in-progress effects all over the screen */
	float LastTakeHitTimeTimeout;

	/** modifier for max movement speed */
	UPROPERTY(EditDefaultsOnly, Category=Inventory)
	float TargetingSpeedModifier;

	/** current targeting state */
	UPROPERTY(Transient, Replicated)
	uint8 bIsTargeting : 1;

	/** modifier for max movement speed */
	UPROPERTY(EditDefaultsOnly, Category=Pawn)
	float RunningSpeedModifier;

	/** current running state */
	UPROPERTY(Transient, Replicated)
	uint8 bWantsToRun : 1;

	/** from gamepad running is toggled */
	uint8 bWantsToRunToggled : 1;

	/** current firing state */
	uint8 bWantsToFire : 1;

	/** when low health effects should start */
	float LowHealthPercentage;

	/** Base turn rate, in deg/sec. Other scaling may affect final turn rate. */
	float BaseTurnRate;

	/** Base lookup rate, in deg/sec. Other scaling may affect final lookup rate. */
	float BaseLookUpRate;

	/** material instances for setting team color in mesh (3rd person view) */
	UPROPERTY(Transient, BlueprintReadOnly)
	TArray<UMaterialInstanceDynamic*> MeshMIDs;

	/** animation played on death */
	UPROPERTY(EditDefaultsOnly, Category=Animation)
	UAnimMontage* DeathAnim;

	/** sound played on death, local player only */
	UPROPERTY(EditDefaultsOnly, Category=Pawn)
	USoundCue* DeathSound;

	/** effect played on respawn */
	UPROPERTY(EditDefaultsOnly, Category=Pawn)
	UParticleSystem* RespawnFX;

	/** sound played on respawn */
	UPROPERTY(EditDefaultsOnly, Category=Pawn)
	USoundCue* RespawnSound;

	/** sound played when health is low */
	UPROPERTY(EditDefaultsOnly, Category=Pawn)
	USoundCue* LowHealthSound;

	/** sound played when running */
	UPROPERTY(EditDefaultsOnly, Category=Pawn)
	USoundCue* RunLoopSound;

	/** sound played when stop running */
	UPROPERTY(EditDefaultsOnly, Category=Pawn)
	USoundCue* RunStopSound;

	/** sound played when targeting state changes */
	UPROPERTY(EditDefaultsOnly, Category=Pawn)
	USoundCue* TargetingSound;

	/** used to manipulate with run loop sound */
	UPROPERTY()
	UAudioComponent* RunLoopAC;

	/** hook to looped low health sound used to stop/adjust volume */
	UPROPERTY()
	UAudioComponent* LowHealthWarningPlayer;

	/** handles sounds for running */
	void UpdateRunSounds(bool bNewRunning);

	/** handle mesh visibility and updates */
	void UpdatePawnMeshes();

	/** handle mesh colors on specified material instance */
	void UpdateTeamColors(UMaterialInstanceDynamic* UseMID);

	/** Responsible for cleaning up bodies on clients. */
	virtual void TornOff();

	//////////////////////////////////////////////////////////////////////////
	// Damage & death

public:

	/** Identifies if pawn is in its dying state */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=Health)
	uint32 bIsDying:1;

	// Current health of the Pawn
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated, Category=Health)
	float Health;

	/** Take damage, handle death */
	virtual float TakeDamage(float Damage, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, class AActor* DamageCauser) override;

	/** Pawn suicide */
	virtual void Suicide();

	/** Kill this pawn */
	virtual void KilledBy(class APawn* EventInstigator);

	/** Returns True if the pawn can die in the current state */
	virtual bool CanDie(float KillingDamage, FDamageEvent const& DamageEvent, AController* Killer, AActor* DamageCauser) const;

	/**
	 * Kills pawn.  Server/authority only.
	 * @param KillingDamage - Damage amount of the killing blow
	 * @param DamageEvent - Damage event of the killing blow
	 * @param Killer - Who killed this pawn
	 * @param DamageCauser - the Actor that directly caused the damage (i.e. the Projectile that exploded, the Weapon that fired, etc)
	 * @returns true if allowed
	 */
	virtual bool Die(float KillingDamage, struct FDamageEvent const& DamageEvent, class AController* Killer, class AActor* DamageCauser);

	// Die when we fall out of the world.
	virtual void FellOutOfWorld(const class UDamageType& dmgType) override;

	/** Called on the actor right before replication occurs */
	virtual void PreReplication( IRepChangedPropertyTracker & ChangedPropertyTracker ) override;
protected:
	/** notification when killed, for both the server and client. */
	virtual void OnDeath(float KillingDamage, struct FDamageEvent const& DamageEvent, class APawn* InstigatingPawn, class AActor* DamageCauser);

	/** play effects on hit */
	virtual void PlayHit(float DamageTaken, struct FDamageEvent const& DamageEvent, class APawn* PawnInstigator, class AActor* DamageCauser);

	/** switch to ragdoll */
	void SetRagdollPhysics();

	/** sets up the replication for taking a hit */
	void ReplicateHit(float Damage, struct FDamageEvent const& DamageEvent, class APawn* InstigatingPawn, class AActor* DamageCauser, bool bKilled);

	/** play hit or death on client */
	UFUNCTION()
	void OnRep_LastTakeHitInfo();

	//////////////////////////////////////////////////////////////////////////
	// Inventory

	/** updates current weapon */
	void SetCurrentWeapon(class AShooterWeapon* NewWeapon, class AShooterWeapon* LastWeapon = NULL);

	/** current weapon rep handler */
	UFUNCTION()
	void OnRep_CurrentWeapon(class AShooterWeapon* LastWeapon);

	/** [server] spawns default inventory */
	void SpawnDefaultInventory();

	/** [server] remove all weapons from inventory and destroy them */
	void DestroyInventory();

	/** equip weapon */
	UFUNCTION(reliable, server, WithValidation)
	void ServerEquipWeapon(class AShooterWeapon* NewWeapon);

	/** update targeting state */
	UFUNCTION(reliable, server, WithValidation)
	void ServerSetTargeting(bool bNewTargeting);
	
	/** update targeting state */
	UFUNCTION(reliable, server, WithValidation)
	void ServerSetRunning(bool bNewRunning, bool bToggle);

protected:
	/** Returns Mesh1P subobject **/
	FORCEINLINE USkeletalMeshComponent* GetMesh1P() const { return Mesh1P; }

	///////////////// ADDED EVERYTHING BELOW THIS:::::::::::::
public:
	/** I ADDED THIS: Camera Component */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera)
		UCameraComponent* FirstPersonCameraComponent;

	/** Whether or not you can ADS */
	UPROPERTY(EditDefaultsOnly, Category = Inventory)
		bool bTargetingAllowed;

	/** Whether this pawn is allowed to run */
	UPROPERTY(EditDefaultsOnly, Category = Pawn)
		bool bRunningAllowed;
public:
	/** Take Headshot damage */
	virtual void TakeHeadshotDamage(float Damage, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, class AActor* DamageCause, float HeadshotDamage);

	/** Calculate Which Damage To Use */
	virtual float CalculateDamageToUse(float Damage, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, class AActor* DamageCause, float HeadshotDamage, float ShieldDamage);
protected:
	UPROPERTY(VisibleDefaultsOnly, Category = Shields)
		UParticleSystemComponent* ShieldBreakParticleComp;

	UPROPERTY(BlueprintReadWrite, Category = Shields)
		bool bShieldsBroke;

	//////////////////////////////////////////////////////////////////////////
	// ZOOMING
public:
	void StartZoom();

	void EndZoom();

	void PerformZoom(int32 ZoomLevel);

	UFUNCTION(reliable, client)
		void ClientEndZoom();

	void UpdateZoom(float DeltaTime);

	//////////////////////////////////////////////////////////////////////////
	// SHIELDS
public:
	/** Whether or not this character has shields. */
	UPROPERTY(EditDefaultsOnly, Category = Shields)
		bool bHasShields;
	/** The Current amount of shields this character has. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Replicated, Category = Shields)
		float CurrentShieldAmount;

	/** The rate at which the shield recovers */
	UPROPERTY(EditDefaultsOnly, Category = Shields)
		float ShieldRecoverRate;
	/** The maximum amount of shields */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = Shields)
		float MaxShieldAmount;
	/** The delay between taking damage and shields recovering */
	UPROPERTY(EditDefaultsOnly, Category = Shields)
		float ShieldRecoverDelay;
	/** The total amount of time, in seconds, it takes to recover shields if currentshieldamount equals zero */
	UPROPERTY(EditDefaultsOnly, Category = Shields)
		float TotalRechargeTime;
protected:
	void RechargeShield();

	void CalculateShieldChargeTime();

	//////////////////////////////////////////////////////////////////////////
	// MELEE

	void StartMelee();

	void StopMelee();

	bool bWantsToMelee;

	void OnStartMelee();

	void OnStopMelee();

	//////////////////////////////////////////////////////////////////////////
	// Crouch
	UFUNCTION(BlueprintCallable, Category = Crouching)
		void MyStartCrouch();
	UFUNCTION(BlueprintCallable, Category = Crouching)
		void MyStopCrouch();
	UFUNCTION(BlueprintCallable, Category = Crouching)
		void MyRecalculateBaseEyeHeight();

	virtual void OnStartCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust) override;

	virtual void OnEndCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust) override;

	UPROPERTY(EditDefaultsOnly, Category = Crouching)
		float CrouchJumpMeshOffsetZ;

	UPROPERTY(EditDefaultsOnly, Category = Crouching)
		float CrouchCameraOffsetZ;

	UPROPERTY(EditDefaultsOnly, Category = Crouching)
		bool bCrouchCameraOffsetOnlyInAir;


	/////////////////////////////////////////////////////////////////////////
	// Grenades
	UPROPERTY(EditDefaultsOnly, Category = Grenades)
		UAnimMontage* GrenadeThrowAnim1P;
	UPROPERTY(EditDefaultsOnly, Category = Grenades)
		UAnimMontage* GrenadeThrowAnim3P;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Grenades)
		int32 Grenades;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Grenades)
		int32 MaxGrenades;
	UPROPERTY(EditDefaultsOnly, Category = Grenades)
		float ThrowTime;
	UPROPERTY(EditDefaultsOnly, Category = Grenades)
		float SpawnGrenTime;
	UPROPERTY()
		bool bThrowingGrenade;

	UFUNCTION(BlueprintCallable, Category = Grenades)
		void StartGrenadeThrow();
	UFUNCTION()
		void GrenadeThrow();

	UFUNCTION(reliable, server, WithValidation)
		void ServerGrenadeThrow(FVector StartLocation, FVector_NetQuantizeNormal MyAim);

	UFUNCTION()
		void RefillGrenades();

	UFUNCTION()
		void PerformGrenadeThrow();

public:
	void ApplyGrenadeConfig(FGrenadeData& Data);

protected:

	/** Grenade Config */
	UPROPERTY(EditDefaultsOnly, Category = Grenades)
		FGrenadeData GrenadeConfig;
public:
	UPROPERTY(EditDefaultsOnly, Category = Grenades)
		FVector GrenadeOffset;

	FGrenadeData GetGrenadeConfig();

	//////////////////////////////////////////////////////
	///// Pickup stuff
	UPROPERTY(replicated, EditAnywhere, BlueprintReadWrite, Category = MyStuff)
		bool bCanUse;

	UFUNCTION(Blueprintcallable, Category = MyStuff)
		void MyAddWeapon(class AShooterWeapon* Weapon);

	void UseSomething();

	void UnUseSomething();

	UFUNCTION(server, reliable, WithValidation, Blueprintcallable, Category = MyStuff)
		void SetCanUse(bool b);

	UFUNCTION(server, reliable, WithValidation, Blueprintcallable, Category = MyStuff)
		void TryPowerup(class AShooterPowerup* P, float Power);

	UFUNCTION(server, reliable, WithValidation, Blueprintcallable, Category = MyStuff)
		void TryPickUp(class AShooter_Pickup* P, int32 C);

	UFUNCTION(BlueprintCallable, Category = MyStuff)
		void MyRemoveWeapon(class AShooterWeapon* Weapon);

	UFUNCTION(BlueprintImplementableEvent, meta = (FriendlyName = "Player Died: Drop Weapon"))
		virtual void PlayerDiedDropWeapon();

	UFUNCTION(server, reliable, WithValidation, BlueprintCallable, Category = MyStuff)
		void EvaluatePickup(class AShooter_Pickup* P);
	
	UFUNCTION(server, reliable, WithValidation, Category = MyStuff)
		void DropCurrentWeapon2();

	UFUNCTION(server, reliable, WithValidation, BlueprintCallable, Category = MyStuff)
		void EvaluateAddingAmmoFromPickup(class AShooter_Pickup* P);
	///////////////////////////////////////////////////////
	///// OVERSHIELD
	void CalculateOvershield();

	void ChargeOvershield();

	void DecayOvershield();

	UPROPERTY(EditDefaultsOnly, Category = Overshield)
		float OvershieldChargeTime;

	float OvershieldChargeRate;

	UPROPERTY(EditDefaultsOnly, Category = Overshield)
		float OvershieldDecayTime;

	float OvershieldDecayRate;

	UPROPERTY(replicated)
		bool bOvershieldCharging;

	UPROPERTY(replicated)
		bool bHasOvershield;
	// used to cache the power of the OS picked up for access later
	float OSPower;

	UPROPERTY(BlueprintReadOnly, replicated, Category = MyStuff)
		AShooterWeapon* PreviousWeapon;

	UFUNCTION(server, reliable, WithValidation)
		void UpdatePreviousWeapon();

	//// FOV
	UPROPERTY(EditDefaultsOnly, Category = FOV)
		float StandardFOV;

	//// Teleporter Helper stuff
	UPROPERTY(BlueprintReadWrite, Category = Teleporter)
		bool bCanTeleport;

	///// Plasma Stun
	float CachedMovementSpeedOnSpawn;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = Stun)
		bool bAffectedByPlasmaStun;
	UFUNCTION(server, reliable, WithValidation, BlueprintCallable, Category = Stun)
		void ApplyPlasmaStun(float StunPower, float StunTime);
	UFUNCTION(server, reliable, WithValidation, BlueprintCallable, Category = Stun)
		void ResetStun();




	///////////////// NEW MELEE STUFF TESTING
	/** current firing state */
	uint8 bWantsToMeleeNew : 1;

	void OnStartMeleeNew();

	/** [local] starts weapon melee */
	void StartWeaponMelee();

	////////////// NEW GRENADE STUFF

	uint8 bWantsToGrenade : 1;

	void OnStartGrenadeNew();

	/** [local] start weapon grenade throw */
	void StartGrenadeThrowNew();
};
	

