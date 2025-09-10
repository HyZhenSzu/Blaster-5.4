// Fill out your copyright notice in the Description page of Project Settings.


#include "Casing.h"
#include "Components/StaticMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundCue.h"

ACasing::ACasing()
{
	PrimaryActorTick.bCanEverTick = true;

    CasingMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CasingMesh"));
    SetRootComponent(CasingMesh);
    CasingMesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Block);
    CasingMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Ignore);
    CasingMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);
    CasingMesh->SetSimulatePhysics(true);
    CasingMesh->SetEnableGravity(true);
    CasingMesh->SetNotifyRigidBodyCollision(true);
    ShellEjectionImpulse = 10.f;
}

void ACasing::BeginPlay()
{
	Super::BeginPlay();
    CasingMesh->OnComponentHit.AddDynamic(this, &ACasing::OnHit);
    CasingMesh->AddImpulse(GetActorForwardVector() * ShellEjectionImpulse);
}

void ACasing::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
    if (CasingSound)
    {
        UGameplayStatics::PlaySoundAtLocation(this, CasingSound, GetActorLocation());
    }
    Destroy();
}

void ACasing::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

