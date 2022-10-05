// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Blaster/BlasterTypes/TurningInPlace.h"
#include "Blaster/Interfaces/Interface_InteractWithCrosshairs.h"
#include "Blaster/BlasterTypes/CombatState.h"

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Components/TimelineComponent.h"
#include "BlasterCharacter.generated.h"

/**
 * @brief ABlasterCharacter is Friend of CombatComponent
 * 
 */
UCLASS()
class BLASTER_API ABlasterCharacter : public ACharacter, public IInterface_InteractWithCrosshairs
{
	GENERATED_BODY()

public:
	ABlasterCharacter();
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void PostInitializeComponents() override;
	virtual void Destroyed() override;
	bool IsWeaponEquipped();
	bool IsAiming();
	void SetOverlappingWeapon(AWeapon* Weapon);
	void Elim();
	
	void PlayFireMontage(bool bAiming);
	void PlayReloadMontage();

	UFUNCTION(NetMulticast, Reliable)
	void MulticastElim();

	virtual void OnRep_ReplicatedMovement() override;

	FORCEINLINE float GetAO_Yaw() const {return AO_Yaw; }
	FORCEINLINE float GetAO_Pitch() const {return AO_Pitch; }
	FORCEINLINE float GetHealth() const { return Health; }
	FORCEINLINE float GetMaxHealth() const { return MaxHealth; }
	FORCEINLINE bool ShouldRotateRootBone() const { return bRotateRootBone; }
	FORCEINLINE bool IsElimned() const { return bElimned; }
	FORCEINLINE bool GetDisableGameplay() const { return bDisableGameplay; }
	FORCEINLINE ETurningInPlace GetETurningInPlace() const {return TurningInPlace; }
	FORCEINLINE class UCameraComponent* GetFollowCamera() const { return FollowCamera; }
	FORCEINLINE class UCombatComponent* GetCombatComponent() const { return CombatComponent; }
	AWeapon* GetEquippedWeapon();
	FVector GetHitTarget() const;
	ECombatState GetCombatState();
	
	void HideCameraIfCharacterClose();

	UPROPERTY(EditAnywhere, Category = Custom)
	float CameraThreshold = 200.f;

	bool bRotateRootBone;
	float TurnThreshold = 0.9f;
	FRotator ProxyRotationLastFrame;
	FRotator ProxyRotation;
	float ProxyYaw;
	float TimeSinceLastMovementReplication;
	
	UPROPERTY(Replicated)
	bool bDisableGameplay = false;

protected:
	virtual void BeginPlay() override;
	
	void MoveForward(float Value);
	void MoveRight(float Value);
	void Turn(float Value);
	void LookUp(float Value);
	void EquipButtonPressed(); // Only used by the server
	void CrouchButtonPressed();
	void ReloadButtonPressed();
	void AimButtonPressed();
	void AimButtonReleased();
	void AimOffset(float DeltaTime);
	void SimProxiesTurn();
	void TurnInPlace(float DeltaTime);
	virtual void Jump() override;
	void PollInit(); // Poll for any relevant classes and initialize our HUD
	void RotateInPlace(float DeltaTime);

	UFUNCTION()
	void ReceiveDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType, class AController* InstigatorController, AActor* DamageCauser);
	void UpdateHUDHealth();

	UFUNCTION(Server, Reliable)
	void ServerEquipButtonPressed(); // RPC

	void FireButtonPressed();
	void FireButtonReleased();
		
	/**
	 * Animation montages
	 * 
	 */

	void PlayHitReactMontage();
	void PlayElimMontage();

	UPROPERTY(EditAnywhere, Category = Custom)
	class UAnimMontage* FireWeaponMontage;

	UPROPERTY(EditAnywhere, Category = Custom)
	class UAnimMontage* ReloadMontage;

	UPROPERTY(EditAnywhere, Category = Custom)
	class UAnimMontage* HitReactMontage;
	
	UPROPERTY(EditAnywhere, Category = Custom)
	class UAnimMontage* ElimMontage;




private:
	UPROPERTY();
	class ABlasterPlayerController* BlasterPlayerController;

	float CalculateSpeed();
	
	UPROPERTY(VisibleAnywhere, Category = Custom)
	class USpringArmComponent* CameraBoom;
	
	UPROPERTY(VisibleAnywhere, Category = Custom)
	class UCameraComponent* FollowCamera;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
	class UWidgetComponent* OverheadWidget;

	UPROPERTY(ReplicatedUsing = OnRep_OverlappingWeapon)
	class AWeapon* OverlappingWeapon;

	UFUNCTION() // Send data only to client
	void OnRep_OverlappingWeapon(AWeapon* LastWeapon);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	class UCombatComponent* CombatComponent;

	/**
	 * Player animations
	 * 
	 */
	float AO_Yaw;
	float Interp_AO_Yaw;
	float AO_Pitch;
	FRotator StartingAimRotation;
	ETurningInPlace TurningInPlace;
	void CalculateAO_Pitch();

	/**
	 * Player health
	 * 
	 */
	UPROPERTY(EditAnywhere, Category = Custom)
	float MaxHealth = 100.f;

	UPROPERTY(VisibleAnywhere, ReplicatedUsing = OnRep_Health, Category = Custom)
	float Health = 100.f;

	bool bElimned = false;

	FTimerHandle ElimTimer;

	UPROPERTY(EditDefaultsOnly, Category = Custom)
	float ElimDelay = 3.f;
	void ElimTimerFinished();

	// It's a rep notify it's only called on clients, and not server
	UFUNCTION()
	void OnRep_Health();

	/**
	 * Dissolve effect
	 * 
	 */
	// UPROPERTY(VisibleAnywhere)
	UTimelineComponent* TimelineDissolve;

	UPROPERTY();
	FOnTimelineFloat DissolveTrack; // If we peek the definition, we see that FOnTimelineFloat is a delegate
	
	UPROPERTY(EditAnywhere, Category = Custom)
	UCurveFloat* DissolveCurve;

	UFUNCTION()
	void UpdateDissolveMaterial(float DissolveValue); // is a Callback
	void StartDissolve();

	// Dynamic instance that we can change at runtime
	UPROPERTY(VisibleAnywhere, Category = Custom)
	UMaterialInstanceDynamic* DynamicDissolveMaterialInstance;

	// Material instance set on the Blueprint, used with dynamic material instance
	UPROPERTY(EditAnywhere, Category = Custom)
	UMaterialInstance* DissolveMaterialInstance;

	/**
	 * Elim bot
	 * 
	 */
	UPROPERTY(EditAnywhere)
	UParticleSystem* ElimBotEffect;

	UPROPERTY(VisibleAnywhere)
	UParticleSystemComponent* ElimBotComponent;

	UPROPERTY(EditAnywhere)
	class USoundCue* ElimBotRandomSound;
	
	UPROPERTY(EditAnywhere)
	class USoundCue* ElimBotSound;

	UPROPERTY();
	class ABlasterPlayerState* BlasterPlayerState;

};
