// Fill out your copyright notice in the Description page of Project Settings.


#include "BlasterCharacter.h"
#include "Blaster/Blaster.h"
#include "Blaster/Weapons/WeaponTypes.h"
#include "Blaster/Weapons/Weapon.h"
#include "BlasterAnimInstance.h"
#include "Blaster/BlasterComponents/CombatComponent.h"
#include "Blaster/GameMode/BlasterGameMode.h"
#include "Blaster/PlayerController/BlasterPlayerController.h"
#include "Blaster/PlayerState/BlasterPlayerState.h"

#include "Net/UnrealNetwork.h"
#include "TimerManager.h"
#include "Sound/SoundCue.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "Camera/CameraComponent.h"
#include "Components/WidgetComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Particles/ParticleSystemComponent.h"

// if(GEngine) GEngine->AddOnScreenDebugMessage(-1, 10, FColor::Green, FString::Printf(TEXT("TEST")));

ABlasterCharacter::ABlasterCharacter()
{
	PrimaryActorTick.bCanEverTick = true;
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(GetMesh());
	CameraBoom->TargetArmLength = 300;
	CameraBoom->bUsePawnControlRotation = true;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	bUseControllerRotationYaw = false;
	GetCharacterMovement()->bOrientRotationToMovement = true;

	OverheadWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("OverheadWidget"));
	OverheadWidget->SetupAttachment(RootComponent);

	CombatComponent = CreateDefaultSubobject<UCombatComponent>(TEXT("CombatComponent"));
	CombatComponent->SetIsReplicated(true);

	GetCharacterMovement()->NavAgentProps.bCanCrouch = true;
	GetCharacterMovement()->RotationRate = FRotator(0, 0, 1000); // Speed at which the distance machines will rotate the character
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);
	GetMesh()->SetCollisionObjectType(ECC_SkeletalMesh);
	GetMesh()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);
	GetMesh()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Visibility, ECollisionResponse::ECR_Block);
	GetMesh()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Block);

	TurningInPlace = ETurningInPlace::ETIP_NotTurning;

	NetUpdateFrequency = 66;
	MinNetUpdateFrequency = 33;

	TimelineDissolve = CreateDefaultSubobject<UTimelineComponent>(TEXT("TimelineDissolveComponent"));
	// if(!TimelineDissolve) {UE_LOG(LogTemp, Error, TEXT("BlasterCharacter.cpp: TimelineDissolve is null")); return;};
}

/**
 * @brief Used to replicate data on clients, here for the prop OverlappingWeapon only on client
 */
void ABlasterCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const 
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME_CONDITION(ABlasterCharacter, OverlappingWeapon, COND_OwnerOnly); // Replicate only to the owner of this instance class (one client)
	DOREPLIFETIME(ABlasterCharacter, Health);
	DOREPLIFETIME(ABlasterCharacter, bDisableGameplay);
}

void ABlasterCharacter::PostInitializeComponents()
{
	Super::PostInitializeComponents();
	if(CombatComponent)
	{
		CombatComponent->Character = this;
	}
}

void ABlasterCharacter::BeginPlay()
{
	Super::BeginPlay();
	UpdateHUDHealth();

	if(!HasAuthority()) return;
	OnTakeAnyDamage.AddDynamic(this, &ABlasterCharacter::ReceiveDamage);
}

void ABlasterCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	
	PlayerInputComponent->BindAxis("Move Forward / Backward", this, &ThisClass::MoveForward);
	PlayerInputComponent->BindAxis("Move Right / Left", this, &ThisClass::MoveRight);
	PlayerInputComponent->BindAxis("Look Up / Down Mouse", this, &ThisClass::LookUp);
	PlayerInputComponent->BindAxis("Turn Right / Left Mouse", this, &ThisClass::Turn);

	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ThisClass::Jump);
	PlayerInputComponent->BindAction("Equip", IE_Pressed, this, &ThisClass::EquipButtonPressed);
	PlayerInputComponent->BindAction("Crouch", IE_Pressed, this, &ThisClass::CrouchButtonPressed);
	PlayerInputComponent->BindAction("Aim", IE_Pressed, this, &ThisClass::AimButtonPressed);
	PlayerInputComponent->BindAction("Aim", IE_Released, this, &ThisClass::AimButtonReleased);
	PlayerInputComponent->BindAction("Fire", IE_Pressed, this, &ThisClass::FireButtonPressed);
	PlayerInputComponent->BindAction("Fire", IE_Released, this, &ThisClass::FireButtonReleased);
	PlayerInputComponent->BindAction("Reload", IE_Pressed, this, &ThisClass::ReloadButtonPressed);
}

void ABlasterCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	RotateInPlace(DeltaTime);
	HideCameraIfCharacterClose();
	PollInit(); // Poll for any relevant classes and initialize our HUD
	// UE_LOG(LogTemp, Warning, TEXT("bUseControllerRotationYaw %d"), bUseControllerRotationYaw );
	// UE_LOG(LogTemp, Warning, TEXT("GetCharacterMovement()->bOrientRotationToMovement %d"), GetCharacterMovement()->bOrientRotationToMovement );
	// UE_LOG(LogTemp, Warning, TEXT("GetCharacterMovement()->bUseControllerDesiredRotation %d"), GetCharacterMovement()->bUseControllerDesiredRotation );
}

void ABlasterCharacter::RotateInPlace(float DeltaTime)
{

	// When the game is over
	if(bDisableGameplay)
	{
		bUseControllerRotationYaw = false;
		TurningInPlace = ETurningInPlace::ETIP_NotTurning;
		return;
	}


	if(GetLocalRole() > ENetRole::ROLE_SimulatedProxy && IsLocallyControlled()) 
	{
		// We want to use theses costly animations only on authonomous and locally controlled actors, 
		// because the BP node "rotate root bone" used in FullBody state machine in BP_AnimBlaster, 
		// is based on net update and not on frame update, and net update is more slow than frame update, so there is some jitter
		// for proxies, look at ABlasterCharacter::SimProxiesTurn()
		AimOffset(DeltaTime);
	} else 
	{
		TimeSinceLastMovementReplication += DeltaTime;
		if(TimeSinceLastMovementReplication > 0.25)
		{
			OnRep_ReplicatedMovement();
		}
		CalculateAO_Pitch();

	}
}

// Poll for any relevant classes and initialize our HUD
void ABlasterCharacter::PollInit()
{
	if(!BlasterPlayerState)	
	{
		BlasterPlayerState = GetPlayerState<ABlasterPlayerState>();
		if(!BlasterPlayerState) return;
		BlasterPlayerState->AddToScore(0);
		BlasterPlayerState->AddToDeaths(0);
	}
}

void ABlasterCharacter::MoveForward(float Value)
{
	if(Controller && Value != 0 && !bDisableGameplay)
	{
		const FRotator YawRotation(0, Controller->GetControlRotation().Yaw, 0);
		const FVector Direction(FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X));
		AddMovementInput(Direction, Value);
	}
}

void ABlasterCharacter::MoveRight(float Value)
{
	if(Controller && Value != 0 && !bDisableGameplay)
	{
		const FRotator YawRotation(0, Controller->GetControlRotation().Yaw, 0);
		const FVector Direction(FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y));
		AddMovementInput(Direction, Value);
	}
}

void ABlasterCharacter::Turn(float Value)
{
	AddControllerYawInput(Value);
}

void ABlasterCharacter::LookUp(float Value)
{
	AddControllerPitchInput(Value);
}

