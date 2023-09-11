// Copyright Epic Games, Inc. All Rights Reserved.

#include "GameplayMechanicsCharacter.h"
#include "GameplayMechanicsProjectile.h"
#include "Animation/AnimInstance.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "GameFramework/InputSettings.h"
#include "HeadMountedDisplayFunctionLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "MotionControllerComponent.h"
#include "Math/UnrealMathUtility.h"
#include "GameFramework/MovementComponent.h"
#include "XRMotionControllerBase.h" // for FXRMotionControllerBase::RightHandSourceId

DEFINE_LOG_CATEGORY_STATIC(LogFPChar, Warning, All);

//////////////////////////////////////////////////////////////////////////
// AGameplayMechanicsCharacter

AGameplayMechanicsCharacter::AGameplayMechanicsCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(55.f, 96.0f);

	// set our turn rates for input
	BaseTurnRate = 45.f;
	BaseLookUpRate = 45.f;

	// Create a CameraComponent	
	FirstPersonCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
	FirstPersonCameraComponent->SetupAttachment(GetCapsuleComponent());
	FirstPersonCameraComponent->SetRelativeLocation(FVector(-39.56f, 1.75f, 64.f)); // Position the camera
	FirstPersonCameraComponent->bUsePawnControlRotation = true;

	// Create a mesh component that will be used when being viewed from a '1st person' view (when controlling this pawn)
	Mesh1P = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("CharacterMesh1P"));
	Mesh1P->SetOnlyOwnerSee(true);
	Mesh1P->SetupAttachment(FirstPersonCameraComponent);
	Mesh1P->bCastDynamicShadow = false;
	Mesh1P->CastShadow = false;
	Mesh1P->SetRelativeRotation(FRotator(1.9f, -19.19f, 5.2f));
	Mesh1P->SetRelativeLocation(FVector(-0.5f, -4.4f, -155.7f));

	// Create a gun mesh component
	FP_Gun = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("FP_Gun"));
	FP_Gun->SetOnlyOwnerSee(false);			// otherwise won't be visible in the multiplayer
	FP_Gun->bCastDynamicShadow = false;
	FP_Gun->CastShadow = false;
	// FP_Gun->SetupAttachment(Mesh1P, TEXT("GripPoint"));
	FP_Gun->SetupAttachment(RootComponent);

	GrabbedObjectLocation = CreateDefaultSubobject<USceneComponent>(TEXT("GrabbedObjectLocation"));
	GrabbedObjectLocation->SetupAttachment(FP_Gun);

}

void AGameplayMechanicsCharacter::BeginPlay()
{
	// Call the base class  
	Super::BeginPlay();

	//Attach gun mesh component to Skeleton, doing it here because the skeleton is not yet created in the constructor
	FP_Gun->AttachToComponent(Mesh1P, FAttachmentTransformRules(EAttachmentRule::SnapToTarget, true), TEXT("GripPoint"));

	CurrentFuel = MaxFuel;
}

//////////////////////////////////////////////////////////////////////////
// Input

void AGameplayMechanicsCharacter::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
	// set up gameplay key bindings
	check(PlayerInputComponent);

	// Bind jump events
	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ACharacter::Jump);
	PlayerInputComponent->BindAction("Jump", IE_Released, this, &ACharacter::StopJumping);

	//Dashing
	PlayerInputComponent->BindAction("Dashing", IE_Pressed, this, &AGameplayMechanicsCharacter::Dashing);

	// Bind fire event
	PlayerInputComponent->BindAction("Fire", IE_Pressed, this, &AGameplayMechanicsCharacter::OnFire);
	PlayerInputComponent->BindAction("Fire", IE_Released, this, &AGameplayMechanicsCharacter::EndFire);

	// Firing Charge
	PlayerInputComponent->BindAction("FiringForceCharge", IE_Pressed, this, &AGameplayMechanicsCharacter::StartCharge);
	PlayerInputComponent->BindAction("FiringForceCharge", IE_Released, this, &AGameplayMechanicsCharacter::EndCharge);

	// Bind movement events
	PlayerInputComponent->BindAxis("MoveForward", this, &AGameplayMechanicsCharacter::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &AGameplayMechanicsCharacter::MoveRight);

	// We have 2 versions of the rotation bindings to handle different kinds of devices differently
	// "turn" handles devices that provide an absolute delta, such as a mouse.
	// "turnrate" is for devices that we choose to treat as a rate of change, such as an analog joystick
	PlayerInputComponent->BindAxis("Turn", this, &APawn::AddControllerYawInput);
	PlayerInputComponent->BindAxis("TurnRate", this, &AGameplayMechanicsCharacter::TurnAtRate);
	PlayerInputComponent->BindAxis("LookUp", this, &APawn::AddControllerPitchInput);
	PlayerInputComponent->BindAxis("LookUpRate", this, &AGameplayMechanicsCharacter::LookUpAtRate);
}

void AGameplayMechanicsCharacter::Tick(float DeltaTime) // TickEvent
{
	Super::Tick(DeltaTime);
	TickCharge(DeltaTime); // for GravGun

	// Jetpack
	if(isSpacebarDown)
	{
		Jetpack(DeltaTime);
	}
	else if(!isSpacebarDown)
	{
		CurrentFuel = FMath::Clamp((CurrentFuel + (DeltaTime/FuelRechargeRate)),0.f,MaxFuel);
	}
}

