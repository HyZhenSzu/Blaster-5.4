// Fill out your copyright notice in the Description page of Project Settings.


#include "RocketProjectile.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "RocketMovementComponent.h"

ARocketProjectile::ARocketProjectile()
{
    RocketMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Rocket Mesh"));
    RocketMesh->SetupAttachment(RootComponent);
    RocketMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    RocketMovementComponent = CreateDefaultSubobject<URocketMovementComponent>(TEXT("RocketMovementComponent"));
    RocketMovementComponent->bRotationFollowsVelocity = true;
    RocketMovementComponent->SetIsReplicated(true);
}

void ARocketProjectile::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
    if (OtherActor == GetOwner())
    {
        return;
    }
    APawn* FiringPawn = GetInstigator();
    if (FiringPawn)
    {
        AController* FiringController = FiringPawn->GetController();
        if (FiringController)
        {
            UGameplayStatics::ApplyRadialDamageWithFalloff(
                this, // world context object
                Damage, // base damage
                10.f, // minimum damage
                GetActorLocation(), // origin location
                200.f, // damage inner radius
                500.f, // damage outer radius
                1.f, // damage fall off,
                UDamageType::StaticClass(), // damage type class
                TArray<AActor*>(), // ingore Actors
                this, // damage causer
                FiringController // instigator controller
            );
        }
    }

    Super::OnHit(HitComp, OtherActor, OtherComp, NormalImpulse, Hit);
}
