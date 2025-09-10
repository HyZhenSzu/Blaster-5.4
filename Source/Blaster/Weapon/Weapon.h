// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WeaponType.h"
#include "Weapon.generated.h"

UENUM(BlueprintType)
enum class EWeaponState : uint8
{
    EWS_Initial UMETA(DisplayName = "Initial State"),
    EWS_Equipped UMETA(DisplayName = "Equipped"),
    EWS_Dropped UMETA(DisplayName = "Dropped"),

    EWS_MAX UMETA(DisplayName = "DefaultMAX")
};


UCLASS()
class BLASTER_API AWeapon : public AActor
{
	GENERATED_BODY()
	
public:	
	AWeapon();
    virtual void Tick(float DeltaTime) override;
    void ShowPickupWidget(bool bShowPickupWidget);

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    virtual void OnRep_Owner() override;

    virtual void Fire(const FVector& HitTarget);
    void SetHUDAmmo();
    void Dropped();
    void ReloadAmmo(int32 ReloadAmmo);

    //
    // Textures of the weapon crosshairs
    //
    UPROPERTY(EditAnywhere, Category = Crosshairs)
    class UTexture2D* CrosshairsCenter;

    UPROPERTY(EditAnywhere, Category = Crosshairs)
    UTexture2D* CrosshairsLeft;

    UPROPERTY(EditAnywhere, Category = Crosshairs)
    UTexture2D* CrosshairsRight;

    UPROPERTY(EditAnywhere, Category = Crosshairs)
    UTexture2D* CrosshairsTop;

    UPROPERTY(EditAnywhere, Category = Crosshairs)
    UTexture2D* CrosshairsBottom;


    /*
    * Automatic Fire
    */

    UPROPERTY(EditAnywhere, Category = Combat)
    float FireDelay = .15f;

    UPROPERTY(EditAnywhere, Category = Combat)
    bool bAutomatic = true;

    /*
    *  Sounds of Weapon
    */
    UPROPERTY(EditAnywhere)
    class USoundCue* EquipSound;
    

protected:
	virtual void BeginPlay() override;

    UFUNCTION()
    virtual void OnSphereOverlop(
        UPrimitiveComponent* OverlappedComponent,
        AActor* OtherActor,
        UPrimitiveComponent* OtherComp,
        int32 OtherBodyIndex,
        bool bFromSweep,
        const FHitResult& SweepResult
    );

    UFUNCTION()
    virtual void EndSphereOverlap(
        UPrimitiveComponent* OverlappedComponent,
        AActor* OtherActor,
        UPrimitiveComponent* OtherComp,
        int32 OtherBodyIndex
    );

private:
    UPROPERTY(VisibleAnywhere, Category = "Weapon Properties")
    USkeletalMeshComponent* WeaponMesh;

    UPROPERTY(VisibleAnywhere, Category = "Weapon Properties")
    class USphereComponent* AreaSphere;

    // Network replicated.
    UPROPERTY(ReplicatedUsing = OnRep_WeaponState, VisibleAnywhere, Category = "Weapon Properties")
    EWeaponState WeaponState;

    UFUNCTION()
    void OnRep_WeaponState();

    UPROPERTY(VisibleAnywhere, Category = "Weapon Properties")
    class UWidgetComponent* PickupWidget;

    UPROPERTY(EditAnywhere, Category = "Weapon Properties")
    class UAnimationAsset* FireAnimation;

    UPROPERTY(EditAnywhere)
    TSubclassOf<class ACasing> CasingClass;

    /*
    *  Ammo 
    */
    UPROPERTY(ReplicatedUsing = OnRep_Ammo, EditAnywhere, Category = "Weapon Properties")
    int32 Ammo;

    UFUNCTION()
    void OnRep_Ammo();

    void SpendRound();

    UPROPERTY(EditAnywhere, Category = "Weapon Properties")
    int32 MagCapacity;


    UPROPERTY(EditAnywhere)
    float ZoomedFOV = 30.f;

    UPROPERTY(EditAnywhere)
    float ZoomInterpSpeed = 20.f;

    UPROPERTY(EditAnywhere)
    EWeaponType WeaponType;

    /*
    *  Character and PlayerController
    */
    UPROPERTY()
    class ABlasterCharacter* OwnerCharacter;

    UPROPERTY()
    class ABlasterPlayerController* OwnerController;

public:
    void SetWeaponState(EWeaponState State);
    bool IsAmmoEmpty();
    FORCEINLINE USphereComponent* GetAreaSphere() const { return AreaSphere; }
    FORCEINLINE USkeletalMeshComponent* GetWeaponMesh() const { return WeaponMesh; }
    FORCEINLINE float GetZoomedFOV() const { return ZoomedFOV; }
    FORCEINLINE float GetZoomInterpSpeed() const { return ZoomInterpSpeed; }
    FORCEINLINE EWeaponType GetWeaponType() const { return WeaponType; }
    FORCEINLINE int32 GetAmmo() const { return Ammo; }
    FORCEINLINE int32 GetMagCapcity() const { return MagCapacity; }
};
