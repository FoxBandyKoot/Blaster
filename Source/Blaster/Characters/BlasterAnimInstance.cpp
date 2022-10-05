// Fill out your copyright notice in the Description page of Project Settings.


#include "BlasterAnimInstance.h"
#include "BlasterCharacter.h"
#include "../Weapons/Weapon.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetMathLibrary.h" 

/**
 * @brief Constructor of the animation
 * 
 */
void UBlasterAnimInstance::NativeInitializeAnimation()
{
    Super::NativeInitializeAnimation();

    BlasterCharacter = Cast<ABlasterCharacter>(TryGetPawnOwner());
}


/**
 * @brief Tick of the animation
 * 
 * @param DeltaTime 
 */
void UBlasterAnimInstance::NativeUpdateAnimation(float DeltaTime)
{
    Super::NativeUpdateAnimation(DeltaTime);

    if(!BlasterCharacter)
    {
        BlasterCharacter = Cast<ABlasterCharacter>(TryGetPawnOwner());
    }

    if(!BlasterCharacter) return;

    FVector Velocity = BlasterCharacter->GetVelocity();
    Velocity.Z = 0;
    Speed = Velocity.Size();

    bIsInAir = BlasterCharacter->GetCharacterMovement()->IsFalling();
    bIsAccelerating = BlasterCharacter->GetCharacterMovement()->GetCurrentAcceleration().Size() > 0 ? true : false;
    bWeaponEquipped = BlasterCharacter->IsWeaponEquipped();
    EquippedWeapon = BlasterCharacter->GetEquippedWeapon();
    bIsCrounched = BlasterCharacter->bIsCrouched;
    bIsAiming = BlasterCharacter->IsAiming();
    TurningInPlace = BlasterCharacter->GetETurningInPlace();
    bRotateRootBone = BlasterCharacter->ShouldRotateRootBone();
    bElimned = BlasterCharacter->IsElimned();

    // 1 - OffsetYaw for straffing
    FRotator AimRotation = BlasterCharacter->GetBaseAimRotation();
    FRotator MovementRotation = UKismetMathLibrary::MakeRotFromX(BlasterCharacter->GetVelocity());
    FRotator DeltaRot = UKismetMathLibrary::NormalizedDeltaRotator(MovementRotation, AimRotation);

    // 2 - Create a smoothly interpolation between rotators for straffing
    DeltaRotation = FMath::RInterpTo(DeltaRotation, DeltaRot, DeltaTime, 6); // The final value is assigned by the "shortest path", for example, to -180 to 180, go directly to 180
    YawOffset = DeltaRotation.Yaw;

    // Lean 
    CharacterRotationLastFrame = CharacterRotation;
    CharacterRotation = BlasterCharacter->GetActorRotation();
    const FRotator Delta = UKismetMathLibrary::NormalizedDeltaRotator(CharacterRotation, CharacterRotationLastFrame);
    const float Target = Delta.Yaw / DeltaTime; // Scale up the value && make it proportional to DeltaTime
    const float Interp = FMath::FInterpTo(Lean, Target, DeltaTime, 6);
    Lean = FMath::Clamp(Interp, -90, 90); // Constrain the value between -90 & 90;

    // Update Yaw and pitch offsets while aiming or not
    AO_Yaw = BlasterCharacter->GetAO_Yaw();
    AO_Pitch = BlasterCharacter->GetAO_Pitch();

    // Update hands transform
    if(bWeaponEquipped && EquippedWeapon && EquippedWeapon->GetWeaponMesh() && BlasterCharacter->GetMesh())
    {
        // Update left hand transform
        LeftHandTransform = EquippedWeapon->GetWeaponMesh()->GetSocketTransform(FName("LeftHandSocket"), ERelativeTransformSpace::RTS_World);
        FVector OutPosition;
        FRotator OutRotation;
        // We want the socket location on the weapon relative to the right hand
        BlasterCharacter->GetMesh()->TransformToBoneSpace(FName("Hand_R"), LeftHandTransform.GetLocation(), FRotator::ZeroRotator, OutPosition, OutRotation);
        LeftHandTransform.SetLocation(OutPosition);
        LeftHandTransform.SetRotation(FQuat(OutRotation));

        // Update right hand transform to align the weapon to the crosshair, we will do this only on the character tht is locally controlled, because others players don't see his crosshair, so they don't care
        if(BlasterCharacter->IsLocallyControlled())
        {
            bIsLocallyControlled = true;
            FTransform RightHandTransform = EquippedWeapon->GetWeaponMesh()->GetSocketTransform(FName("Hand_R"), ERelativeTransformSpace::RTS_World);
            
            // Calcul the angle between the 2 next debug lines
            // The X axis of the hand is reverted on the character, so we do (RightHandTransform.GetLocation() - BlasterCharacter->GetHitTarget())
            FRotator LookAtRotation = UKismetMathLibrary::FindLookAtRotation(RightHandTransform.GetLocation(), RightHandTransform.GetLocation() + (RightHandTransform.GetLocation() - BlasterCharacter->GetHitTarget()));
            
            // Move smoothly the right hand
            RightHandRotation = FMath::RInterpTo(RightHandRotation, LookAtRotation, DeltaTime, 15.f);
        }

        // For debugging
        FTransform MuzzleTipTransform = EquippedWeapon->GetWeaponMesh()->GetSocketTransform(FName("MuzzleFlash"), ERelativeTransformSpace::RTS_World);
        FVector MuzzleX(FRotationMatrix(MuzzleTipTransform.GetRotation().Rotator()).GetUnitAxis(EAxis::X));
    
        DrawDebugLine(GetWorld(), MuzzleTipTransform.GetLocation(), MuzzleX * 1000, FColor::Red);
        DrawDebugLine(GetWorld(), MuzzleTipTransform.GetLocation(), BlasterCharacter->GetHitTarget(), FColor::Orange);
        
        // DrawDebugSphere(GetWorld(), MuzzleX * 1000, 16.f, 12, FColor::Red);
        // DrawDebugSphere(GetWorld(), BlasterCharacter->GetHitTarget(), 16.f, 12, FColor::Orange);
    }
    
    bUseFABRIK = BlasterCharacter->GetCombatState() != ECombatState::ECS_Reloading;
    bUseAimOffsets = BlasterCharacter->GetCombatState() != ECombatState::ECS_Reloading && !BlasterCharacter->GetDisableGameplay();
    bTransformRightHand = BlasterCharacter->GetCombatState() != ECombatState::ECS_Reloading && !BlasterCharacter->GetDisableGameplay();
}