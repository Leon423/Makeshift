// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GameFramework/Actor.h"
#include "ShooterGame/Classes/JumpPadRenderingComponent.h"
#include "JumpPad.generated.h"

/**
 * 
 */
UCLASS(Blueprintable)
class SHOOTERGAME_API AJumpPad : public AActor
{
	GENERATED_BODY()

		AJumpPad(const FObjectInitializer& ObjectInitializer);
public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = JumpPad)
		USceneComponent* SceneRoot;

	/** Static mesh for the Jump Pad */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = JumpPad)
	class UStaticMeshComponent* Mesh;

	/** The Player will Jump when overlapping this box */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = JumpPad)
	class UBoxComponent* TriggerBox;

	/** Sound to play when we jump*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = JumpPad)
		USoundBase* JumpSound;

	/** The destination of this Jump Pad */
	UPROPERTY(EditAnywhere, Category = JumpPad, BlueprintReadWrite, meta = (MakeEditWidget = ""))
		FVector JumpTarget;

	/** The length of air time it takes to reach the target */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = JumpPad, meta = (ClampMin = "0.1"))
		float JumpTime;

	/** amount of jump time during which a Pawn's movement is restricted (no air control, etc) to avoid them unintentionally missing the expected destination */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = JumpPad)
		float RestrictedJumpTime;

	/** Keep the characters XY Velocity when jumping */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = JumpPad)
		bool bMaintainVelocity;

	/** Area type for this Navigation link */
	//UPROPERTY(EditAnywhere, Category = JumpPad)
		//TSubclassOf<class UNavArea> AreaClass;

	/** Get the Jump Pad Velocity from JumpActor's location */
	UFUNCTION(BlueprintCallable, Category = JumpPad)
		FVector CalculateJumpVelocity(AActor* JumpActor);

protected:

#if WITH_EDITORONLY_DATA
	UPROPERTY()
	class UJumpPadRenderingComponent* JumpPadComp;
#endif

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	virtual void CheckForErrors() override;
#endif // WITH_EDITOR

	/** Overridden to launch PendingJumpActors */
	virtual void Tick(float DeltaTime) override;

	/** returns whether the given Actor can be launched by this jumppad */
	UFUNCTION(BlueprintNativeEvent)
		bool CanLaunch(AActor* TestActor);
	/** Launches the actor */
	UFUNCTION(BlueprintNativeEvent)
		void Launch(AActor* Actor);

	/** Actors we want to Jump next tick */
	TArray<AActor*> PendingJumpActors;

	/** Event when this actor overlaps another actor. */
	virtual void ReceiveActorBeginOverlap(class AActor* OtherActor) override;
	
};
