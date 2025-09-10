// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Blaster/HUD/BlasterHUD.h"
#include "Blaster/Weapon/WeaponType.h"
#include "Blaster/BlasterTypes/CombatState.h"
#include "CombatComponent.generated.h"

#define TRACE_LENGTH 80000.f;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class BLASTER_API UCombatComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UCombatComponent();
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    friend class ABlasterCharacter;

    void EquipWeapon(class AWeapon* WeaponEquipped);
    void FireButtonPressed(bool bPressed);

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    void Reload();
    UFUNCTION(BlueprintCallable)
    void FinishReloading();


protected:
	virtual void BeginPlay() override;

    void SetAiming(bool bIsAiming);

    UFUNCTION(Server, Reliable)
    void ServerSetAiming(bool bIsAiming);

    UFUNCTION()
    void OnRep_EquippedWeapon();

    

    void Fire();

    UFUNCTION(Server, Reliable)
    void ServerFire(const FVector_NetQuantize& TargetHitTarget);

    UFUNCTION(NetMulticast, Reliable)
    void MulticastFire(const FVector_NetQuantize& TargetHitTarget);

    void TraceUnderCrosshairs(FHitResult& TraceHitResult);

    void SetHUDCrosshairs(float DeltaTime);

    UFUNCTION(Server, Reliable)
    void ServerReload();

    void HandleReload();

    /*
    *  Calculate the Reload Capacity
    */
    int32 AmountToReload();

private:
    class ABlasterCharacter* Character;
    class ABlasterPlayerController* Controller;
    class ABlasterHUD* HUD;

    UPROPERTY(ReplicatedUsing = OnRep_EquippedWeapon)
    AWeapon* EquippedWeapon;
	
    UPROPERTY(Replicated)
    bool bAiming;

    UPROPERTY(EditAnywhere)
    float BaseWalkSpeed;

    UPROPERTY(EditAnywhere)
    float AimWalkSpeed;

    bool bFireButtonPressed;

    /**
    *  HUD and Crosshairs
    */

    FHUDPackage Package;

    FVector HitTarget;

    float CrosshairVelocityFactor;
    float CrosshairInAirFactor;
    float CrosshairAimingFactor;
    float CrosshairShootingFactor;


    /**
    *  Aiming and FOV
    */

    // the base field of view when not aiming
    float DefaultFOV;
    float CurrentFOV;

    UPROPERTY(EditAnywhere, Category = Combat)
    float ZoomedFOV = 30.f;

    UPROPERTY(EditAnywhere, Category = Combat)
    float ZoomInterpSpeed = 20.f;

    void InterpFOV(float DeltaTime);

    /*
    * Automatic Fire
    */
    FTimerHandle FireTimer;
    bool bCanFire = true;

    void StartFireTimer();
    void FireTimerFinished();

    bool CanFire();

    UPROPERTY(ReplicatedUsing = OnRep_CarriedAmmo)
    int32 CarriedAmmo;

    UFUNCTION()
    void OnRep_CarriedAmmo();

    TMap<EWeaponType, int32> CarriedAmmoMap;

    // Initalize Carried Ammos of all kind of Projectile Weapons.
    UPROPERTY(EditAnywhere, Category = Projectiles)
    int32 StartingARAmmo = 45;

    UPROPERTY(EditAnywhere, Category = Projectiles)
    int32 StartingRocketAmmo = 8;

    void InitializeCarriedAmmo();

    /*
    *  Combat State
    */
    UPROPERTY(ReplicatedUsing = OnRep_CombatState)
    ECombatState CombatState = ECombatState::ECS_Unoccupied;

    UFUNCTION()
    void OnRep_CombatState();

    void UpdateAmmoValue();
};
