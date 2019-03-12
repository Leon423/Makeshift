// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "ShooterGame.h"

AShooterPlayerCameraManager::AShooterPlayerCameraManager(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	NormalFOV = 110.0f;
	TargetingFOV = 60.0f;
	ViewPitchMin = -87.0f;
	ViewPitchMax = 87.0f;
	bAlwaysApplyModifiers = true;
	bFirstTick = true;
}

// CHANGED STUFF IN THIS
void AShooterPlayerCameraManager::UpdateCamera(float DeltaTime)
{
	AShooterCharacter* MyPawn = PCOwner ? Cast<AShooterCharacter>(PCOwner->GetPawn()) : NULL;
	if (MyPawn && MyPawn->IsFirstPerson())
	{
		if (bFirstTick)
		{
			NormalFOV = MyPawn->StandardFOV;
			bFirstTick = false;
		}
		//const float TargetFOV = MyPawn->IsTargeting() ? TargetingFOV : NormalFOV;
		//DefaultFOV = FMath::FInterpTo(DefaultFOV, TargetFOV, DeltaTime, 20.0f);

		if (MyPawn != NULL && MyPawn->GetWeapon() != NULL)
		{

			if (MyPawn->GetWeapon()->CurrentZoomLevel != 0)
			{
				//GEngine->AddOnScreenDebugMessage(-1, 1.0f, FColor::Blue, TEXT("ZoomLevel isnt 0, should be doin stuff"));
				// we are zoomed, so we gotta do shit
				const float TargetFOV = MyPawn->GetWeapon()->ZoomLevelFOV[MyPawn->GetWeapon()->CurrentZoomLevel - 1];
				DefaultFOV = FMath::FInterpTo(DefaultFOV, TargetFOV, DeltaTime, 30.0f);
				//GEngine->AddOnScreenDebugMessage(-1, 1.f, FColor::Red, FString::Printf(TEXT("TargetFOV: %f"), TargetFOV));

			}
			else if (MyPawn->GetWeapon()->CurrentZoomLevel == 0)
			{
				//GEngine->AddOnScreenDebugMessage(-1, 1.0f, FColor::Yellow, TEXT("ZoomLevel is 0, should be normal"));
				// not zoomed so we need to do shit to get back to normal fov
				DefaultFOV = FMath::FInterpTo(DefaultFOV, NormalFOV, DeltaTime, 30.0f);
			}
			MyPawn->FirstPersonCameraComponent->FieldOfView = DefaultFOV;
		}


	}

	//MyPawn->UpdateZoom(DeltaTime);
	Super::UpdateCamera(DeltaTime);

	if (MyPawn && MyPawn->IsFirstPerson())
	{
		MyPawn->OnCameraUpdate(GetCameraLocation(), GetCameraRotation());
	}
}
