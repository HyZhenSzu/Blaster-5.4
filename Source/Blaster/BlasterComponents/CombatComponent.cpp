// Fill out your copyright notice in the Description page of Project Settings.


#include "CombatComponent.h"
#include "Blaster/Weapon/Weapon.h"
#include "Blaster/Character/BlasterCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Net/UnrealNetwork.h"
#include "Kismet/GameplayStatics.h"
#include "Blaster/PlayerController/BlasterPlayerController.h"
#include "Camera/CameraComponent.h"
#include "TimerManager.h"
#include "Sound/SoundCue.h"

UCombatComponent::UCombatComponent()
{
	PrimaryComponentTick.bCanEverTick = true;

    BaseWalkSpeed = 800.f;
    AimWalkSpeed = 450.f;
}

void UCombatComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(UCombatComponent, EquippedWeapon);
    DOREPLIFETIME(UCombatComponent, bAiming);
    DOREPLIFETIME_CONDITION(UCombatComponent, CarriedAmmo, COND_OwnerOnly);
    DOREPLIFETIME(UCombatComponent, CombatState);
}



void UCombatComponent::OnRep_CarriedAmmo()
{
    Controller = Controller == nullptr ? Cast<ABlasterPlayerController>(Character->Controller) : Controller;
    if (Controller)
    {
        Controller->SetHUDCarriedAmmo(CarriedAmmo);
    }
}

void UCombatComponent::InitializeCarriedAmmo()
{
    CarriedAmmoMap.Emplace(EWeaponType::EWT_AssaultRifle, StartingARAmmo);
    CarriedAmmoMap.Emplace(EWeaponType::EWT_RocketLancher, StartingRocketAmmo);
}


void UCombatComponent::BeginPlay()
{
	Super::BeginPlay();
    if (Character)
    {
        Character->GetCharacterMovement()->MaxWalkSpeed = BaseWalkSpeed;
        if (Character->GetFollowCamera())
        {
            DefaultFOV = Character->GetFollowCamera()->FieldOfView;
            CurrentFOV = DefaultFOV;
        }
        if (Character->HasAuthority())
        {
            InitializeCarriedAmmo();
        }
    }
}

void UCombatComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (Character && Character->IsLocallyControlled())
    {
        FHitResult HitResult;
        TraceUnderCrosshairs(HitResult);
        HitTarget = HitResult.ImpactPoint;

        SetHUDCrosshairs(DeltaTime);
        InterpFOV(DeltaTime);
    }
}

