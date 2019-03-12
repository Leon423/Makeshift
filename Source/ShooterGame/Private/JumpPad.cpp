// Fill out your copyright notice in the Description page of Project Settings.

#include "ShooterGame/Public/ShooterGame.h"
#include "JumpPad.h"


AJumpPad::AJumpPad(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = true;

	SceneRoot = ObjectInitializer.CreateDefaultSubobject<USceneComponent>(this, TEXT("SceneComponent"));
	RootComponent = SceneRoot;
	RootComponent->bShouldUpdatePhysicsVolume = true;

	// setup the mesh
	Mesh = ObjectInitializer.CreateDefaultSubobject<UStaticMeshComponent>(this, TEXT("JumpPadMesh"));
	Mesh->AttachParent = RootComponent;
	Mesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);

	TriggerBox = ObjectInitializer.CreateDefaultSubobject<UBoxComponent>(this, TEXT("TriggerBox"));
	TriggerBox->SetCollisionProfileName(TEXT("Trigger"));
	TriggerBox->AttachParent = RootComponent;

	JumpSound = nullptr;
	JumpTarget = FVector(100.0f, 0.0f, 0.0f);
	JumpTime = 1.0f;
	bMaintainVelocity = false;

#if WITH_EDITORONLY_DATA
	JumpPadComp = ObjectInitializer.CreateDefaultSubobject<UJumpPadRenderingComponent>(this, TEXT("JumpPadComp"));
	JumpPadComp->PostPhysicsComponentTick.bCanEverTick = false;
	JumpPadComp->AttachParent = RootComponent;
#endif
}

void AJumpPad::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	// Launch the pending actors

	if (PendingJumpActors.Num() > 0)
	{
		for (AActor* Actor : PendingJumpActors)
		{
			Launch(Actor);
		}
		PendingJumpActors.Reset();
	}
}

bool AJumpPad::CanLaunch_Implementation(AActor* TestActor)
{
	return (Cast<ACharacter>(TestActor) != NULL && TestActor->Role >= ROLE_AutonomousProxy);
}

void AJumpPad::Launch_Implementation(AActor* Actor)
{
	// Only filtering for Characters at this time, could later make it so certain projectiles or whatever go through it too
	ACharacter* Char = Cast<ACharacter>(Actor);

	if (Char != NULL)
	{
		// launch character towards target
		Char->LaunchCharacter(CalculateJumpVelocity(Char), !bMaintainVelocity, true);

		if (RestrictedJumpTime > 0.0f)
		{

		}
	}

	// Play jump sound if we have one
	// add sound stuff here


}

void AJumpPad::ReceiveActorBeginOverlap(AActor* OtherActor)
{
	Super::ReceiveActorBeginOverlap(OtherActor);

	if (!PendingJumpActors.Contains(OtherActor) && CanLaunch(OtherActor))
	{
		PendingJumpActors.Add(OtherActor);
	}
}

FVector AJumpPad::CalculateJumpVelocity(AActor* JumpActor)
{
	FVector Target = ActorToWorld().TransformPosition(JumpTarget) - JumpActor->GetActorLocation();

	float SizeZ = Target.Z / JumpTime + 0.5f * -GetWorld()->GetGravityZ() * JumpTime;
	float SizeXY = Target.Size2D() / JumpTime;

	FVector Velocity = Target.SafeNormal2D() * SizeXY + FVector(0.0f, 0.0f, SizeZ);

	// scale velocity if character has gravity scaled
	ACharacter* Char = Cast<ACharacter>(JumpActor);
	if (Char != NULL && Char->GetCharacterMovement() != NULL && Char->GetCharacterMovement()->GravityScale != 1.0f)
	{
		Velocity *= FMath::Sqrt(Char->GetCharacterMovement()->GravityScale);
	}
	return Velocity;
}

#if WITH_EDITOR
void AJumpPad::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
}

void AJumpPad::CheckForErrors()
{
	Super::CheckForErrors();

	FVector JumpVelocity = CalculateJumpVelocity(this);

	// figure out default game mode from which we will derive the default character
	TSubclassOf<AGameMode> GameClass = GetWorld()->GetWorldSettings()->DefaultGameMode;
	if (GameClass == NULL)
	{
		// horrible config hack around unexported function UGameMapSettigns::GetGlobalDefaultGameMode()
		FString GameClassPath;
		GConfig->GetString(TEXT("Script/EngineSettings.GameMapSettings"), TEXT("GlobalDefaultGameMode"), GameClassPath, GEngineIni);
		GameClass = LoadClass<AGameMode>(NULL, *GameClassPath, NULL, 0, NULL);
	}
	const ACharacter* DefaultChar = (GameClass != NULL) ? Cast<ACharacter>(GameClass.GetDefaultObject()->DefaultPawnClass.GetDefaultObject()) : GetDefault<AShooterCharacter>();
	if (DefaultChar != NULL && DefaultChar->GetCharacterMovement() != NULL)
	{
		JumpVelocity *= FMath::Sqrt(DefaultChar->GetCharacterMovement()->GravityScale);
	}

	// check if velocity is faster than physics allow
	APhysicsVolume* PhysVolume = (RootComponent != NULL) ? RootComponent->GetPhysicsVolume() : GetWorld()->GetDefaultPhysicsVolume();
	if (JumpVelocity.Size() > PhysVolume->TerminalVelocity)
	{
		/*FFormatNamedArguments Arguments;
		Arguments.Add(TEXT("ActorName"), FText::FromString(GetName()));
		Arguments.Add(TEXT("Speed"), FText::AsNumber(JumpVelocity.Size()));
		Arguments.Add(TEXT("TerminalVelocity"), FText::AsNumer(PhysVolume->TerminalVelocity));
		FMessageLog("MapCheck").Warning()
			->AddToken(FUObjectToken::Create(this))
			->AddToken(FTextToken::Create)*/
	}
}
#endif