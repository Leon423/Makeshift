// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "Sound/SoundNode.h"
#include "SoundNodeLocalPlayer.generated.h"

/**
 * Choose different branch for sounds attached to locally controlled player
 */
UCLASS(hidecategories=Object, editinlinenew)
class USoundNodeLocalPlayer : public USoundNode
{
	GENERATED_UCLASS_BODY()

	// Begin USoundNode interface.
	virtual void ParseNodes( FAudioDevice* AudioDevice, const UPTRINT NodeWaveInstanceHash, FActiveSound& ActiveSound, const FSoundParseParameters& ParseParams, TArray<FWaveInstance*>& WaveInstances ) override;
	virtual void CreateStartingConnectors( void ) override;
	virtual int32 GetMaxChildNodes() const override;
	virtual int32 GetMinChildNodes() const override;
#if WITH_EDITOR
	virtual FString GetInputPinName(int32 PinIndex) const override;
#endif
	// End USoundNode interface.
};
