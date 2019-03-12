// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "ShooterGame_FreeForAll.generated.h"

UCLASS()
class AShooterGame_FreeForAll : public AShooterGameMode
{
	GENERATED_UCLASS_BODY()

protected:

	/** best player */
	UPROPERTY(transient)
	class AShooterPlayerState* WinnerPlayerState;

	/** check who won */
	virtual void DetermineMatchWinner() override;

	/** check if PlayerState is a winner */
	virtual bool IsWinner(class AShooterPlayerState* PlayerState) const override;
};
