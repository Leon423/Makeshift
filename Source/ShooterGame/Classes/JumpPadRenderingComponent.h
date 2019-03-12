// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

//#include "Components/PrimitiveComponent.h"
#include "JumpPadRenderingComponent.generated.h"

/**
 * 
 */
UCLASS()
class SHOOTERGAME_API UJumpPadRenderingComponent : public UPrimitiveComponent
{
	GENERATED_BODY()
	
		UJumpPadRenderingComponent(const FObjectInitializer& ObjectInitializer);

		// Begin UPrimitiveComponent Interface
		virtual FPrimitiveSceneProxy* CreateSceneProxy() override;

	/** Should recreate proxy one very update */
	virtual bool ShouldRecreateProxyOnUpdateTransform() const override{ return true; }
	// End UPrimitiveComponent Interface

	// Begin USceneComponent Interface
	virtual FBoxSphereBounds CalcBounds(const FTransform &LocalToWorld) const override;
	// End USceneComponent Interface
	
	
};