void ABlasterCharacter::EquipButtonPressed()
{
	if(CombatComponent && !bDisableGameplay)
	{
		if(HasAuthority() && OverlappingWeapon)
		{
			// Stole weapon
			// if(OverlappingWeapon->WeaponState == EWeaponState::EWS_Equipped)
			// {
			// 	if(!IsLocallyControlled() && CombatComponent->EquippedWeapon != nullptr)
			// 	{
			// 		UE_LOG(LogTemp, Warning, TEXT("Weapon Stolen"));
			// 		CombatComponent->EquippedWeapon = nullptr;
			// 	}
			// 	OverlappingWeapon->WeaponState = EWeaponState::EWS_Dropped;
			// }
			CombatComponent->EquipWeapon(OverlappingWeapon);
		}
		else
		{
			ServerEquipButtonPressed();
		}
	}
}

/**
 * Is an RPC
 * */
void ABlasterCharacter::ServerEquipButtonPressed_Implementation()
{
	// if(CombatComponent) CombatComponent->EquipWeapon(OverlappingWeapon);
	if(CombatComponent && OverlappingWeapon) CombatComponent->EquipWeapon(OverlappingWeapon);
}

void ABlasterCharacter::CrouchButtonPressed()
{
	if(!bDisableGameplay)
	{
		if(!bIsCrouched) Crouch();
		else UnCrouch();
	}
}

void ABlasterCharacter::ReloadButtonPressed()
{
	if(!bDisableGameplay) CombatComponent->Reload();
}

void ABlasterCharacter::AimButtonPressed() {if(CombatComponent && !bDisableGameplay) CombatComponent->SetAiming(true);}

void ABlasterCharacter::AimButtonReleased()	{if(CombatComponent && !bDisableGameplay) CombatComponent->SetAiming(false);}

void ABlasterCharacter::AimOffset(float DeltaTime)
{
	if(CombatComponent && CombatComponent->EquippedWeapon == nullptr) return;
	
	float Speed = CalculateSpeed();
	bool bIsInAir = GetCharacterMovement()->IsFalling();
	
	// Manage the YAW
	if(Speed == 0.f && !bIsInAir) // Character is standing and not jumping
	{
		bRotateRootBone = true;
		FRotator CurrentAimRotation = FRotator(0, GetBaseAimRotation().Yaw, 0);
		FRotator DeltaAimRotation = UKismetMathLibrary::NormalizedDeltaRotator(CurrentAimRotation, StartingAimRotation);
		AO_Yaw = DeltaAimRotation.Yaw;
		if(TurningInPlace == ETurningInPlace::ETIP_NotTurning)
		{
			Interp_AO_Yaw = AO_Yaw;
		}
		
		// bUseControllerRotationYaw = true; // Useless ?
		TurnInPlace(DeltaTime);

	}

	if(Speed > 0.f || bIsInAir) // Character is running or jumping
	{
		bRotateRootBone = false;
		AO_Yaw = 0;
		StartingAimRotation = FRotator(0, GetBaseAimRotation().Yaw, 0);
		// bUseControllerRotationYaw = true; // Useless ?
		TurningInPlace = ETurningInPlace::ETIP_NotTurning;
	}

	CalculateAO_Pitch();
}

void ABlasterCharacter::CalculateAO_Pitch()
{
	// Manage the PITCH
	AO_Pitch = GetBaseAimRotation().Pitch;
	if (AO_Pitch > 90 && !IsLocallyControlled()) // Because Pitch is compressed by bitwise operation when it is sended to the server(for performence), so we need to fix it
	{
		// map pitch from [270, 360) to [-90, 0]
		FVector2D InRange(270, 360);
		FVector2D OutRange(-90, 0);
		AO_Pitch = FMath::GetMappedRangeValueClamped(InRange, OutRange, AO_Pitch);
	}
	// UE_LOG(LogTemp, Warning, TEXT("AO_PITCH %f"), AO_Pitch);
}

/**
 * @brief If we are a simulated proxy, we don't want to rotate the root bone, because it's cause Jitter, so we simplify the animation by just turning
 */
