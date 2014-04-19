// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "SpaceToJump.h"
#include "SpaceToJumpCharacter.h"
#include "SpaceToJumpProjectile.h"

//////////////////////////////////////////////////////////////////////////
// ASpaceToJumpCharacter

ASpaceToJumpCharacter::ASpaceToJumpCharacter(const class FPostConstructInitializeProperties& PCIP)
: Super(PCIP)
{
	// Set size for collision capsule
	CapsuleComponent->InitCapsuleSize(42.f, 96.0f);

	// set our turn rates for input
	BaseTurnRate = 45.f;
	BaseLookUpRate = 45.f;

	// Create a CameraComponent	
	FirstPersonCameraComponent = PCIP.CreateDefaultSubobject<UCameraComponent>(this, TEXT("FirstPersonCamera"));
	FirstPersonCameraComponent->AttachParent = CapsuleComponent;
	FirstPersonCameraComponent->RelativeLocation = FVector(0, 0, 64.f); // Position the camera

	// Default offset from the character location for projectiles to spawn
	GunOffset = FVector(100.0f, 30.0f, 10.0f);

	// Create a mesh component that will be used when being viewed from a '1st person' view (when controlling this pawn)
	Mesh1P = PCIP.CreateDefaultSubobject<USkeletalMeshComponent>(this, TEXT("CharacterMesh1P"));
	Mesh1P->SetOnlyOwnerSee(true);			// only the owning player will see this mesh
	Mesh1P->AttachParent = FirstPersonCameraComponent;
	Mesh1P->RelativeLocation = FVector(0.f, 0.f, -150.f);
	Mesh1P->bCastDynamicShadow = false;
	Mesh1P->CastShadow = false;

	// Note: The ProjectileClass and the skeletal mesh/anim blueprints for Mesh1P are set in the
	// derived blueprint asset named MyCharacter (to avoid direct content references in C++)

	//Gravity stuff

	deltaTimePhyiscsBoostMultiplier = 2.f;

	gravityPower = 570.f;
	magneticFieldSize = 700.f;
	magnetCloseEnoughRange = 160.f;

}

void ASpaceToJumpCharacter::ReceiveHit(
	class UPrimitiveComponent * MyComp,
	class AActor * Other,
	class UPrimitiveComponent * OtherComp,
	bool bSelfMoved,
	FVector HitLocation,
	FVector HitNormal,
	FVector NormalImpulse,
	const FHitResult & Hit) 
{
	Super::ReceiveHit(MyComp, Other, OtherComp, bSelfMoved, HitLocation, HitNormal, NormalImpulse, Hit);
	UE_LOG(LogTemp, Warning, TEXT("Collided!"));
	gravitySource = Other;
}

void ASpaceToJumpCharacter::Tick(float DeltaTime) {
	Super::Tick(DeltaTime);

	applyMagnetModeGravity();

	deltaTimePhyiscsBoostMultiplier = DeltaTime * 40;

}


//////////////////////////////////////////////////////////////////////////
// Input

void ASpaceToJumpCharacter::SetupPlayerInputComponent(class UInputComponent* InputComponent)
{
	// set up gameplay key bindings
	check(InputComponent);

	InputComponent->BindAction("Jump", IE_Pressed, this, &ACharacter::Jump);

	InputComponent->BindAction("PlatformJump", IE_Pressed, this, &ASpaceToJumpCharacter::OnPlatformJump);
	
	InputComponent->BindAction("Fire", IE_Pressed, this, &ASpaceToJumpCharacter::OnFire);
	InputComponent->BindTouch(EInputEvent::IE_Pressed, this, &ASpaceToJumpCharacter::TouchStarted);

	InputComponent->BindAxis("MoveForward", this, &ASpaceToJumpCharacter::MoveForward);
	InputComponent->BindAxis("MoveRight", this, &ASpaceToJumpCharacter::MoveRight);
	
	// We have 2 versions of the rotation bindings to handle different kinds of devices differently
	// "turn" handles devices that provide an absolute delta, such as a mouse.
	// "turnrate" is for devices that we choose to treat as a rate of change, such as an analog joystick
	InputComponent->BindAxis("Turn", this, &APawn::AddControllerYawInput);
	InputComponent->BindAxis("TurnRate", this, &ASpaceToJumpCharacter::TurnAtRate);
	InputComponent->BindAxis("LookUp", this, &APawn::AddControllerPitchInput);
	InputComponent->BindAxis("LookUpRate", this, &ASpaceToJumpCharacter::LookUpAtRate);
}

void ASpaceToJumpCharacter::OnFire()
{
	// try and fire a projectile
	if (ProjectileClass != NULL)
	{
		const FRotator SpawnRotation = GetControlRotation();
		// MuzzleOffset is in camera space, so transform it to world space before offsetting from the character location to find the final muzzle position
		const FVector SpawnLocation = GetActorLocation() + SpawnRotation.RotateVector(GunOffset);

		UWorld* const World = GetWorld();
		if (World != NULL)
		{
			// spawn the projectile at the muzzle
			World->SpawnActor<ASpaceToJumpProjectile>(ProjectileClass, SpawnLocation, SpawnRotation);
		}
	}

	// try and play the sound if specified
	if (FireSound != NULL)
	{
		UGameplayStatics::PlaySoundAtLocation(this, FireSound, GetActorLocation());
	}

	// try and play a firing animation if specified
	if(FireAnimation != NULL)
	{
		// Get the animation object for the arms mesh
		UAnimInstance* AnimInstance = Mesh1P->GetAnimInstance();
		if(AnimInstance != NULL)
		{
			AnimInstance->Montage_Play(FireAnimation, 1.f);
		}
	}

}

void ASpaceToJumpCharacter::OnPlatformJump() {
	const FRotator Rotation = GetControlRotation();
	FRotator YawRotation(0, Rotation.Yaw, 0);
	const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);

	CharacterMovement->Velocity += Direction*1000;

	if (GEngine) {
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow, TEXT("PlatformJump"));
	}
}

void ASpaceToJumpCharacter::TouchStarted(const ETouchIndex::Type FingerIndex, const FVector Location)
{
	// only fire for first finger down
	if (FingerIndex == 0)
	{
		OnFire();
	}
}

void ASpaceToJumpCharacter::MoveForward(float Value)
{
	if (Value != 0.0f)
	{
		// find out which way is forward
		const FRotator Rotation = GetControlRotation();
		FRotator YawRotation(0, Rotation.Yaw, 0);

		// Get forward vector
		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);

		// add movement in that direction
		AddMovementInput(Direction, Value);
	}
}

void ASpaceToJumpCharacter::MoveRight(float Value)
{
	if (Value != 0.0f)
	{
		// find out which way is right
		const FRotator Rotation = GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// Get right vector
		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		// add movement in that direction
		AddMovementInput(Direction, Value);
	}
}

void ASpaceToJumpCharacter::TurnAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerYawInput(Rate * BaseTurnRate * GetWorld()->GetDeltaSeconds());
}

void ASpaceToJumpCharacter::LookUpAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerPitchInput(Rate * BaseLookUpRate * GetWorld()->GetDeltaSeconds());
}


////// GRAVITY!

void ASpaceToJumpCharacter::applyMagnetModeGravity() {

}
