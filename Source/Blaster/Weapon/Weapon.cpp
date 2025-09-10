// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon.h"
#include "Components/SphereComponent.h"
#include "Components/WidgetComponent.h"
#include "Blaster/Character/BlasterCharacter.h"
#include "Net/UnrealNetwork.h"
#include "Animation/AnimationAsset.h"
#include "Components/SkeletalMeshComponent.h"
#include "Casing.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Blaster/PlayerController/BlasterPlayerController.h"

// Sets default values
AWeapon::AWeapon()
{
	PrimaryActorTick.bCanEverTick = false;
    bReplicates = true;
    SetReplicateMovement(true);

    WeaponMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("WeaponMesh"));
    SetRootComponent(WeaponMesh);

    WeaponMesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Block);
    WeaponMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Ignore);
    WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    AreaSphere = CreateDefaultSubobject<USphereComponent>(TEXT("AreaSphere"));
    AreaSphere->SetupAttachment(RootComponent);
    AreaSphere->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
    AreaSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    PickupWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("PickupWidget"));
    PickupWidget->SetupAttachment(RootComponent);
}

void AWeapon::BeginPlay()
{
	Super::BeginPlay();

    if (PickupWidget)
    {
        PickupWidget->SetVisibility(false);
    }

    if (HasAuthority())
    {
        AreaSphere->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
        AreaSphere->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Overlap);
        AreaSphere->OnComponentBeginOverlap.AddDynamic(this, &AWeapon::OnSphereOverlop);
        AreaSphere->OnComponentEndOverlap.AddDynamic(this, &AWeapon::EndSphereOverlap);
    }
}

void AWeapon::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

}

void AWeapon::OnSphereOverlop(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
    ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(OtherActor);
    if (BlasterCharacter)
    {
        BlasterCharacter->SetOverlappingWeapon(this);
    }
}

void AWeapon::EndSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
    ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(OtherActor);
    if (BlasterCharacter)
    {
        BlasterCharacter->SetOverlappingWeapon(nullptr);
    }
}

void AWeapon::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(AWeapon, WeaponState);
    DOREPLIFETIME(AWeapon, Ammo);
}

void AWeapon::OnRep_Owner()
{
    Super::OnRep_Owner();
    /*
    *  We need to refresh the Ammo Text HUD when the character equipped a weapon and set a new owner.
    */
    if (Owner == nullptr)
    {
        OwnerCharacter = nullptr;
        OwnerController = nullptr;
    }
    else
        SetHUDAmmo();
}

void AWeapon::OnRep_WeaponState()
{
    switch (WeaponState)
    {
    case EWeaponState::EWS_Equipped:
        ShowPickupWidget(false);
        WeaponMesh->SetSimulatePhysics(false);
        WeaponMesh->SetEnableGravity(false);
        WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        break;
    case EWeaponState::EWS_Dropped:
        WeaponMesh->SetSimulatePhysics(true);
        WeaponMesh->SetEnableGravity(true);
        WeaponMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
        break;
    }
}

void AWeapon::OnRep_Ammo()
{
    SetHUDAmmo();
}

void AWeapon::SetWeaponState(EWeaponState State)
{
    WeaponState = State;
    switch (WeaponState)
    {
    case EWeaponState::EWS_Equipped:
        ShowPickupWidget(false);
        AreaSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        WeaponMesh->SetSimulatePhysics(false);
        WeaponMesh->SetEnableGravity(false);
        WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        break;
    case EWeaponState::EWS_Dropped:
        if (HasAuthority())
        {
            AreaSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
        }
        // After dropping the weapon, should enable the physics and gravity of it, 
        // so it can lap to the ground.
        WeaponMesh->SetSimulatePhysics(true);
        WeaponMesh->SetEnableGravity(true);
        // recover the collision so that the weapon can be equipped.
        WeaponMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
        break;
    }
}

bool AWeapon::IsAmmoEmpty()
{
    return Ammo <= 0;
}

void AWeapon::ShowPickupWidget(bool bShowPickupWidget)
{
    if (PickupWidget)
    {
        PickupWidget->SetVisibility(bShowPickupWidget);
    }
}

void AWeapon::Fire(const FVector& HitTarget)
{
    if (FireAnimation)
    {
        WeaponMesh->PlayAnimation(FireAnimation, false);
    }
    if (CasingClass)
    {
        APawn* InstigatorPawn = Cast<APawn>(GetOwner());
        const USkeletalMeshSocket* AmmoEjectSocket = WeaponMesh->GetSocketByName(FName("MuzzleFlash"));
        if (AmmoEjectSocket)
        {
            FTransform SocketTransform = AmmoEjectSocket->GetSocketTransform(WeaponMesh);
            UWorld* World = GetWorld();
            if (World)
            {
                World->SpawnActor<ACasing>(
                    CasingClass,
                    SocketTransform.GetLocation(),
                    SocketTransform.GetRotation().Rotator()
                );
            }
        }
    }
    SpendRound();
}

void AWeapon::SpendRound()
{
    // When fire, spend Ammo.
    Ammo = FMath::Clamp(Ammo - 1, 0, MagCapacity);

    // Refresh the HUD when change Ammo.
    SetHUDAmmo();
}

void AWeapon::SetHUDAmmo()
{
    OwnerCharacter = OwnerCharacter == nullptr ? Cast<ABlasterCharacter>(GetOwner()) : OwnerCharacter;
    if (OwnerCharacter)
    {
        OwnerController = OwnerController == nullptr ? Cast<ABlasterPlayerController>(OwnerCharacter->Controller) : OwnerController;
        if (OwnerController)
        {
            OwnerController->SetHUDAmmoValue(Ammo);
        }
    }
}

void AWeapon::Dropped()
{
    SetWeaponState(EWeaponState::EWS_Dropped);
    FDetachmentTransformRules DetachRules(EDetachmentRule::KeepWorld, true);
    WeaponMesh->DetachFromComponent(DetachRules);
    SetOwner(nullptr);
    OwnerCharacter = nullptr;
    OwnerController = nullptr;
}

void AWeapon::ReloadAmmo(int32 ReloadAmount)
{
    Ammo = FMath::Clamp(Ammo - ReloadAmount, 0, MagCapacity);
    SetHUDAmmo();
}