void ABlasterCharacter::SimProxiesTurn()
{
	if(!CombatComponent || !CombatComponent->EquippedWeapon) return;

	bRotateRootBone = false;

	if(CalculateSpeed() > 0)
	{
		TurningInPlace = ETurningInPlace::ETIP_NotTurning;
		return;
	}

	ProxyRotationLastFrame = ProxyRotation;
	ProxyRotation = GetActorRotation();
	ProxyYaw = UKismetMathLibrary::NormalizedDeltaRotator(ProxyRotation, ProxyRotationLastFrame).Yaw;

	if(FMath::Abs(ProxyYaw) > TurnThreshold)
	{
		if((ProxyYaw) > TurnThreshold)
		{
			TurningInPlace = ETurningInPlace::ETIP_Right;
		}
		else if((ProxyYaw) < -TurnThreshold)
		{
			TurningInPlace = ETurningInPlace::ETIP_Left;
		}
		else
		{
			TurningInPlace = ETurningInPlace::ETIP_NotTurning;
		}
		return;
	}
	TurningInPlace = ETurningInPlace::ETIP_NotTurning;
}
/**
 * @brief Replication on the client
 */
void ABlasterCharacter::OnRep_OverlappingWeapon(AWeapon* LastWeapon)
{
	if(OverlappingWeapon) OverlappingWeapon->ShowPickupWidget(true);
	if(LastWeapon) LastWeapon->ShowPickupWidget(false);
}

bool ABlasterCharacter::IsWeaponEquipped()
{
	return (CombatComponent && CombatComponent->EquippedWeapon);
}

bool ABlasterCharacter::IsAiming()
{
	return (CombatComponent && CombatComponent->bAiming);
}

/**
 * @brief Play fire animations
 * The FireWeapon Blueprint animation montage file is connected to the .... in the BlasterAnimBP file
 * It is also cached in the cache "WeaponSlot"
 */
void ABlasterCharacter::PlayFireMontage(bool bAiming)
{
	if(!CombatComponent || !CombatComponent->EquippedWeapon) return;
	
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if(!AnimInstance) {UE_LOG(LogTemp, Error, TEXT("BlasterCharacter.cpp: AnimInstance is null")); return;};
	if(!FireWeaponMontage) {UE_LOG(LogTemp, Error, TEXT("BlasterCharacter.cpp: FireWeaponMontage is null")); return;};

	AnimInstance->Montage_Play(FireWeaponMontage);
	FName SectionName = bAiming ? FName("RifleAim") : FName("RifleHip"); // Name are montages section we created in the FireWeapon Blueprint animation montage file
	AnimInstance->Montage_JumpToSection(SectionName);
}

void ABlasterCharacter::PlayReloadMontage()
{
	if(!CombatComponent || !CombatComponent->EquippedWeapon) return;
	
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if(!AnimInstance) {UE_LOG(LogTemp, Error, TEXT("BlasterCharacter.PlayReloadMontage: AnimInstance is null")); return;};
	if(!ReloadMontage) {UE_LOG(LogTemp, Error, TEXT("BlasterCharacter.PlayReloadMontage: ReloadMontage is null")); return;};

	AnimInstance->Montage_Play(ReloadMontage);
	FName SectionName;
	switch (CombatComponent->EquippedWeapon->GetWeaponType())
	{
		case EWeaponType::EWT_AssaultRifle:
			SectionName = FName("Rifle");
			break;
		default:
			break;
	}
	AnimInstance->Montage_JumpToSection(SectionName);
}

void ABlasterCharacter::PlayHitReactMontage()
{
	if(!CombatComponent || !CombatComponent->EquippedWeapon) return;
	
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if(!AnimInstance) {UE_LOG(LogTemp, Error, TEXT("BlasterCharacter.cpp: AnimInstance is null")); return;};
	if(!HitReactMontage) {UE_LOG(LogTemp, Error, TEXT("BlasterCharacter.cpp: HitReactMontage is null")); return;};

	AnimInstance->Montage_Play(HitReactMontage);
	// FName SectionName = bAiming ? FName("RifleAim") : FName("RifleHip"); // Name are montages section we created in the HitReact Blueprint animation montage file
	FName SectionName("FromFront");
	AnimInstance->Montage_JumpToSection(SectionName);
}

