// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CharacterOverlay.generated.h"

/**
 * 
 */
UCLASS()
class BLASTER_API UCharacterOverlay : public UUserWidget
{
	GENERATED_BODY()
public:
    /**
    * Show health bar and value of player.
    */
    UPROPERTY(meta = (BindWidget))
    class UProgressBar* HealthBar;

    UPROPERTY(meta = (BindWidget))
    class UTextBlock* HealthValue;

    UPROPERTY(meta = (BindWidget))
    UTextBlock* ScoreValue;

    UPROPERTY(meta = (BindWidget))
    UTextBlock* DefeatsValue;

    UPROPERTY(meta = (BindWidget))
    UTextBlock* AmmoValue;

    UPROPERTY(meta = (BindWidget))
    UTextBlock* CarriedAmmo;

    UPROPERTY(meta = (BindWidget))
    UTextBlock* CountingGameTime;
};
