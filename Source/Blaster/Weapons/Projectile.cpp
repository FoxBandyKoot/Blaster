// Fill out your copyright notice in the Description page of Project Settings.


#include "Projectile.h"
#include "Blaster/Blaster.h"
#include "Sound/SoundCue.h"
#include "Kismet/GameplayStatics.h"
#include "Components/BoxComponent.h"
#include "Particles/ParticleSystem.h"
#include "Particles/ParticleSystemComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"

#include "Blaster/Characters/BlasterCharacter.h"

AProjectile::AProjectile()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;

	CollisionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("CollisionBox"));
	SetRootComponent(CollisionBox);
	CollisionBox->SetCollisionObjectType(ECollisionChannel::ECC_WorldDynamic);
	CollisionBox->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	CollisionBox->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	CollisionBox->SetCollisionResponseToChannel(ECollisionChannel::ECC_Visibility, ECollisionResponse::ECR_Block);
	CollisionBox->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldStatic, ECollisionResponse::ECR_Block);
	CollisionBox->SetCollisionResponseToChannel(ECC_SkeletalMesh, ECollisionResponse::ECR_Block);

	ProjectileMovementComponent = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovementComponent"));
	ProjectileMovementComponent->bRotationFollowsVelocity = true;
}

void AProjectile::BeginPlay()
{
	Super::BeginPlay();

	if(!Tracer){UE_LOG(LogTemp, Error, TEXT("Projectile.cpp: Tracer is null")); return;}

	TracerComponent = UGameplayStatics::SpawnEmitterAttached
	(
		Tracer,
		CollisionBox, 
		FName(""), // If we want to attach it to a bone
		GetActorLocation(),
		GetActorRotation(),
		EAttachLocation::KeepWorldPosition
	);
	
	if(!HasAuthority()) return;
	CollisionBox->OnComponentHit.AddDynamic(this, &AProjectile::OnHit);
	
}

void AProjectile::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AProjectile::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	Destroy();
}

void AProjectile::Destroyed()
{
	Super::Destroyed(); // Is broadcasted

	if(!ImpactParticles) {UE_LOG(LogTemp, Error, TEXT("Projectile.cpp: ImpactParticles is null")); return;}
	if(!ImpactSound) {UE_LOG(LogTemp, Error, TEXT("Projectile.cpp: ImpactSound is null")); return;}
	
	UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ImpactParticles, GetActorTransform());
	UGameplayStatics::PlaySoundAtLocation(this, ImpactSound, GetActorLocation());
}