FVector ABlasterCharacter::GetHitTarget() const
{
	if(!CombatComponent) return FVector();
	return CombatComponent->HitTarget;
}

void ABlasterCharacter::HideCameraIfCharacterClose()
{
	if(!IsLocallyControlled() || !GetMesh()) return;
	if((FollowCamera->GetComponentLocation() - GetActorLocation()).Size() < CameraThreshold)
	{
		GetMesh()->SetVisibility(false);
		if(CombatComponent && CombatComponent->EquippedWeapon && CombatComponent->EquippedWeapon->GetWeaponMesh())
		{
			CombatComponent->EquippedWeapon->GetWeaponMesh()->bOwnerNoSee = true;
		}
	}
	else if(GetMesh())
	{
		GetMesh()->SetVisibility(true);
		if(CombatComponent && CombatComponent->EquippedWeapon && CombatComponent->EquippedWeapon->GetWeaponMesh())
		{
			CombatComponent->EquippedWeapon->GetWeaponMesh()->bOwnerNoSee = false;
		}
	}
}

void ABlasterCharacter::SetOverlappingWeapon(AWeapon* Weapon)
{
	if(OverlappingWeapon) // We use this to check if the pawn is controlled on the current machine (for the case we are on the server, to execute code also on the server, because a Notifie (OnRep_OverlappingWeapon) actually sent data only to the client owner of this instance class)
	{
		OverlappingWeapon->ShowPickupWidget(false);
	}

	OverlappingWeapon = Weapon;

	if(IsLocallyControlled() && OverlappingWeapon) // We use this to check if the pawn is controlled on the current machine (for the case we are on the server, to execute code also on the server, because a Notifie (OnRep_OverlappingWeapon) actually sent data only to the client owner of this instance class)
	{
		// if(GetEquippedWeapon() != OverlappingWeapon)
		// {
			OverlappingWeapon->ShowPickupWidget(true);
		// }
	}
}

AWeapon* ABlasterCharacter::GetEquippedWeapon()
{
	if(CombatComponent == nullptr) return nullptr;
	return CombatComponent->EquippedWeapon;
}

void ABlasterCharacter::TurnInPlace(float DeltaTime)
{
	// Just for start the animation
	if(AO_Yaw > 90)
	{
		TurningInPlace = ETurningInPlace::ETIP_Right;
	}
	else if(AO_Yaw < -90)
	{
		TurningInPlace = ETurningInPlace::ETIP_Left;
	}

	// For rotate the character
	if(TurningInPlace != ETurningInPlace::ETIP_NotTurning)
	{
		Interp_AO_Yaw = FMath::FInterpTo(Interp_AO_Yaw, 0, DeltaTime, 4);
		AO_Yaw = Interp_AO_Yaw;
		if(FMath::Abs(AO_Yaw) < 15) // Under 15 degrees, we stop turning, and reset some parameters
		{
			TurningInPlace = ETurningInPlace::ETIP_NotTurning;
			StartingAimRotation = FRotator(0, GetBaseAimRotation().Yaw, 0);
		}
	}
}

void ABlasterCharacter::Jump()
{
	if(bDisableGameplay) return;
	
	if(!bIsCrouched){ Super::Jump();}
	else UnCrouch();	
}

void ABlasterCharacter::FireButtonPressed()
{
	if(CombatComponent && !bDisableGameplay)
	{
		CombatComponent->FireButtonPressed(true);
	}
}

void ABlasterCharacter::FireButtonReleased()
{
	if(CombatComponent)
	{
		CombatComponent->FireButtonPressed(false);
	}
}

void ABlasterCharacter::OnRep_ReplicatedMovement()
{
	Super::OnRep_ReplicatedMovement();
	SimProxiesTurn();
	TimeSinceLastMovementReplication = 0;
}

