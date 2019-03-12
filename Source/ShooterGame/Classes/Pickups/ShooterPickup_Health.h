// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "ShooterPickup_Health.generated.h"

// A pickup object that replenishes character health
UCLASS(Abstract, Blueprintable)
class AShooterPickup_Health : public AShooterPickup
{
	GENERATED_UCLASS_BODY()

	/** check if pawn can use this pickup */
	virtual bool CanBePickedUp(class AShooterCharacter* TestPawn) const override;

protected:

	/** how much health does it give? */
	UPROPERTY(EditDefaultsOnly, Category=Pickup)
	int32 Health;

	/** give pickup */
	virtual void GivePickupTo(class AShooterCharacter* Pawn) override;
};
