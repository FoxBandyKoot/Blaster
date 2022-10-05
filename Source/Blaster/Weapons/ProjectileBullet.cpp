// Fill out your copyright notice in the Description page of Project Settings.


#include "ProjectileBullet.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Character.h"

void AProjectileBullet::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
    ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
    if(!OwnerCharacter) {UE_LOG(LogTemp, Error, TEXT("ProjectileBullet.cpp: OwnerCharacter is null")); return;}

    AController* OwnerController = OwnerCharacter->Controller;
    if(!OwnerController) {UE_LOG(LogTemp, Error, TEXT("ProjectileBullet.cpp: OwnerController is null")); return;}
    
    UGameplayStatics::ApplyDamage(OtherActor, Damage, OwnerController, this, UDamageType::StaticClass());
    Super::OnHit(HitComp, OtherActor, OtherComp, NormalImpulse, Hit);
}