void UCombatComponent::SetHUDCrosshairs(float DeltaTime)
{
    if (Character == nullptr || Character->Controller == nullptr) return;
    Controller = Controller == nullptr ? Cast<ABlasterPlayerController>(Character->Controller) : Controller;
    if (Controller)
    {
        HUD = HUD == nullptr ? Cast<ABlasterHUD>(Controller->GetHUD()) : HUD;
        if (HUD)
        {
            if (EquippedWeapon)
            {
                Package.CrosshairsCenter = EquippedWeapon->CrosshairsCenter;
                Package.CrosshairsLeft = EquippedWeapon->CrosshairsLeft;
                Package.CrosshairsRight = EquippedWeapon->CrosshairsRight;
                Package.CrosshairsTop = EquippedWeapon->CrosshairsTop;
                Package.CrosshairsBottom = EquippedWeapon->CrosshairsBottom;
            }
            else
            {
                Package.CrosshairsCenter = nullptr;
                Package.CrosshairsLeft = nullptr;
                Package.CrosshairsRight = nullptr;
                Package.CrosshairsTop = nullptr;
                Package.CrosshairsBottom = nullptr;
            }
            //
            // calculating the crosshair spread
            // mapping [0, 600) -> [0, 1)
            //
            FVector2D WalkSpeedRange(0.f, Character->GetCharacterMovement()->MaxWalkSpeed);
            FVector2D CrosshairSpreadRange(0.f, 1.f);
            FVector Velocity = Character->GetVelocity();
            Velocity.Z = 0.f;
            CrosshairVelocityFactor = FMath::GetMappedRangeValueClamped(WalkSpeedRange, CrosshairSpreadRange, Velocity.Size());

            // setting the InAir Factor
            if (Character->GetCharacterMovement()->IsFalling())
            {
                CrosshairInAirFactor = FMath::FInterpTo(CrosshairInAirFactor, 2.25f, DeltaTime, 2.25f);
            }
            else
            {
                CrosshairInAirFactor = FMath::FInterpTo(CrosshairInAirFactor, 0.f, DeltaTime, 30.f);
            }

            // setting the Aiming Factor
            if (bAiming)
            {
                CrosshairAimingFactor = FMath::FInterpTo(CrosshairAimingFactor, 0.62f, DeltaTime, 30.f);
            }
            else
            {
                CrosshairAimingFactor = FMath::FInterpTo(CrosshairAimingFactor, 0.f, DeltaTime, 30.f);
            }

            // recover the shooting Factor back to 0
            CrosshairShootingFactor = FMath::FInterpTo(CrosshairShootingFactor, 0.f, DeltaTime, 30.f);

            Package.CrosshairsSpread =
                0.5f
                + CrosshairVelocityFactor
                + CrosshairInAirFactor
                - CrosshairAimingFactor
                + CrosshairShootingFactor;

            HUD->SetHUDPackage(Package);
        }
    }

}


void UCombatComponent::InterpFOV(float DeltaTime)
{
    // To handling the Interp of FOV

    if (!EquippedWeapon) return;
    if (bAiming)
    {
        CurrentFOV = FMath::FInterpTo(CurrentFOV, EquippedWeapon->GetZoomedFOV(), DeltaTime, EquippedWeapon->GetZoomInterpSpeed());
    }
    else
    {
        CurrentFOV = FMath::FInterpTo(CurrentFOV, DefaultFOV, DeltaTime, ZoomInterpSpeed);
    }
    if (Character && Character->GetFollowCamera())
    {
        Character->GetFollowCamera()->SetFieldOfView(CurrentFOV);
    }
}

void UCombatComponent::EquipWeapon(AWeapon* WeaponEquipped)
{
    if (Character == nullptr || WeaponEquipped == nullptr) return;
    if (EquippedWeapon)
    {
        EquippedWeapon->Dropped();
    }
    EquippedWeapon = WeaponEquipped;
    EquippedWeapon->SetWeaponState(EWeaponState::EWS_Equipped);
    const USkeletalMeshSocket* HandSocket = Character->GetMesh()->GetSocketByName(FName("RightHandSocket"));
    if (HandSocket)
    {
        HandSocket->AttachActor(EquippedWeapon, Character->GetMesh());
    }

    if (EquippedWeapon->EquipSound)
    {
        UGameplayStatics::PlaySoundAtLocation(
            this,
            EquippedWeapon->EquipSound,
            Character->GetActorLocation()
        );
    }

    EquippedWeapon->SetOwner(Character);
    EquippedWeapon->SetHUDAmmo();

    // Set CarriedAmmo when carrying a weapon.
    if (CarriedAmmoMap.Contains(EquippedWeapon->GetWeaponType()))
    {
        CarriedAmmo = CarriedAmmoMap[EquippedWeapon->GetWeaponType()];
    }

    if (EquippedWeapon->IsAmmoEmpty())
    {
        Reload();
    }

    Controller = Controller == nullptr ? Cast<ABlasterPlayerController>(Character->Controller) : Controller;
    if (Controller)
    {
        Controller->SetHUDCarriedAmmo(CarriedAmmo);
    }

    Character->GetCharacterMovement()->bOrientRotationToMovement = false;
    Character->bUseControllerRotationYaw = true;
}

