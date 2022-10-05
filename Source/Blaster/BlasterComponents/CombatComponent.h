// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Blaster/Weapons/WeaponTypes.h"
#include "Blaster/BlasterTypes/CombatState.h"
#include "Blaster/HUD/BlasterHUD.h"
#include "CombatComponent.generated.h"

#define TRACE_LENGTH 80000.f;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class BLASTER_API UCombatComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UCombatComponent();
	friend class ABlasterCharacter;

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	void EquipWeapon(class AWeapon* WeaponToEquip);
	
	/**
	 * @brief Reload chain functions
	 */
	void Reload(); // executed only on the client which call
	UFUNCTION(Server, Reliable)
	void ServerReload(); // RPC (executed only the server)
	void HandleReload(); // executed on all machines
	
	UFUNCTION(BlueprintCallable)
	void FinishReloading(); // Called by the BP_AnimBlaster.EventGraph
	int32 AmountToReload();
	void FireButtonPressed(bool bPressed);


protected:
	virtual void BeginPlay() override;

	void SetAiming(bool _bAiming);

	UFUNCTION(Server, Reliable)
	void ServerSetAiming(bool _bAiming);

	UFUNCTION()
	void OnRep_EquippedWeapon();
	
	UFUNCTION(Server, Reliable)
	void Server_FireButtonPressed(const FVector_NetQuantize TraceHitTarget);

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_FireButtonPressed(const FVector_NetQuantize TraceHitTarget);

	void TraceUnderCrosshairs(FHitResult& TraceHitResult);

	void SetHUDCrosshairs(float DeltaTime);

private:	

	UPROPERTY();
	class ABlasterCharacter* Character;

	UPROPERTY();
	class ABlasterPlayerController* Controller;

	UPROPERTY()
	class ABlasterHUD* HUD;
	
	UPROPERTY(ReplicatedUsing = OnRep_EquippedWeapon)
	class AWeapon* EquippedWeapon;
	
	UPROPERTY(Replicated)
	bool bAiming;

	UPROPERTY(EditAnywhere)
	float BaseWalkSpeed;

	UPROPERTY(EditAnywhere)
	float AimWalkSpeed;

	bool bFireButtonPressed;

	FHitResult HitResult;

	float MinimalCrosshairSpread = 0.5f;
	float CrosshairVelocityFactor; // Spread crosshair
	float CrosshairInAirFactor; // Spread crosshair
	float CrosshairAimFactor; // Shrink crosshair
	float CrosshairShootingFactor;

	FHUDPackage HUDPackage;

	FVector HitTarget;

	// Field of view when not aiming; set to the camera's base FOV in BeginPlay
	float DefaultFOV;

	UPROPERTY(EditAnywhere, Category = Custom)
	float ZoomedFOV = 30.f;

	float CurrentFOV;

	UPROPERTY(EditAnywhere, Category = Custom)
	float ZoomInterpSpeed = 20.f;

	void InterpFOV(float DeltaTime);

	/**
	 * @brief Automatic fire
	 * 
	 */
	FTimerHandle FireTimer;
	bool bCanFire = true;
	void StartFireTimer();
	void FireTimerFinished();
	void Fire();
	bool CanFire();

	// Carried ammo for the currently equipped weapon
	UPROPERTY(ReplicatedUsing = OnRep_CarriedAmmo);
	int32 CarriedAmmo;

	UFUNCTION()
	void OnRep_CarriedAmmo();

	TMap<EWeaponType, int32> CarriedAmmoMap;

	UPROPERTY(EditAnywhere)
	int32 StartingAmmo_AssaultRifle;
	
	void InitializeCarriedAmmo();

	UPROPERTY(ReplicatedUsing = OnRep_CombatState)
	ECombatState CombatState = ECombatState::ECS_Unoccupied;

	UFUNCTION()
	void OnRep_CombatState();

	void UpdateAmmoValues();

};
