// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameplayMechanicsCharacter.generated.h"

class UInputComponent;
class USkeletalMeshComponent;
class USceneComponent;
class UCameraComponent;
class UMotionControllerComponent;
class UAnimMontage;
class USoundBase;

UCLASS(config=Game)
class AGameplayMechanicsCharacter : public ACharacter
{
	GENERATED_BODY()

	/** Pawn mesh: 1st person view (arms; seen only by self) */
	UPROPERTY(VisibleDefaultsOnly, Category=Mesh)
	USkeletalMeshComponent* Mesh1P;

	/** Gun mesh: 1st person view (seen only by self) */
	UPROPERTY(VisibleDefaultsOnly, Category = Mesh)
	USkeletalMeshComponent* FP_Gun;

	/** Location on gun mesh where projectiles should spawn. */
	UPROPERTY(VisibleDefaultsOnly, Category = Mesh)
	USceneComponent* FP_MuzzleLocation;

	/** First person camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	UCameraComponent* FirstPersonCameraComponent;

	UPROPERTY(EditDefaultsOnly, Category = Components)
	class USceneComponent* GrabbedObjectLocation;

public:
	AGameplayMechanicsCharacter();

	// GravGun
	virtual void Tick(float DeltaTime) override;
	void TickCharge(float DeltaTime);
	bool IsCharging = false;

protected:
	virtual void BeginPlay();

public:
	/** Base turn rate, in deg/sec. Other scaling may affect final turn rate. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=Camera)
	float BaseTurnRate;

	/** Base look up/down rate, in deg/sec. Other scaling may affect final rate. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=Camera)
	float BaseLookUpRate;

	/** Gun muzzle's offset from the characters location */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Gameplay)
	FVector GunOffset;

	/** Sound to play each time we fire */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Gameplay)
	USoundBase* FireSound;

	/** AnimMontage to play each time we fire */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Gameplay)
	UAnimMontage* FireAnimation;

protected:

	// Dashing
	void Dashing();

	UPROPERTY(EditAnywhere, Category = Dashing)
	float DashDistance = 1000.f;

	UPROPERTY(EditAnywhere, Category = Dashing)
	float DashSpeed = 9000.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool IsDashing = false;

	UFUNCTION(BlueprintPure)
	float GetDelayDuration();

	// Task 2 - GravGun
	void OnFire();
	void EndFire();
	void SetGrabbedObject(UPrimitiveComponent* ObjectToGrab);

	UPROPERTY()
	UPrimitiveComponent* GrabbedObject;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = GravGun)
	float FiringForce = 2000.f; // Firing Force
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = GravGun)
	float PickUpRadius = 5000.f; // Pick Up Radius

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = GravGun)
	float MaxPickUpMass = 500.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = GravGun)
	float FiringChargeTime = 3.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = GravGun)
	float MinFiringForce = 2000.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = GravGun)
	float MaxFiringForce = 4000.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = GravGun)
	float FiringCharge = 0.f; // Default value when begin play

	// Jetpack
	void Jump() override;
	void StopJumping() override;
	void Jetpack(float DeltaTime);
	
	UFUNCTION(BlueprintPure)
	float GetFuelPercent();

	bool isSpacebarDown;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Jetpack)
	float LaunchSpeed = 420.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Jetpack)
	float MaxFuel = 1.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Jetpack)
	float JetpackAirControl = 1.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Jetpack)
	float FuelRechargeRate = 3.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Jetpack)
	float FuelConsumptionRate = 1.f;
	float CurrentFuel = 0;

	/** Handles moving forward/backward */
	void MoveForward(float Val);

	/** Handles stafing movement, left and right */
	void MoveRight(float Val);

	/**
	 * Called via input to turn at a given rate.
	 * @param Rate	This is a normalized rate, i.e. 1.0 means 100% of desired turn rate
	 */
	void TurnAtRate(float Rate);

	/**
	 * Called via input to turn look up/down at a given rate.
	 * @param Rate	This is a normalized rate, i.e. 1.0 means 100% of desired turn rate
	 */
	void LookUpAtRate(float Rate);

	void StartCharge();
	void EndCharge();

protected:
	// APawn interface
	virtual void SetupPlayerInputComponent(UInputComponent* InputComponent) override;
	// End of APawn interface


public:
	/** Returns Mesh1P subobject **/
	USkeletalMeshComponent* GetMesh1P() const { return Mesh1P; }
	/** Returns FirstPersonCameraComponent subobject **/
	UCameraComponent* GetFirstPersonCameraComponent() const { return FirstPersonCameraComponent; }

};