void UCombatComponent::OnRep_EquippedWeapon()
{
    if (EquippedWeapon && Character)
    {
        EquippedWeapon->SetWeaponState(EWeaponState::EWS_Equipped);
        const USkeletalMeshSocket* HandSocket = Character->GetMesh()->GetSocketByName(FName("RightHandSocket"));
        if (HandSocket)
        {
            HandSocket->AttachActor(EquippedWeapon, Character->GetMesh());
        }

        if (EquippedWeapon->EquipSound)
        {
            UGameplayStatics::PlaySoundAtLocation(
                this,
                EquippedWeapon->EquipSound,
                Character->GetActorLocation()
            );
        }

        Character->GetCharacterMovement()->bOrientRotationToMovement = false;
        Character->bUseControllerRotationYaw = true;
    }
}

void UCombatComponent::FireButtonPressed(bool bPressed)
{
    if (EquippedWeapon == nullptr) return;

    bFireButtonPressed = bPressed;
    if (bFireButtonPressed)
    {
        Fire();
    }
}

void UCombatComponent::Fire()
{
    if (CanFire())
    {
        bCanFire = false;
        ServerFire(HitTarget);
        if (EquippedWeapon)
        {
            CrosshairShootingFactor = .55f;
        }
        StartFireTimer();
    }
}

void UCombatComponent::StartFireTimer()
{
    if (EquippedWeapon == nullptr || Character == nullptr) return;
    Character->GetWorldTimerManager().SetTimer(
        FireTimer,
        this,
        &UCombatComponent::FireTimerFinished,
        EquippedWeapon->FireDelay
    );
}

void UCombatComponent::FireTimerFinished()
{
    if (EquippedWeapon == nullptr) return;
    bCanFire = true;
    if (bFireButtonPressed && EquippedWeapon->bAutomatic)
    {
        Fire();
    }
    if (EquippedWeapon->IsAmmoEmpty())
    {
        Reload();
    }
}

bool UCombatComponent::CanFire()
{
    if (EquippedWeapon == nullptr) return false;
    return !EquippedWeapon->IsAmmoEmpty() && bCanFire && CombatState == ECombatState::ECS_Unoccupied;
}


void UCombatComponent::ServerFire_Implementation(const FVector_NetQuantize& TargetHitTarget)
{
    MulticastFire(TargetHitTarget);
}

void UCombatComponent::MulticastFire_Implementation(const FVector_NetQuantize& TargetHitTarget)
{
    if (EquippedWeapon == nullptr) return;
    if (Character && CombatState == ECombatState::ECS_Unoccupied)
    {
        Character->PlayFireMontage(bAiming);
        EquippedWeapon->Fire(TargetHitTarget);
    }
}

void UCombatComponent::TraceUnderCrosshairs(FHitResult& TraceHitResult)
{
    // Trace from the center of the screen
    
    // Get size of viewport
    FVector2D ViewportSize;
    if (GEngine && GEngine->GameViewport)
    {
        GEngine->GameViewport->GetViewportSize(ViewportSize);
    }
    // the center point's location is the crosshair's location
    FVector2D CrosshairLocation(ViewportSize.X / 2.f, ViewportSize.Y / 2.f);

    // Use the DeprojectScreenToWorld function in namespace "UGameplayStatic"
    // to change the point in the screen to a WorldPosition for our using
    FVector CrosshairWorldPosition;
    FVector CrosshairWorldDirection;
    bool bScreenToWorld = UGameplayStatics::DeprojectScreenToWorld(
        UGameplayStatics::GetPlayerController(this, 0),
        CrosshairLocation,
        CrosshairWorldPosition,
        CrosshairWorldDirection
    );

    if (bScreenToWorld)
    {
        FVector Start = CrosshairWorldPosition;
        FVector End = Start + CrosshairWorldDirection * TRACE_LENGTH;
        if (Character)
        {
            float DistanceToCharacter = (Character->GetActorLocation() - Start).Size();
            Start += CrosshairWorldDirection * (DistanceToCharacter + 100.f);
        }

        GetWorld()->LineTraceSingleByChannel(
            TraceHitResult,
            Start,
            End,
            ECollisionChannel::ECC_Visibility
        );
        if (TraceHitResult.GetActor() && TraceHitResult.GetActor()->Implements<UInteractWithCrosshairInterface>())
        {
            Package.CrosshairColor = FLinearColor::Red;
        }
        else
        {
            Package.CrosshairColor = FLinearColor::White;
        }
    }
}

