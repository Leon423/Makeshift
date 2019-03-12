// Fill out your copyright notice in the Description page of Project Settings.

#include "ShooterGame/Public/ShooterGame.h"
#include "ShooterGame/Classes/JumpPad.h"
#include "ShooterGame/Classes/JumpPadRenderingComponent.h"

class JumpPadRenderingProxy : public FPrimitiveSceneProxy
{
private:
	FVector JumpPadLocation;
	FVector JumpPadTarget;
	FVector JumpVelocity;
	float JumpTime;
	float GravityZ;

public:

	JumpPadRenderingProxy(const UPrimitiveComponent* InComponent) : FPrimitiveSceneProxy(InComponent)
	{
		AJumpPad *JumpPad = Cast<AJumpPad>(InComponent->GetOwner());
		
		if (JumpPad != NULL)
		{
			JumpPadLocation = InComponent->GetOwner()->GetActorLocation();
			JumpVelocity = JumpPad->CalculateJumpVelocity(JumpPad);
			JumpTime = JumpPad->JumpTime;
			GravityZ = JumpPad->GetWorld()->GetGravityZ();
			JumpPadTarget = JumpPad->JumpTarget;
		}
	}

	virtual void GetDynamicMeshElements(const TArray<const FSceneView*>& Views, const FSceneViewFamily& ViewFamily, uint32 VisibilityMap, FMeshElementCollector& Collector) const override
	{
		for (int32 ViewIndex = 0; ViewIndex < Views.Num(); ViewIndex++)
		{
			if (VisibilityMap & (1 << ViewIndex))
			{
				FPrimitiveDrawInterface* PDI = Collector.GetPDI(ViewIndex);

				static const float LINE_THICKNESS = 20;
				static const int32 NUM_DRAW_LINES = 16;
				static const float FALL_DAMAGE_VELOCITY = 2400.0f;  // TODO: Swap this hard coded number for fall damage stuff once thats added

				FVector Start = JumpPadLocation;
				float TimeTick = JumpTime / NUM_DRAW_LINES;

				for (int32 i = 1; i < NUM_DRAW_LINES; i++)
				{
					// find position in the trajectory
					float TimeElapsed = TimeTick * i;
					FVector End = JumpPadLocation + (JumpVelocity * TimeElapsed);
					End.Z -= (-GravityZ * FMath::Pow(TimeElapsed, 2)) / 2;

					// Color gradient to show how fast the player will be travelling. useful to visualize fall damage
					float speed = FMath::Clamp(FMath::Abs(Start.Z - End.Z) / TimeTick / FALL_DAMAGE_VELOCITY, 0.0f, 1.0f);
					FColor LineClr = FColor::MakeRedToGreenColorFromScalar(1 - speed);

					// draw and swap line ends
					PDI->DrawLine(Start, End, LineClr, 0, LINE_THICKNESS);
					Start = End;
				}
			}
		}
	}

	virtual uint32 GetMemoryFootprint(void) const
	{
		return(sizeof(*this));
	}

	FPrimitiveViewRelevance GetViewRelevance(const FSceneView* View)
	{
		FPrimitiveViewRelevance Result;
		Result.bDrawRelevance = IsShown(View) && (IsSelected() || View->Family->EngineShowFlags.Navigation);
		Result.bDynamicRelevance = true;
		Result.bNormalTranslucencyRelevance = IsShown(View);
		Result.bShadowRelevance = IsShadowCast(View);
		Result.bEditorPrimitiveRelevance = UseEditorCompositing(View);
		return Result;
	}
};

UJumpPadRenderingComponent::UJumpPadRenderingComponent(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	// allows updting in game, while optimizing rendering for the case that it is not modified
	Mobility = EComponentMobility::Stationary;

	BodyInstance.SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);

	AlwaysLoadOnClient = false;
	AlwaysLoadOnServer = false;
	bHiddenInGame = true;
	bGenerateOverlapEvents = false;
}

FBoxSphereBounds UJumpPadRenderingComponent::CalcBounds(const FTransform& LocalToWorld) const
{
	FBox Bounds(0);

	/*
	AJumpPad *JumpPad = Cast<AJumpPad>(GetOwner());
	
	if (JumpPad != NULL)
	{
		FVector JumpPadLocation = JumpPad->GetActorLocation();
		FVector JumpPadTarget = JumpPad->JumpTarget;
		FVector JumpVelocity = JumpPad->CalculateJumpVelocity(JumpPad);
		float JumpTime = JumpPad->JumpTime;
		float GravityZ = -JumpPad->GetWorld()->GetGravityZ();

		Bounds += JumpPadLocation;
		Bounds += JumpPad->ActorToWorld().TransformPosition(JumpPadTarget);

		//Guard divide by zero potential with gravity
		if (JumpPad->GetWorld()->GetGravityZ() != 0.0f)
		{
			//If the apex of the jump is within the Pad and destination add to the bounds
			float ApexTime = JumpVelocity.Z / GravityZ;
			if (ApexTime > 0.0f && ApexTime < JumpTime)
			{
				FVector Apex = JumpPadLocation + (JumpVelocity * ApexTime);
				Apex.Z -= (GravityZ * FMath::Pow(ApexTime, 2)) / 2;
				Bounds += Apex;
			}
		}
	}
	*/
	return FBoxSphereBounds(Bounds);
	
}

FPrimitiveSceneProxy* UJumpPadRenderingComponent::CreateSceneProxy()
{
	return new JumpPadRenderingProxy(this);
}

