// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "BlasterHUD.generated.h"

USTRUCT(BlueprintType)
struct FHUDPackage
{
    GENERATED_BODY()

public:
    class UTexture2D* CrosshairsCenter;
    UTexture2D* CrosshairsLeft;
    UTexture2D* CrosshairsRight;
    UTexture2D* CrosshairsTop;
    UTexture2D* CrosshairsBottom;
    float CrosshairsSpread;
    FLinearColor CrosshairColor;
};


/**
 * BlasterHUD shows all informations on players' screen, including
 * the crosshair in the middle,
 * the health bar and value
 */
UCLASS()
class BLASTER_API ABlasterHUD : public AHUD
{
	GENERATED_BODY()

public:
    virtual void DrawHUD() override;

    UPROPERTY(EditAnywhere, Category = "Player State")
    TSubclassOf<class UUserWidget> CharacterOverlayClass;

    UPROPERTY()
    class UCharacterOverlay* CharacterOverlay;

    void AddCharacterOverlay();

    /*
    *  The Announcement HUD when game begin and end.
    */
    UPROPERTY(EditAnywhere, Category = "Announcements")
    TSubclassOf<class UUserWidget> AnnouncementClass;

    UPROPERTY()
    class UAnnouncement* Announcement;

    void AddAnnouncement();

protected:
    virtual void BeginPlay() override;

private:
    FHUDPackage HUDPackage;

    UPROPERTY(EditAnywhere)
    float CrosshairSpreadMax = 16.f;

    void DrawCrosshair(UTexture2D* Texture, FVector2D ViewportCenter, FVector2D Spread, FLinearColor CrosshairColor);

public:
    FORCEINLINE void SetHUDPackage(const FHUDPackage& Package) { HUDPackage = Package; }
};
