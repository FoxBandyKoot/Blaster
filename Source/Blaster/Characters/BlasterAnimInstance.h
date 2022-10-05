// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Blaster/BlasterTypes/TurningInPlace.h"

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "Blaster/BlasterTypes/CombatState.h"
#include "BlasterAnimInstance.generated.h"

/**
 * 
 */
UCLASS()
class BLASTER_API UBlasterAnimInstance : public UAnimInstance
{
	GENERATED_BODY()
	
public:

	virtual void NativeInitializeAnimation() override; // Equivalent to BeginPlay for animations
	virtual void NativeUpdateAnimation(float DeltaTime) override; // Equivalent to Tick for animations

private:
	UPROPERTY(BlueprintReadOnly, Category = Custom, meta = (AllowPrivateAccess = "true"));
	class ABlasterCharacter* BlasterCharacter;

	UPROPERTY(BlueprintReadOnly, Category = Custom, meta = (AllowPrivateAccess = "true"));
	float Speed;

	UPROPERTY(BlueprintReadOnly, Category = Custom, meta = (AllowPrivateAccess = "true"));
	bool bIsInAir;

	UPROPERTY(BlueprintReadOnly, Category = Custom, meta = (AllowPrivateAccess = "true"));
	bool bIsAccelerating; // When the player press the key to move

	UPROPERTY(BlueprintReadOnly, Category = Custom, meta = (AllowPrivateAccess = "true"));
	bool bWeaponEquipped;

	UPROPERTY();
	class AWeapon* EquippedWeapon;

	UPROPERTY(BlueprintReadOnly, Category = Custom, meta = (AllowPrivateAccess = "true"));
	bool bIsCrounched;

	UPROPERTY(BlueprintReadOnly, Category = Custom, meta = (AllowPrivateAccess = "true"));
	bool bIsAiming;

	UPROPERTY(BlueprintReadOnly, Category = Custom, meta = (AllowPrivateAccess = "true"));
	float YawOffset;

	UPROPERTY(BlueprintReadOnly, Category = Custom, meta = (AllowPrivateAccess = "true"));
	float Lean;

	UPROPERTY(BlueprintReadOnly, Category = Custom, meta = (AllowPrivateAccess = "true"));
	float AO_Yaw;

	UPROPERTY(BlueprintReadOnly, Category = Custom, meta = (AllowPrivateAccess = "true"));
	float AO_Pitch;

	UPROPERTY(BlueprintReadOnly, Category = Custom, meta = (AllowPrivateAccess = "true"));
	FTransform LeftHandTransform;

	FRotator CharacterRotationLastFrame;
	FRotator CharacterRotation;
	FRotator DeltaRotation;

	UPROPERTY(BlueprintReadOnly, Category = Custom, meta = (AllowPrivateAccess = "true"));
	ETurningInPlace TurningInPlace;

	UPROPERTY(BlueprintReadOnly, Category = Custom, meta = (AllowPrivateAccess = "true"));
	FRotator RightHandRotation;

	UPROPERTY(BlueprintReadOnly, Category = Custom, meta = (AllowPrivateAccess = "true"));
	bool bIsLocallyControlled;

	UPROPERTY(BlueprintReadOnly, Category = Custom, meta = (AllowPrivateAccess = "true"));
	bool bRotateRootBone;
	
	UPROPERTY(BlueprintReadOnly, Category = Custom, meta = (AllowPrivateAccess = "true"));
	bool bElimned;

	UPROPERTY(BlueprintReadOnly, Category = Custom, meta = (AllowPrivateAccess = "true"));
	bool bUseFABRIK;

	UPROPERTY(BlueprintReadOnly, Category = Custom, meta = (AllowPrivateAccess = "true"));
	bool bUseAimOffsets;

	UPROPERTY(BlueprintReadOnly, Category = Custom, meta = (AllowPrivateAccess = "true"));
	bool bTransformRightHand;
};
