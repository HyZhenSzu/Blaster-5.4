#pragma once

UENUM(BlueprintType)
enum class EWeaponType : uint8
{
    EWT_AssaultRifle UMETA(DisplayName = "Assault Rifle"),
    EWT_RocketLancher UMETA(DisplayName = "Rocket Lancher"),

    EWT_MAX UMETA(DisplayName = "DefaultMax")
};
