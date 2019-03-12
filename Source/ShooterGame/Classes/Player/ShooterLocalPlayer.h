// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "ShooterPersistentUser.h"
#include "ShooterLocalPlayer.generated.h"

UCLASS(config=Engine, transient)
class UShooterLocalPlayer : public ULocalPlayer
{
	GENERATED_UCLASS_BODY()

public:

	virtual void SetControllerId(int32 NewControllerId) override;

	virtual FString GetNickname() const;

	class UShooterPersistentUser* GetPersistentUser() const;
	
	/** Initializes the PersistentUser */
	void LoadPersistentUser();

private:
	/** Persistent user data stored between sessions (i.e. the user's savegame) */
	UPROPERTY()
	class UShooterPersistentUser* PersistentUser;

	/** @return OnlineSession class to use for this player */
	TSubclassOf<UOnlineSession> GetOnlineSessionClass() override;
};



