// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "BlasterGameMode.generated.h"

namespace MatchState
{
    extern BLASTER_API const FName CoolDown; // Match duration has ended. Display viewport and show the winner. 
}

/**
 * 
 */
UCLASS()
class BLASTER_API ABlasterGameMode : public AGameMode
{
	GENERATED_BODY()
	
public:
    ABlasterGameMode();
    virtual void Tick(float DeltaTime) override;
	virtual void PlayerEliminated(class ABlasterCharacter* ElimmedCharacter, class ABlasterPlayerController* VictimPlayerController, class ABlasterPlayerController* AttackerPlayerController);
	virtual void RequestRespawn(class ABlasterCharacter* ElimmedCharacter, class AController* ElimmedController);

    UPROPERTY(EditDefaultsOnly)
    float MatchTime = 120.f;

    UPROPERTY(EditDefaultsOnly)
    float WarmUpTime = 10.f;

    float StartingLevelTime = 0.f;

    UPROPERTY(EditDefaultsOnly)
    float CoolDownTime = 10.f;

protected:
    virtual void BeginPlay() override;
    virtual void OnMatchStateSet() override;
    
private:

    float CountDownTime = 0.f;

public:
    FORCEINLINE float GetCountDownTime() { return CountDownTime; }
};
