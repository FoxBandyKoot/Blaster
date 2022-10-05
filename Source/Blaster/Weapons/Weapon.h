// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WeaponTypes.h"
#include "Animation/AnimationAsset.h"
#include "Weapon.generated.h"



UENUM(BlueprintType)
enum class EWeaponState : uint8
{
	EWS_Initial UMETA(DisplayName = "Initial State"),
	EWS_Equipped UMETA(DisplayName = "Equipped"),
	EWS_Dropped UMETA(DisplayName = "Dropped"),
	EWS_MAX UMETA(DisplayName = "DefaultMAX"),
};



UCLASS()
class BLASTER_API AWeapon : public AActor
{
	GENERATED_BODY()
	
public:	
	AWeapon();
	virtual void Tick(float DeltaTime) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
	void Dropped();
	void AddAmmo(int32 AmmotToAdd);
	bool IsEmpty();
	void SetHUDAmmo();
	virtual void OnRep_Owner() override;
	void ShowPickupWidget(bool bShowWidget);
	void SetWeaponState(EWeaponState State);
	virtual void Fire(const FVector& HitTarget);
	class USphereComponent* GetAreaShere() const {return AreaSphere;}

	FORCEINLINE USkeletalMeshComponent* GetWeaponMesh() const {return WeaponMesh;}
	FORCEINLINE float GetZoomedFOV() const {return ZoomedFOV;}
	FORCEINLINE float GetZoomInterpSpeed() const {return ZoomInterpSpeed;}
	FORCEINLINE EWeaponType GetWeaponType() const {return WeaponType;}
	FORCEINLINE int32 GetAmmo() const {return Ammo;}
	FORCEINLINE int32 GetMagCapacity() const {return MagCapacity;}

	/**
	 * @brief Textures for the weapon crosshairs;
	 * 
	 */
	UPROPERTY(EditAnywhere, Category = "Custom - Crosshairs")
	class UTexture2D* CrosshairsCenter;

	UPROPERTY(EditAnywhere, Category = "Custom - Crosshairs")
	class UTexture2D* CrosshairsLeft;

	UPROPERTY(EditAnywhere, Category = "Custom - Crosshairs")
	class UTexture2D* CrosshairsRight;

	UPROPERTY(EditAnywhere, Category = "Custom - Crosshairs")
	class UTexture2D* CrosshairsTop;

	UPROPERTY(EditAnywhere, Category = "Custom - Crosshairs")
	class UTexture2D* CrosshairsBottom;

	/**
	 * @brief Automatic fire
	 * 
	 */
	UPROPERTY(EditAnywhere, Category = Custom)
	float FireDelay = 0.15f;
	UPROPERTY(EditAnywhere, Category = Custom)
	float bAutomatic = true;

	UPROPERTY(EditAnywhere)
	class USoundCue* EquipSound;

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	virtual void OnSphereOverlap(
		UPrimitiveComponent* OverlappedComponent, 
		AActor* OtherActor, 
		UPrimitiveComponent* OtherComp, 
		int32 OtherBodyIndex, 
		bool bFromSweep, 
		const FHitResult & SweepResult 
	);
	
	UFUNCTION()
	virtual void OnSphereEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	UPROPERTY(EditAnywhere, Category = "Custom")
	UAnimationAsset* FireAnimation;

private:
	UPROPERTY(VisibleAnywhere, Category = "Custom")
	USkeletalMeshComponent* WeaponMesh;

	UPROPERTY(VisibleAnywhere, Category = "Custom")
	class USphereComponent* AreaSphere;

	UPROPERTY(ReplicatedUsing = OnRep_WeaponState, VisibleAnywhere, Category = "Custom")
	EWeaponState WeaponState;

	UFUNCTION()
	void OnRep_WeaponState();
	
	UPROPERTY()
	class ABlasterCharacter* BlasterOwnerCharacter;

	UPROPERTY()
	class ABlasterPlayerController* BlasterOwnerController;

	UPROPERTY(VisibleAnywhere, Category = "Custom")
	class UWidgetComponent* PickupWidget;

	UPROPERTY(EditAnywhere, Category = "Custom")
	TSubclassOf<class ACasing> CasingClass;

	UPROPERTY(EditAnywhere, Category = "Custom")
	float ZoomedFOV = 60.f;

	UPROPERTY(EditAnywhere, Category = "Custom")
	float ZoomInterpSpeed = 12.f;

	UPROPERTY(EditAnywhere, ReplicatedUsing = OnRep_Ammo)
	int32 Ammo;

	UFUNCTION()
	void OnRep_Ammo();

	void SpendRound();

	UPROPERTY(EditAnywhere)
	int32 MagCapacity;

	UPROPERTY(EditAnywhere)
	EWeaponType WeaponType;
};