float ABlasterCharacter::CalculateSpeed()
{
	FVector Velocity = GetVelocity();
    Velocity.Z = 0;
    return Velocity.Size();	
}

// Called only on clients
void ABlasterCharacter::OnRep_Health()
{
	PlayHitReactMontage();
	UpdateHUDHealth();
}

// Called only on server
void ABlasterCharacter::ReceiveDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType, class AController* InstigatorController, AActor* DamageCauser)
{
	Health = FMath::Clamp(Health - Damage, 0.f, MaxHealth);
	PlayHitReactMontage();
	UpdateHUDHealth();

	if(Health == 0)
	{
		ABlasterGameMode* BlasterGameMode = GetWorld()->GetAuthGameMode<ABlasterGameMode>();
		BlasterPlayerController = BlasterPlayerController == nullptr ? Cast<ABlasterPlayerController>(Controller) : BlasterPlayerController;
		ABlasterPlayerController* AttackerController = Cast<ABlasterPlayerController>(InstigatorController);
		
		if(!BlasterGameMode) {UE_LOG(LogTemp, Error, TEXT("BlasterCharacter.ReceiveDamage: BlasterGameMode is null")); return;}
		if(!BlasterPlayerController) {UE_LOG(LogTemp, Error, TEXT("BlasterCharacter.ReceiveDamage: BlasterPlayerController is null")); return;}

		BlasterGameMode->PlayerEliminated(this, BlasterPlayerController, AttackerController);	
	}
}

void ABlasterCharacter::UpdateHUDHealth()
{
	BlasterPlayerController = BlasterPlayerController == nullptr ? Cast<ABlasterPlayerController>(Controller) : BlasterPlayerController;
    if(!BlasterPlayerController) return;

	BlasterPlayerController->SetHUDHealth(Health, MaxHealth);
}

// Called only on the server
void ABlasterCharacter::Elim()
{
	MulticastElim();
	GetWorldTimerManager().SetTimer
	(
		ElimTimer,
		this,
		&ABlasterCharacter::ElimTimerFinished,
		ElimDelay
	);

	if(!CombatComponent) {UE_LOG(LogTemp, Error, TEXT("BlasterCharacter.cpp: CombatComponent is null")); return;}
    if(!CombatComponent->EquippedWeapon) {UE_LOG(LogTemp, Error, TEXT("BlasterCharacter.cpp: CombatComponent->EquippedWeapon is null")); return;}
	
	CombatComponent->EquippedWeapon->Dropped();
}

// Called on all machines
void ABlasterCharacter::MulticastElim_Implementation()
{
	if(BlasterPlayerController)
	{
		BlasterPlayerController->SetHUDWeaponAmmo(0);
	}
	
	bElimned = true;
	PlayElimMontage();

	// Start dissole effect
	if(DissolveMaterialInstance) 
	{
		DynamicDissolveMaterialInstance = UMaterialInstanceDynamic::Create(DissolveMaterialInstance, this);
		GetMesh()->SetMaterial(0, DynamicDissolveMaterialInstance);
		DynamicDissolveMaterialInstance->SetScalarParameterValue(TEXT("Dissolve"), 0.55f);
		DynamicDissolveMaterialInstance->SetScalarParameterValue(TEXT("Glow"), 50.f);
		StartDissolve();
	}
	else {UE_LOG(LogTemp, Error, TEXT("BlasterCharacter.MulticastElim_Implementation: DissolveMaterialInstance is null")); }
	
	// Disable character controls
	GetCharacterMovement()->DisableMovement(); // Prevent to move
	GetCharacterMovement()->StopMovementImmediately(); // Prevent to rotate
	
	if(BlasterPlayerController) 
	{
		// DisableInput(BlasterPlayerController); // Everything is disabled, we choose now to customize what we disable with bDisableGameplay.
		bDisableGameplay = true;
	}
	else { UE_LOG(LogTemp, Error, TEXT("BlasterCharacter.MulticastElim_Implementation: BlasterPlayerController is null")); }
	
	if(CombatComponent) CombatComponent->FireButtonPressed(false);

	// Disable collision
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// Spawn elim bot
	if(ElimBotEffect && ElimBotSound && ElimBotRandomSound)
	{
		FVector ElimBotSpawnPoint(GetActorLocation().X, GetActorLocation().Y, GetActorLocation().Z + 200.f);
		ElimBotComponent = UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ElimBotEffect, ElimBotSpawnPoint, GetActorRotation());
		
		if(FMath::RandRange(1, 5) == 1)
		{
			UGameplayStatics::SpawnSoundAtLocation(this, ElimBotRandomSound, GetActorLocation());
		}
		
		UGameplayStatics::SpawnSoundAtLocation(this, ElimBotSound, GetActorLocation());
	}
	else { UE_LOG(LogTemp, Error, TEXT("BlasterCharacter.MulticastElim_Implementation: ElimBotEffect OR ElimBotSound OR ElimBotRandomSound is null")); }
	
}

