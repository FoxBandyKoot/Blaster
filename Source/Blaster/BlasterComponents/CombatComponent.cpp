// Fill out your copyright notice in the Description page of Project Settings.


#include "CombatComponent.h"
#include "Blaster/Weapons/Weapon.h"
#include "Blaster/Characters/BlasterCharacter.h"
#include "Blaster/PlayerController/BlasterPlayerController.h"

#include "TimerManager.h"
#include "Net/UnrealNetwork.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Components/SphereComponent.h"
#include "DrawDebugHelpers.h"
#include "Math/UnrealMathUtility.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Camera/CameraComponent.h"
#include "Sound/SoundCue.h"

UCombatComponent::UCombatComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	BaseWalkSpeed = 600;
	AimWalkSpeed = 450;
}

void UCombatComponent::BeginPlay()
{
	Super::BeginPlay();
	if(Character)
	{
		Character->GetCharacterMovement()->MaxWalkSpeed = BaseWalkSpeed;
		
		if(Character->GetFollowCamera()) 
		{
			DefaultFOV = Character->GetFollowCamera()->FieldOfView;
			CurrentFOV = DefaultFOV;
		}
		
		if(Character->HasAuthority())
		{	
			InitializeCarriedAmmo();
		}
	}	
}

void UCombatComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	

	if(Character && Character->IsLocallyControlled())
	{
		TraceUnderCrosshairs(HitResult);
		HitTarget = HitResult.ImpactPoint;
		
		SetHUDCrosshairs(DeltaTime);
		InterpFOV(DeltaTime);
	}
}


// EquippedWeapon && bAiming becomes replicated props
void UCombatComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UCombatComponent, EquippedWeapon);
	DOREPLIFETIME(UCombatComponent, bAiming);
	DOREPLIFETIME(UCombatComponent, CombatState);
	DOREPLIFETIME_CONDITION(UCombatComponent, CarriedAmmo, COND_OwnerOnly);
}

void UCombatComponent::EquipWeapon(class AWeapon* WeaponToEquip)
{
	if(Character == nullptr || WeaponToEquip == nullptr || CombatState != ECombatState::ECS_Unoccupied) return;
	
	if(EquippedWeapon)
	{
		EquippedWeapon->Dropped();
	}

	EquippedWeapon = WeaponToEquip;
	EquippedWeapon->SetWeaponState(EWeaponState::EWS_Equipped);

	const USkeletalMeshSocket* HandSocket = Character->GetMesh()->GetSocketByName(FName("RightHandSocket"));
	if(HandSocket)
	{
		HandSocket->AttachActor(EquippedWeapon, Character->GetMesh());
	}

	EquippedWeapon->SetOwner(Character);
	EquippedWeapon->SetHUDAmmo(); // SetHUDAmmo on the server

	if(CarriedAmmoMap.Contains(EquippedWeapon->GetWeaponType()))
	{
		CarriedAmmo = CarriedAmmoMap[EquippedWeapon->GetWeaponType()];
	}

	Controller = Controller == nullptr ? Cast<ABlasterPlayerController>(Character->Controller) : Controller;
	if(Controller)
	{
		Controller->SetHUDCarriedAmmo(CarriedAmmo);
	}

	// Update movement (for the camera too)
	Character->GetCharacterMovement()->bOrientRotationToMovement = false;
	Character->bUseControllerRotationYaw = true;

	if(EquippedWeapon->EquipSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, EquippedWeapon->EquipSound, Character->GetActorLocation());
	}

	if(EquippedWeapon->IsEmpty())
	{
		Reload();
	}
}

void UCombatComponent::OnRep_EquippedWeapon()
{
	if(EquippedWeapon && Character)
	{
		EquippedWeapon->SetWeaponState(EWeaponState::EWS_Equipped);
	
		const USkeletalMeshSocket* HandSocket = Character->GetMesh()->GetSocketByName(FName("RightHandSocket"));
		if(HandSocket)
		{
			HandSocket->AttachActor(EquippedWeapon, Character->GetMesh());
		}

		// UE_LOG(LogTemp, Warning, TEXT("OnRep_EquippedWeapon 2"));
		Character->GetCharacterMovement()->bOrientRotationToMovement = false;
		Character->bUseControllerRotationYaw = true;

		if(EquippedWeapon->EquipSound)
		{
			UGameplayStatics::PlaySoundAtLocation(this, EquippedWeapon->EquipSound, Character->GetActorLocation());
		}
	}
}


