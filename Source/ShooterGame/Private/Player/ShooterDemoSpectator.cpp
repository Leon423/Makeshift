// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "ShooterGame.h"
#include "UI/Menu/ShooterDemoPlaybackMenu.h"

AShooterDemoSpectator::AShooterDemoSpectator(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
}

void AShooterDemoSpectator::SetupInputComponent()
{
	Super::SetupInputComponent();

	// UI input
	InputComponent->BindAction( "InGameMenu", IE_Pressed, this, &AShooterDemoSpectator::OnToggleInGameMenu );
}

void AShooterDemoSpectator::SetPlayer( UPlayer* InPlayer )
{
	Super::SetPlayer( InPlayer );

	// Build menu only after game is initialized
	ShooterDemoPlaybackMenu = MakeShareable( new FShooterDemoPlaybackMenu() );
	ShooterDemoPlaybackMenu->Construct( Cast< ULocalPlayer >( Player ) );
}

void AShooterDemoSpectator::OnToggleInGameMenu()
{
	// if no one's paused, pause
	if ( ShooterDemoPlaybackMenu.IsValid() )
	{
		ShooterDemoPlaybackMenu->ToggleGameMenu();
	}
}
