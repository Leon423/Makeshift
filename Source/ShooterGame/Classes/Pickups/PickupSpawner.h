// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GameFramework/Actor.h"
#include "PickupSpawner.generated.h"

/**
 * 
 */
UCLASS()
class SHOOTERGAME_API APickupSpawner : public AActor
{
	GENERATED_BODY()

	APickupSpawner(const FObjectInitializer& ObjectInitializer);
	
public:
	UPROPERTY(VisibleAnywhere, Category = MyStuff)
		UStaticMeshComponent* EditorMesh;
	
};