// executed only on the client which call
void UCombatComponent::Reload()
{
	if(CarriedAmmo > 0 && CombatState != ECombatState::ECS_Reloading && EquippedWeapon->GetAmmo() != EquippedWeapon->GetMagCapacity())
	{
		ServerReload();
	}
}

// RPC (only executed on the server)
void UCombatComponent::ServerReload_Implementation()
{	
	CombatState = ECombatState::ECS_Reloading;
	HandleReload();
}

// executed on server because of be called on ServerReload_Implementation()
// executed on all clients because of be called on OnRep_CombatState()
void UCombatComponent::HandleReload()
{
	Character->PlayReloadMontage();
}

// Called by the BP_AnimBlaster.EventGraph
void UCombatComponent::FinishReloading()
{
	if (!Character) return;
	if (Character->HasAuthority())
	{
		UpdateAmmoValues();	
		CombatState = ECombatState::ECS_Unoccupied;
	}
	if(bFireButtonPressed)
	{
		Fire();
	}	
}

int32 UCombatComponent::AmountToReload()
{
	if(!EquippedWeapon) return 0;

	int32 RoomInMag = EquippedWeapon->GetMagCapacity() - EquippedWeapon->GetAmmo();

	if(CarriedAmmoMap.Contains(EquippedWeapon->GetWeaponType()))
	{
		int32 AmountCarried = CarriedAmmoMap[EquippedWeapon->GetWeaponType()];
		int32 Least = FMath::Min(RoomInMag, AmountCarried); // Number of ammo to reload
		return FMath::Clamp(RoomInMag, 0, Least); // Security if we set Ammo greater than MagCapacity
	}
	return 0;
}

void UCombatComponent::UpdateAmmoValues()
{
	if(!Character || !EquippedWeapon ) return;

	if(CarriedAmmoMap.Contains(EquippedWeapon->GetWeaponType()))
	{
		CarriedAmmoMap[EquippedWeapon->GetWeaponType()] -= AmountToReload();
		CarriedAmmo = CarriedAmmoMap[EquippedWeapon->GetWeaponType()];
		EquippedWeapon->AddAmmo(CarriedAmmo);
	}
	else 
	{
		UE_LOG(LogTemp, Error, TEXT("UCombatComponent.AmountToReload : CarriedAmmoMap does not contains our weapon."));
		return;
	}
	
	Controller = Controller == nullptr ? Cast<ABlasterPlayerController>(Character->Controller) : Controller;
	if(Controller)
	{
		Controller->SetHUDCarriedAmmo(CarriedAmmo);
	}
}

void UCombatComponent::SetAiming(bool _bAiming)
{
	bAiming = _bAiming;
	if(Character)
	{
		Character->GetCharacterMovement()->MaxWalkSpeed = bAiming ? AimWalkSpeed : BaseWalkSpeed;
	}
	ServerSetAiming(_bAiming);
}

/**
 * Is an RPC
 * */
void UCombatComponent::ServerSetAiming_Implementation(bool _bAiming)
{
	bAiming = _bAiming;
	if(Character)
	{
		Character->GetCharacterMovement()->MaxWalkSpeed = bAiming ? AimWalkSpeed : BaseWalkSpeed;
	}
}

void UCombatComponent::FireButtonPressed(bool bPressed)
{
	bFireButtonPressed = bPressed;
	if(!bFireButtonPressed) return;
	Fire();
}

void UCombatComponent::Fire()
{
	if(!CanFire()) return;
	bCanFire = false;
	Server_FireButtonPressed(HitTarget);
	CrosshairShootingFactor = 1.f;
	StartFireTimer();
}

/**
 * @brief RPC designed to be called on a client and executed on a server
 * 
 * @param bPressed 
 */
void UCombatComponent::Server_FireButtonPressed_Implementation(const FVector_NetQuantize TraceHitTarget)
{
	Multicast_FireButtonPressed(TraceHitTarget);
}

