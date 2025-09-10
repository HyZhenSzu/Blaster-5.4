// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Blaster/Interfaces/InteractWithCrosshairInterface.h"
#include "Blaster/BlasterTypes/TurningInPlace.h"
#include "Components/TimelineComponent.h"
#include "Blaster/BlasterTypes/CombatState.h"
#include "BlasterCharacter.generated.h"

struct FInputActionValue;


UCLASS()
class BLASTER_API ABlasterCharacter : public ACharacter, public IInteractWithCrosshairInterface
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	ABlasterCharacter();
    // Called every frame
    virtual void Tick(float DeltaTime) override;
    void RotateInPlace(float DeltaTime);
    // Called to bind functionality to input
    virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

    void SetMappingContextInput();

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    virtual void PostInitializeComponents() override;

    void PlayFireMontage(bool bAiming);
    void PlayElimMontage();

    void Elim();
    UFUNCTION(NetMulticast, Reliable)
    void MulticastElim();

    virtual void OnRep_ReplicatedMovement() override;

    void PlayReloadMontage();

    /*
    *  When it comes to WaitingToStart or CoolDown, the Character need to look around but not move.
    */
    UPROPERTY(Replicated)
    bool bDisableGameplay = false;


protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

    void Move(const FInputActionValue& Value);
    void Look(const FInputActionValue& Value);
    void Jump() override;
    void EquipButtonPressed(const FInputActionValue& Value);
    void CrouchButtonPressed(const FInputActionValue& Value);
    void AimingButtonPressed(const FInputActionValue& Value);
    void AimingButtonReleased(const FInputActionValue& Value);
    void AimOffSet(float DeltaTime);
    void CalculateAO_Pitch();
    void SimProxiesTurn();
    void FireButtonPressed(const FInputActionValue& Value);
    void FireButtonReleased(const FInputActionValue& Value);
    void PlayHitMontage();
    void ReloadButtonPressed(const FInputActionValue& Value);

    virtual void Destroyed() override;


    UFUNCTION()
    void ReceiveDamage(AActor* DamageActor, float Damage, const UDamageType* DamageType, class AController* InstigatorController, AActor* DamageCauser);
    void UpdateHealthHUD();
    // Poll for  relevant classes and initalize HUD.
    void PollInit();
private:
    UPROPERTY(VisibleAnywhere, Category = Camera)
    class USpringArmComponent* CameraBoom;

    UPROPERTY(VisibleAnywhere, Category = Camera)
    class UCameraComponent* FollowCamera;

    UPROPERTY(EditDefaultsOnly, Category = Input)
    class UInputMappingContext* ImcBlasterMappingContext;

    UPROPERTY(EditAnywhere, Category = Input)
    class UInputAction* MoveAction;

    UPROPERTY(EditAnywhere, Category = Input)
    UInputAction* LookAction;

    UPROPERTY(EditAnywhere, Category = Input)
    UInputAction* JumpAction;

    UPROPERTY(EditAnywhere, Category = Input)
    UInputAction* EquipAction;

    UPROPERTY(EditAnywhere, Category = Input)
    UInputAction* CrouchAction;

    UPROPERTY(EditAnywhere, Category = Input)
    UInputAction* AimingAction;

    UPROPERTY(EditAnywhere, Category = Input)
    UInputAction* FireAction;

    UPROPERTY(EditAnywhere, Category = Input)
    UInputAction* ReloadAction;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
    class UWidgetComponent* OverheadWidget;

    UPROPERTY(ReplicatedUsing = OnRep_OverlappingWeapon)
    class AWeapon* OverlappingWeapon;

    UFUNCTION()
    void OnRep_OverlappingWeapon(AWeapon* LastWeapon);

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
    class UCombatComponent* Combat;

    //
    // This function is a RPC(Remote Procedure Call) function which clients could get information from.
    // An unreliable package is could be dropped.
    //
    UFUNCTION(Server, Reliable)
    void ServerEquipButtonPressed();

    float AO_Yaw;
    float InterpAO_Yaw;
    float AO_Pitch;
    FRotator StartingAimRotation;

    // The enum about turning of Character when holding a weapon
    ETurningInPlace TurningInPlace;

    void TurnInPlace(float DeltaTime);


    void HideCharacterIfClose();

    UPROPERTY(EditAnywhere)
    float CameraThresHold = 200.f;

    /*
    *  Montage Properties
    */

    UPROPERTY(EditAnywhere, Category = Combat)
    class UAnimMontage* FireWeaponMontage;

    UPROPERTY(EditAnywhere, Category = Combat)
    UAnimMontage* OnHitMontage;

    UPROPERTY(EditAnywhere, Category = Combat)
    UAnimMontage* ElimMontage;

    UPROPERTY(EditAnywhere, Category = Combat)
    UAnimMontage* ReloadMontage;



    bool bRotateRootBone;
    float TurnThresold = 0.5f;
    FRotator ProxyRotationLastFrame;
    FRotator ProxyRotation;
    float ProxyYaw;
    float TimeSinceLastMovementReplication;
    float CalculateSpeed();


    /**
    * Player Health
    */

    UPROPERTY(EditAnywhere, Category = "Player State")
    float MaxHealth = 100.f;

    UPROPERTY(ReplicatedUsing = OnRep_Health, VisibleAnywhere, Category = "Player State")
    float Health = 100.f;

    UFUNCTION()
    void OnRep_Health();

    class ABlasterPlayerController* BlasterPlayerController;

    /*
    *  Elim Player and Respwan when time out.
    */
    bool bElimmed = false;

    FTimerHandle ElimTimer;
    UPROPERTY(EditDefaultsOnly)
    float ElimDelay = 3.f;

    void ElimTimerFinish();

    /*
    *  Dissolve Effect
    */
    UPROPERTY(VisibleAnywhere)
    UTimelineComponent* DissolveTimeline;
    FOnTimelineFloat DissolveTrack;

    UPROPERTY(EditAnywhere)
    UCurveFloat* DissolveCurve;

    UFUNCTION()
    void UpdateDissolveMaterial(float DissolveValue);
    void StartDissolve();

    UPROPERTY(VisibleAnywhere, Category = Elim)
    UMaterialInstanceDynamic* DynamicDissolveMaterialInstance;

    UPROPERTY(EditAnywhere, Category = Elim)
    UMaterialInstance* DissolveMaterialInstance;

    class ABlasterPlayerState* BlasterPlayerState;


public:	
    void SetOverlappingWeapon(AWeapon* Weapon);
    bool IsWeaponEquipped();
    bool IsAiming();
    FORCEINLINE float GetAO_Yaw() const { return AO_Yaw; }
    FORCEINLINE float GetAO_Pitch() const { return AO_Pitch; }
    FORCEINLINE ETurningInPlace GetTurningInPlace() const { return TurningInPlace; }
    FORCEINLINE UCameraComponent* GetFollowCamera() const { return FollowCamera; }
    FORCEINLINE bool ShouldRotateRootBone() const { return bRotateRootBone; }
    FORCEINLINE bool IsElimmed() const { return bElimmed; }
    FORCEINLINE float GetHealth() const { return Health; }
    FORCEINLINE float GetMaxHealth() const { return MaxHealth; }
    AWeapon* GetEquippedWeapon();
    FVector GetHitTarget() const;
    ECombatState GetCombatState() const;
    FORCEINLINE UCombatComponent* GetCombatComponent() const { return Combat; }
    FORCEINLINE bool IsDisableGamplay() const { return bDisableGameplay; }
};
