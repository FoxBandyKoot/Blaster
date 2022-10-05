// Fill out your copyright notice in the Description page of Project Settings.


#include "ProjectileWeapon.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Projectile.h"

void AProjectileWeapon::Fire(const FVector& HitTarget)
{
    if(!ProjectileClass){UE_LOG(LogTemp, Error, TEXT("ProjectileWeapon.cpp: ProjectileClass is null")); return;}
    
    APawn* InstigatorPawn = Cast<APawn>(GetOwner());
    if(!InstigatorPawn) {UE_LOG(LogTemp, Error, TEXT("ProjectileWeapon.cpp: InstigatorPawn is null")); return;}
    
    UWorld* World = GetWorld();
    if(!World) {UE_LOG(LogTemp, Error, TEXT("ProjectileWeapon.cpp: World is null")); return;}

    const USkeletalMeshSocket* MuzzleFlashSocket = GetWeaponMesh()->GetSocketByName("MuzzleFlash");
    if(!MuzzleFlashSocket) {UE_LOG(LogTemp, Error, TEXT("ProjectileWeapon.cpp: MuzzleFlashSocket is null")); return;} 

    Super::Fire(HitTarget);

    if(!HasAuthority()) return;
    
    FTransform SocketTranform = MuzzleFlashSocket->GetSocketTransform(GetWeaponMesh());
    FVector ToTarget = HitTarget - SocketTranform.GetLocation();
    FRotator ToTargetRotation = ToTarget.Rotation();
    
    FActorSpawnParameters SpawnParams;
    SpawnParams.Owner = GetOwner();
    SpawnParams.Instigator = InstigatorPawn;

    World->SpawnActor<AProjectile>
    (
        ProjectileClass,
        SocketTranform.GetLocation(),
        ToTargetRotation,
        // SocketTranform.GetRotation().Rotator(),
        SpawnParams
    );
}