void UCombatComponent::Multicast_FireButtonPressed_Implementation(const FVector_NetQuantize TraceHitTarget)
{
	if(!Character) return;
	if(!EquippedWeapon)
	{
		// bCanFire = true;
		return;
	}
	if(CombatState == ECombatState::ECS_Unoccupied)
	{
		Character->PlayFireMontage(bAiming);
	 	EquippedWeapon->Fire(TraceHitTarget);
	}
	
}


void UCombatComponent::StartFireTimer()
{
	if(!EquippedWeapon || !Character) return;
	Character->GetWorldTimerManager().SetTimer
	(
		FireTimer,
		this,
		&UCombatComponent::FireTimerFinished,
		EquippedWeapon->FireDelay
	); 
}


void UCombatComponent::FireTimerFinished()
{
	if(!EquippedWeapon) return;
	bCanFire = true;
	if(!bFireButtonPressed || !EquippedWeapon->bAutomatic) return;
	Fire();

	if(EquippedWeapon->IsEmpty())
	{
		Reload();
	}
}

/**
 * Display the cross hair
 * Compute the position of the hit
 * @param TraceHitResult 
 */
void UCombatComponent::TraceUnderCrosshairs(FHitResult& TraceHitResult)
{
	if(!GEngine || !GEngine->GameViewport) return;

	// Display the cross hair
	FVector2D ViewportSize;
	GEngine->GameViewport->GetViewportSize(ViewportSize);

	FVector2D CrosshairLocation(ViewportSize.X / 2, ViewportSize.Y / 2);
	FVector CrosshairWorldPosition;
	FVector CrosshairWorldDirection;

	bool bScreenToWorld = UGameplayStatics::DeprojectScreenToWorld(
		UGameplayStatics::GetPlayerController(this, 0),
		CrosshairLocation, CrosshairWorldPosition, CrosshairWorldDirection);
	
	// Compute the position of the hit
	if(bScreenToWorld)
	{
		FVector Start = CrosshairWorldPosition;
		
		// Compute the distance between the crosshair and the player 
		// to adjust the lenght of the CameraBoom to start to trace and do not enter in collision with our own character
		if(Character)
		{			
			float DistanceToCharacter = (Character->GetActorLocation() - Start).Size();
			Start += CrosshairWorldDirection * (DistanceToCharacter + 100.f);
			// UE_LOG(LogTemp, Warning, TEXT("StartFinal %f"), Start.Size());
			
			// DrawDebugSphere(GetWorld(), Start, 16.f, 12, FColor::Red);
		}
		FVector End = Start + CrosshairWorldDirection * TRACE_LENGTH;

		GetWorld()->LineTraceSingleByChannel(TraceHitResult, Start, End, ECollisionChannel::ECC_Visibility);

		if(TraceHitResult.GetActor() && TraceHitResult.GetActor()->Implements<UInterface_InteractWithCrosshairs>())
		{
			HUDPackage.CrosshairsColor = FLinearColor::Red;
		}
		else 
		{
			HUDPackage.CrosshairsColor = FLinearColor::White;
		}

		// If we hit nothing in the range of TRACE_LENGTH, set an impact point yet
		if(!TraceHitResult.bBlockingHit)
		{
			TraceHitResult.ImpactPoint = End;
		}
		else 
		{
			// DrawDebugSphere(GetWorld(), TraceHitResult.ImpactPoint, 12, 12, FColor::Red);
		}
	}	
}

