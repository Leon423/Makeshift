// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "ShooterPickup_Ammo.generated.h"

// A pickup object that replenishes ammunition for a weapon
UCLASS(Abstract, Blueprintable)
class AShooterPickup_Ammo : public AShooterPickup
{
	GENERATED_UCLASS_BODY()

	/** check if pawn can use this pickup */
	virtual bool CanBePickedUp(class AShooterCharacter* TestPawn) const override;

	bool IsForWeapon(UClass* WeaponClass);

protected:

	/** how much ammo does it give? */
	UPROPERTY(EditDefaultsOnly, Category=Pickup)
	int32 AmmoClips;

	/** which weapon gets ammo? */
	UPROPERTY(EditDefaultsOnly, Category=Pickup)
	TSubclassOf<class AShooterWeapon> WeaponType;

	/** give pickup */
	virtual void GivePickupTo(class AShooterCharacter* Pawn) override;
};
