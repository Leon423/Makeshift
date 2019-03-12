// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Kismet/GameplayStatics.h"
#include "MakeshiftGameplayStatics.generated.h"

/**
 * 
 */
UCLASS()
class SHOOTERGAME_API UMakeshiftGameplayStatics : public UGameplayStatics
{
	GENERATED_BODY()

		//UMakeShiftGameplayStatics(const FObjectInitializer& ObjectInitializer);
public:
		UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Game|Damage",
		Meta = (WorldContext = "WorldContextObject", AutoCreateRefTerm = "IgnoreActors"))
		static bool ApplyRadialDamageWithShieldDamage(
		UObject * WorldContextObject,
		float BaseDamage,
		const FVector & Origin,
		float DamageRadius,
		TSubclassOf< class UDamageType > DamageTypeClass,
		const TArray< AActor * > & IgnoreActors,
		AActor * DamageCauser,
		AController * InstigatedByController,
		bool bDoFullDamage,
		ECollisionChannel DamagePreventionChannel,
		float ShieldDamage
		);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Game|Damage",
		Meta = (WorldContext = "WorldContextObject", AutoCreateRefTerm = "IgnoreActors"))
		static bool ApplyRadialDamageWithFalloffWithShieldDamage
		(
		UObject * WorldContextObject,
		float BaseDamage,
		float MinimumDamage,
		const FVector & Origin,
		float DamageInnerRadius,
		float DamageOuterRadius,
		float DamageFalloff,
		TSubclassOf< class UDamageType > DamageTypeClass,
		const TArray< AActor * > & IgnoreActors,
		AActor * DamageCauser,
		AController * InstigatedByController,
		ECollisionChannel DamagePreventionChannel,
		float BaseShieldDamage,
		float MinimumShieldDamage
		);

	//static bool ComponentIsDamageableFrom(UPrimitiveComponent* VictimComp, FVector const& Origin, AActor const* IgnoredActor, const TArray<AActor*>& IgnoreActors, ECollisionChannel TraceChannel, FHitResult& OutHitResult);
	
};
