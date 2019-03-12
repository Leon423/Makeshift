// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "ShooterDemoSpectator.generated.h"

UCLASS(config=Game)
class AShooterDemoSpectator : public APlayerController
{
	GENERATED_UCLASS_BODY()

public:
	/** shooter in-game menu */
	TSharedPtr<class FShooterDemoPlaybackMenu> ShooterDemoPlaybackMenu;

	virtual void SetupInputComponent() override;
	virtual void SetPlayer( UPlayer* Player ) override;

	void OnToggleInGameMenu();
};

