// Fill out your copyright notice in the Description page of Project Settings.

#include "ShooterGame.h"
#include "ShooterGame/Classes/Pickups/PickupSpawner.h"


APickupSpawner::APickupSpawner(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	EditorMesh = ObjectInitializer.CreateDefaultSubobject<UStaticMeshComponent>(this, TEXT("EditorMesh"));
	RootComponent = EditorMesh;
}

