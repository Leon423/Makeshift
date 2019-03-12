// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "ShooterGame.h"


//----------------------------------------------------------------------//
// UPawnMovementComponent
//----------------------------------------------------------------------//
UShooterCharacterMovement::UShooterCharacterMovement(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}


float UShooterCharacterMovement::GetMaxSpeed() const
{
	float MaxSpeed = Super::GetMaxSpeed();

	const AShooterCharacter* ShooterCharacterOwner = Cast<AShooterCharacter>(PawnOwner);
	if (ShooterCharacterOwner)
	{
		if (ShooterCharacterOwner->IsTargeting())
		{
			MaxSpeed *= ShooterCharacterOwner->GetTargetingSpeedModifier();
		}
		if (ShooterCharacterOwner->IsRunning())
		{
			MaxSpeed *= ShooterCharacterOwner->GetRunningSpeedModifier();
		}
	}

	return MaxSpeed;
}
// added this
void UShooterCharacterMovement::OnMovementModeChanged(EMovementMode PreviousMovementMode, uint8 PreviousCustomMode)
{
	if (!HasValidData())
	{
		return;
	}

	// react to changes in the movement speed
	if (MovementMode == MOVE_Walking)
	{
		// walking uses only XY velocity, and must be on a walkable floor, with a base
		Velocity.Z = 0.f;
		
		if (PreviousMovementMode == MOVE_Falling && CharacterOwner->bIsCrouched)
		{
			AShooterCharacter* DefaultCharacter = CharacterOwner->GetClass()->GetDefaultObject<AShooterCharacter>();

			const float ComponentScale = CharacterOwner->GetCapsuleComponent()->GetShapeScale();
			const float OldUnscaledHalfHeight = CharacterOwner->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight();
			
			float HalfHeightAdjust = (DefaultCharacter->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight() - CrouchedHalfHeight);
			
			 float ScaledHalfHeight = HalfHeightAdjust * ComponentScale;

			//const FVector PawnLocation = CharacterOwner->GetActorLocation();

			 ///////////////////////////////////////

			//CharacterOwner->GetMesh()->RelativeLocation.Z = GetDefault<AShooterCharacter>(GetClass())->GetMesh()->RelativeLocation.Z + HalfHeightAdjust;
			//CharacterOwner->BaseTranslationOffset.Z = CharacterOwner->GetMesh()->RelativeLocation.Z;


			CharacterOwner->OnStartCrouch(HalfHeightAdjust, ScaledHalfHeight);
		}

		bCrouchMaintainsBaseLocation = true;
		// make sure we update our new floor/base on initial entry of the walking physics
	}
	else
	{

	}

	Super::OnMovementModeChanged(PreviousMovementMode, PreviousCustomMode);
}
