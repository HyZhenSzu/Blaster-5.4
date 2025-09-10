// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "Blaster/BlasterTypes/TurningInPlace.h"
#include "BlasterAnimInstance.generated.h"

/**
 * 
 */
UCLASS()
class BLASTER_API UBlasterAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
    virtual void NativeInitializeAnimation() override;
    virtual void NativeUpdateAnimation(float DeltaTime) override;

private:
    UPROPERTY(BlueprintReadOnly, Category = Character, meta = (AllowPrivateAccess = "true"))
    class ABlasterCharacter* BlasterCharacter;

protected:
    UPROPERTY(BlueprintReadOnly, Category = Movement, meta = (AllowPrivateAcess = "true"))
    float Speed;

    UPROPERTY(BlueprintReadOnly, Category = Movement, meta = (AllowPrivateAcess = "true"))
    bool bIsInAir;

    UPROPERTY(BlueprintReadOnly, Category = Movement, meta = (AllowPrivateAcess = "true"))
    bool bIsAccelerating;

    UPROPERTY(BlueprintReadOnly, Category = Movement, meta = (AllowPrivateAcess = "true"))
    bool bWeaponEquipped;

    class AWeapon* EquippedWeapon;

    UPROPERTY(BlueprintReadOnly, Category = Movement, meta = (AllowPrivateAcess = "true"))
    bool bIsCrouched;

    UPROPERTY(BlueprintReadOnly, Category = Movement, meta = (AllowPrivateAcess = "true"))
    bool bIsAiming;

    UPROPERTY(BlueprintReadOnly, Category = Movement, meta = (AllowPrivateAcess = "true"))
    float YawOffSet;

    UPROPERTY(BlueprintReadOnly, Category = Movement, meta = (AllowPrivateAcess = "true"))
    float Lean;

    UPROPERTY(BlueprintReadOnly, Category = Movement, meta = (AllowPrivateAcess = "true"))
    float AO_Yaw;

    UPROPERTY(BlueprintReadOnly, Category = Movement, meta = (AllowPrivateAcess = "true"))
    float AO_Pitch;

    UPROPERTY(BlueprintReadOnly, Category = Movement, meta = (AllowPrivateAcess = "true"))
    ETurningInPlace TurningInPlace;

    FRotator CharacterRotationLastFrame;
    FRotator CharacterRotation;
    FRotator DeltaRotation;

    UPROPERTY(BlueprintReadOnly, Category = Movement, meta = (AllowPrivateAcess = "true"))
    FTransform LeftHandTransform;

    UPROPERTY(BlueprintReadOnly, Category = Movement, meta = (AllowPrivateAcess = "true"))
    FRotator RightHandRotation;

    UPROPERTY(BlueprintReadOnly, Category = Movement, meta = (AllowPrivateAcess = "true"))
    bool bLocallyControlled;

    UPROPERTY(BlueprintReadOnly, Category = Movement, meta = (AllowPrivateAcess = "true"))
    bool bRotateRootBone;

    UPROPERTY(BlueprintReadOnly, Category = Movement, meta = (AllowPrivateAcess = "true"))
    bool bElimmed;

    UPROPERTY(BlueprintReadOnly, Category = Movement, meta = (AllowPrivateAcess = "true"))
    bool bUseFABRIK;

    UPROPERTY(BlueprintReadOnly, Category = Movement, meta = (AllowPrivateAcess = "true"))
    bool bUseAimOffSet;

    UPROPERTY(BlueprintReadOnly, Category = Movement, meta = (AllowPrivateAcess = "true"))
    bool bUseTransfromRightHand;
};
