// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "BlasterPlayerController.generated.h"

/**
 * 
 */
UCLASS()
class BLASTER_API ABlasterPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
    void SetHUDHealthValue(float Health, float MaxHealth);
    void SetHUDScoreValue(float Score);
    void SetHUDDefeatsValue(int32 Defeats);
    void SetHUDAmmoValue(int32 Ammo);
    void SetHUDCarriedAmmo(int32 CarriedAmmo);
    void SetHUDMatchCountDown(float CountDownTime);
    void SetHUDAnnouncementCountDowm(float CountDownTime);
    virtual void OnPossess(APawn* InPawn) override;
    virtual void Tick(float DeltaTime) override;

    virtual float GetServerTime();
    virtual void ReceivedPlayer() override;

    virtual void GetLifetimeReplicatedProps(TArray< FLifetimeProperty >& OutLifetimeProps) const override;
    void OnMatchStateSet(FName State);
    void HandleMatchHasStarted();
    void HandleCoolDown();

protected:
    virtual void BeginPlay() override;
    void SetHUDTime();
    void PollInit();

    /*
    * Sync time between client and server
    */
    // Requests the current server time, passing in the client's time when the request was sent.
    UFUNCTION(Server, Reliable)
    void ServerRequestServerTime(float TimeOfClientRequest);

    // Reports the current server time to the client in response to ServerRequestServerTime
    UFUNCTION(Client, Reliable)
    void ClientReportServerTime(float TimeOfClientRequest, float TimeServerReceivedClientRequest);

    float ClientServerDelta = 0.f; // different between client and server time

    UPROPERTY(EditAnywhere, Category = Time)
    float SyncTimeFrequency = 5.f;

    float TimeSyncRunningTime = 0.f;
    void CheckTimeSync(float DeltaTime);


    /*
    * 
    */
    UFUNCTION(Server, Reliable)
    void ServerCheckMatchState();

    UFUNCTION(Client, Reliable)
    void ClientJoinMidGame(FName StateOfMatch, float TimeOfWarmUp, float TimeOfMatch, float TimeOfCoolDown, float TimeOfStartingLevel);

private:
    UPROPERTY()
    class ABlasterHUD* BlasterHUD;

    UPROPERTY()
    class ABlasterGameMode* BlasterGameMode;

    /*
    *  The time of a round when the game over and checking the score.
    */
    float StartingLevelTime = 0.f;
    float MatchTime = 0.f;
    float WarmUpTime = 0.f;
    float CoolDownTime = 0.f;
    uint32 CountDownInt;

    UPROPERTY(ReplicatedUsing = OnRep_MatchState)
    FName MatchState;

    UFUNCTION()
    void OnRep_MatchState();

    UPROPERTY()
    class UCharacterOverlay* CharacterOverlay;
    bool bCharacterOverlayInitialized = false;

    float HUDHealth;
    float HUDMaxHealth;
    float HUDScore;
    int32 HUDDefeats;
};