void UCombatComponent::SetAiming(bool bIsAiming)
{
    bAiming = bIsAiming;
    ServerSetAiming(bIsAiming);
    if (Character)
    {
        Character->GetCharacterMovement()->MaxWalkSpeed = bIsAiming ? AimWalkSpeed : BaseWalkSpeed;
    }
}

void UCombatComponent::ServerSetAiming_Implementation(bool bIsAiming)
{
    bAiming = bIsAiming;
    if (Character)
    {
        Character->GetCharacterMovement()->MaxWalkSpeed = bIsAiming ? AimWalkSpeed : BaseWalkSpeed;
    }
}


void UCombatComponent::Reload()
{
    if (CarriedAmmo > 0 && CombatState != ECombatState::ECS_Reloading)
    {
        // If has Ammo left, can reload
        ServerReload();
    }
}

void UCombatComponent::FinishReloading()
{
    if (Character == nullptr) return;
    if (Character->HasAuthority())
    {
        CombatState = ECombatState::ECS_Unoccupied;
        UpdateAmmoValue();
    }
    if (bFireButtonPressed)
    {
        Fire();
    }
}

void UCombatComponent::UpdateAmmoValue()
{
    if (Character == nullptr || EquippedWeapon == nullptr) return;
    int32 ReloadAmount = AmountToReload();
    if (CarriedAmmoMap.Contains(EquippedWeapon->GetWeaponType()))
    {
        CarriedAmmoMap[EquippedWeapon->GetWeaponType()] -= ReloadAmount;
        CarriedAmmo = CarriedAmmoMap[EquippedWeapon->GetWeaponType()];
    }
    Controller = Controller == nullptr ? Cast<ABlasterPlayerController>(Character->Controller) : Controller;
    if (Controller)
    {
        Controller->SetHUDCarriedAmmo(CarriedAmmo);
    }
    EquippedWeapon->ReloadAmmo(-ReloadAmount);
}

void UCombatComponent::ServerReload_Implementation()
{
    if (Character == nullptr || EquippedWeapon == nullptr) return;

    CombatState = ECombatState::ECS_Reloading;
    HandleReload();
}


void UCombatComponent::OnRep_CombatState()
{
    switch (CombatState)
    {
    case ECombatState::ECS_Unoccupied:
        if (bFireButtonPressed)
        {
            Fire();
        }
        break;
    case ECombatState::ECS_Reloading:
        HandleReload();
        break;
    }
}

void UCombatComponent::HandleReload()
{
    Character->PlayReloadMontage();
}

int32 UCombatComponent::AmountToReload()
{
    if (EquippedWeapon == nullptr) return 0;
    int32 RoomInMag = EquippedWeapon->GetMagCapcity() - EquippedWeapon->GetAmmo();
    if (CarriedAmmoMap.Contains(EquippedWeapon->GetWeaponType()))
    {
        int32 AmountCarried = CarriedAmmoMap[EquippedWeapon->GetWeaponType()];
        int32 LeastReload = FMath::Min(AmountCarried, RoomInMag);
        // If RoomInMag < AmountCarried, the number of reloading is equal to RoomInMag, so don't need to change it,
        // else if all your CarriedAmmo <= RoomInMag, change it to AmountCarried also the left Carried. 
        return FMath::Clamp(RoomInMag, 0, LeastReload);
    }
    return 0;
}