void ABlasterCharacter::PlayElimMontage()
{
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if(!AnimInstance) {UE_LOG(LogTemp, Error, TEXT("BlasterCharacter.cpp: AnimInstance is null")); return;};
	if(!ElimMontage) {UE_LOG(LogTemp, Error, TEXT("BlasterCharacter.cpp: ElimMontage is null")); return;};

	AnimInstance->Montage_Play(ElimMontage);
}

void ABlasterCharacter::ElimTimerFinished()
{
	ABlasterGameMode* BlasterGameMode = GetWorld()->GetAuthGameMode<ABlasterGameMode>();
	if(!BlasterGameMode) {UE_LOG(LogTemp, Error, TEXT("BlasterCharacter.cpp: BlasterGameMode is null")); return;}
	if(!Controller) {UE_LOG(LogTemp, Error, TEXT("BlasterCharacter.cpp: BlasterGameMode is null")); return;}

	BlasterGameMode->RequestRespawn(this, Controller);
}

void ABlasterCharacter::Destroyed()
{
	Super::Destroyed();
	
	if (ElimBotComponent) 
	{
		ElimBotComponent->DestroyComponent();
	}

	ABlasterGameMode* BlasterGameMode = Cast<ABlasterGameMode>(UGameplayStatics::GetGameMode(this));
	if(BlasterGameMode && BlasterGameMode->GetMatchState() != MatchState::InProgress && CombatComponent && CombatComponent->EquippedWeapon)
	{
		CombatComponent->EquippedWeapon->Destroy();
	}
}

/**
 * @param DissolveValue Get from the curve
 */
void ABlasterCharacter::UpdateDissolveMaterial(float DissolveValue) 
{
	if(!DynamicDissolveMaterialInstance) {UE_LOG(LogTemp, Error, TEXT("BlasterCharacter.cpp: DynamicDissolveMaterialInstance is null")); return;}
	DynamicDissolveMaterialInstance->SetScalarParameterValue(TEXT("Dissolve"), DissolveValue);
	// DynamicDissolveMaterialInstance->SetScalarParameterValue(TEXT("Glow"), DissolveValue);

}

void ABlasterCharacter::StartDissolve()
{
	// Bind the callback UpdateDissolveMaterial
	DissolveTrack.BindDynamic(this, &ABlasterCharacter::UpdateDissolveMaterial);
	
	if(!DissolveCurve) {UE_LOG(LogTemp, Error, TEXT("BlasterCharacter.cpp: DissolveCurve is null")); return;}
	if(!TimelineDissolve) {UE_LOG(LogTemp, Error, TEXT("BlasterCharacter.cpp: TimelineDissolve is null")); return;}

	TimelineDissolve->AddInterpFloat(DissolveCurve, DissolveTrack); // Prepare the timeline with his curve and his track
	TimelineDissolve->Play();
	
}

ECombatState ABlasterCharacter::GetCombatState()
{
	if(!CombatComponent) return ECombatState::ECS_MAX;
	return CombatComponent->CombatState;
}