void AGameplayMechanicsCharacter::Dashing() // Dashing
{
	// Allow dashing only when Player is moving
	if(GetCharacterMovement()->Velocity != FVector::ZeroVector)
	{
		IsDashing = true;
		// Get last movement input for dash direction
		const FVector DashDirection = GetCharacterMovement()->GetLastInputVector();
		LaunchCharacter(DashDirection*DashSpeed, true, true);
	}
}

float AGameplayMechanicsCharacter::GetDelayDuration() // Get Dash Duration
{
	//Launch Velocity = DashSpeed = DashDistance/Time
	//Time = DashDistance/DashSpeed
	return DashDistance/DashSpeed;
}

void AGameplayMechanicsCharacter::OnFire() // GravGun
{
	const FCollisionQueryParams QueryParams("GravGunTrace",false,this);
	const FVector StartTrace = FirstPersonCameraComponent->GetComponentLocation();
	const FVector EndTrace = (FirstPersonCameraComponent->GetForwardVector() * PickUpRadius) + StartTrace;
	FHitResult Hit;

	if(GetWorld()->LineTraceSingleByChannel(Hit,StartTrace,EndTrace,ECC_Visibility,QueryParams))
	{
		if(UPrimitiveComponent* TracedObject = Hit.GetComponent())
		{
			if(TracedObject->IsSimulatingPhysics() && TracedObject->GetMass()<MaxPickUpMass)
			{
				SetGrabbedObject(TracedObject);
			}
		}
	}
}

void AGameplayMechanicsCharacter::EndFire() // GravGun, Releasing GrabbedObject
{
	
	if(GrabbedObject)
	{
		const FVector ShootVelocity = FirstPersonCameraComponent->GetForwardVector()*FiringForce;

		GrabbedObject->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
		GrabbedObject->SetSimulatePhysics(true);
		GrabbedObject->AddImpulse(ShootVelocity,NAME_None, true);

		SetGrabbedObject(nullptr);
			
		// try and play the sound if specified
		if (FireSound != nullptr)
		{
			UGameplayStatics::PlaySoundAtLocation(this, FireSound, GetActorLocation());
		}

		// try and play a firing animation if specified
		if (FireAnimation != nullptr)
		{
			// Get the animation object for the arms mesh
			UAnimInstance* AnimInstance = Mesh1P->GetAnimInstance();
			if (AnimInstance != nullptr)
			{
				AnimInstance->Montage_Play(FireAnimation, 1.f);
			}
		}			
	}	
}

void AGameplayMechanicsCharacter::SetGrabbedObject(UPrimitiveComponent* ObjectToGrab)
{
	GrabbedObject = ObjectToGrab;
	if(GrabbedObject)
	{
		GrabbedObject->SetSimulatePhysics(false);
		GrabbedObject->AttachToComponent
		(GrabbedObjectLocation, FAttachmentTransformRules::SnapToTargetNotIncludingScale);

	}
}

void AGameplayMechanicsCharacter::TickCharge(float DeltaTime) // GravGun Add-Ons
{
	if(IsCharging)
	{
		FiringCharge = FMath::Clamp((FiringCharge + (DeltaTime/FiringChargeTime)), 0.f,1.f);
	}
}

void AGameplayMechanicsCharacter::StartCharge()
{
	IsCharging = true;
	FiringCharge = 0;
}

void AGameplayMechanicsCharacter::EndCharge()
{
	IsCharging = false;
	FiringForce = FMath::Lerp(MinFiringForce, MaxFiringForce, FiringCharge);
}

void AGameplayMechanicsCharacter::Jetpack(float DeltaTime) // Jetpack
{
	if(CurrentFuel>0)
	{
		if(isSpacebarDown)
		{
			GetMovementComponent()->Velocity.Z = LaunchSpeed;
			GetCharacterMovement()->SetMovementMode(MOVE_Falling);
			GetCharacterMovement()->AirControl = JetpackAirControl;
			CurrentFuel -= (DeltaTime/FuelConsumptionRate);
		}
	}

	else if(CurrentFuel<=0)
	{
		StopJumping();
		GetCharacterMovement()->SetMovementMode(MOVE_Falling);
	}
}

float AGameplayMechanicsCharacter::GetFuelPercent() // Fuel Percentage for HUD
{
	return CurrentFuel/MaxFuel;
}

void AGameplayMechanicsCharacter::Jump()
{
	isSpacebarDown = true;
}

void AGameplayMechanicsCharacter::StopJumping()
{
	isSpacebarDown = false;
	GetCharacterMovement()->AirControl = 0.05f;
}


void AGameplayMechanicsCharacter::MoveForward(float Value)
{
	if (Value != 0.0f)
	{
		// add movement in that direction
		AddMovementInput(GetActorForwardVector(), Value);
	}
}


void AGameplayMechanicsCharacter::MoveRight(float Value)
{
	if (Value != 0.0f)
	{
		// add movement in that direction
		AddMovementInput(GetActorRightVector(), Value);
	}
}

void AGameplayMechanicsCharacter::TurnAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerYawInput(Rate * BaseTurnRate * GetWorld()->GetDeltaSeconds());
}

void AGameplayMechanicsCharacter::LookUpAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerPitchInput(Rate * BaseLookUpRate * GetWorld()->GetDeltaSeconds());
}