void UCombatComponent::SetHUDCrosshairs(float DeltaTime)
{
	if(!Character){UE_LOG(LogTemp, Error, TEXT("CombatComponent.cpp: Character is null")); return;}
	if(!Character->IsLocallyControlled()) return;
	if(!Character->Controller){UE_LOG(LogTemp, Error, TEXT("CombatComponent.cpp: Character->Controller is null")); return;}
	
	Controller = Controller == nullptr ? Cast<ABlasterPlayerController>(Character->Controller) : Controller;
	if(!Controller) {UE_LOG(LogTemp, Error, TEXT("CombatComponent.cpp: Controller is null")); return;}

	HUD = HUD == nullptr ? Cast<ABlasterHUD>(Controller->GetHUD()) : HUD;
	if(!HUD) {UE_LOG(LogTemp, Error, TEXT("CombatComponent.cpp: HUD is null")); return;}

	if(!EquippedWeapon) {return;}

	// Setup crosshair widgets
	HUDPackage.CrosshairsCenter = EquippedWeapon->CrosshairsCenter;
	HUDPackage.CrosshairsLeft = EquippedWeapon->CrosshairsLeft;
	HUDPackage.CrosshairsRight = EquippedWeapon->CrosshairsRight;
	HUDPackage.CrosshairsTop = EquippedWeapon->CrosshairsTop;
	HUDPackage.CrosshairsBottom = EquippedWeapon->CrosshairsBottom;

	// Calculate crosshair spread in function of the velocity
	// [0, 600] -> [0, 1]
	FVector2D WalkSpeedRange(0, Character->GetCharacterMovement()->MaxWalkSpeed);
	FVector2D VelocityMultiplierRange(0, 1);
	FVector Velocity = Character->GetVelocity();
	Velocity.Z = 0;

	CrosshairVelocityFactor = FMath::GetMappedRangeValueClamped(WalkSpeedRange, VelocityMultiplierRange, Velocity.Size());
	HUDPackage.CrosshairSpread = CrosshairVelocityFactor;

	// Calculate crosshair spread in function of if the character is in the air
	if(Character->GetCharacterMovement()->IsFalling())
	{
		CrosshairInAirFactor = FMath::FInterpTo(CrosshairInAirFactor, 2.25, DeltaTime, 2);
	}
	else
	{
		CrosshairInAirFactor = FMath::FInterpTo(CrosshairInAirFactor, 0, DeltaTime, 30);
	}

	// Calculate crosshair shrink in function of if the character is aiming
	if(bAiming && EquippedWeapon)
	{
		CrosshairAimFactor = FMath::FInterpTo(CrosshairAimFactor, 0.58f, DeltaTime, EquippedWeapon->GetZoomInterpSpeed());
	}
	else
	{
		CrosshairAimFactor = FMath::FInterpTo(CrosshairAimFactor, 0, DeltaTime, EquippedWeapon->GetZoomInterpSpeed());
	}
	
	CrosshairShootingFactor = FMath::FInterpTo(CrosshairShootingFactor, 0, DeltaTime, EquippedWeapon->GetZoomInterpSpeed());

	HUDPackage.CrosshairSpread = 
	MinimalCrosshairSpread + CrosshairVelocityFactor + CrosshairInAirFactor + CrosshairShootingFactor
	- CrosshairAimFactor;

	HUD->SetHUDPackage(HUDPackage);	
}


/**
 * @brief Update Field of view 
 * 
 * @param DeltaTime 
 */
void UCombatComponent::InterpFOV(float DeltaTime)
{
	if(!EquippedWeapon) {return;}
	if(bAiming)
	{
		CurrentFOV = FMath::FInterpTo(CurrentFOV, EquippedWeapon->GetZoomedFOV(), DeltaTime, EquippedWeapon->GetZoomInterpSpeed());
	}
	else 
	{
		CurrentFOV = FMath::FInterpTo(CurrentFOV, DefaultFOV, DeltaTime, EquippedWeapon->GetZoomInterpSpeed());
	}

	if(!Character || !Character->GetFollowCamera()) {return;}
	
	Character->GetFollowCamera()->SetFieldOfView(CurrentFOV);
}


bool UCombatComponent::CanFire()
{
	if(!EquippedWeapon) return false;
	if(EquippedWeapon->IsEmpty()) return false;
	if(bCanFire == false) return false;
	if(CombatState != ECombatState::ECS_Unoccupied) return false;
	return true;
}

void UCombatComponent::OnRep_CarriedAmmo()
{
	Controller = Controller == nullptr ? Cast<ABlasterPlayerController>(Character->Controller) : Controller;
	if(Controller)
	{
		Controller->SetHUDCarriedAmmo(CarriedAmmo);
	}
}

void UCombatComponent::InitializeCarriedAmmo()
{
	CarriedAmmoMap.Emplace(EWeaponType::EWT_AssaultRifle, StartingAmmo_AssaultRifle);
}

void UCombatComponent::OnRep_CombatState()
{
	switch(CombatState)
	{
		case ECombatState::ECS_Reloading:
			HandleReload();
			break;
		case ECombatState::ECS_Unoccupied:
			if(bFireButtonPressed)
			{
				Fire();
			}
	}
}