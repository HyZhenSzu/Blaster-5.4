// Fill out your copyright notice in the Description page of Project Settings.


#include "BlasterPlayerController.h"
#include "Blaster/HUD/BlasterHUD.h"
#include "Blaster/HUD/CharacterOverlay.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Blaster/Character/BlasterCharacter.h"
#include "GameFramework/GameMode.h"
#include "Net/UnrealNetwork.h"
#include "Blaster/HUD/Announcement.h"
#include "Kismet/GameplayStatics.h"
#include "Blaster/GameMode/BlasterGameMode.h"
#include "Blaster/BlasterComponents/CombatComponent.h"
#include "Blaster/GameState/BlasterGameState.h"
#include "Blaster/PlayerStates/BlasterPlayerState.h"

void ABlasterPlayerController::BeginPlay()
{
    Super::BeginPlay();

    BlasterHUD = Cast<ABlasterHUD>(GetHUD());
    ServerCheckMatchState();
}

void ABlasterPlayerController::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    SetHUDTime();
    CheckTimeSync(DeltaTime);
    PollInit();
}

void ABlasterPlayerController::PollInit()
{
    if (CharacterOverlay == nullptr)
    {
        // if CharcterOverlay is not exist, get it and set the values of it.
        if (BlasterHUD && BlasterHUD->CharacterOverlay)
        {
            CharacterOverlay = BlasterHUD->CharacterOverlay;
            if (CharacterOverlay)
            {
                SetHUDHealthValue(HUDHealth, HUDMaxHealth);
                SetHUDScoreValue(HUDScore);
                SetHUDDefeatsValue(HUDDefeats);
            }
        }
    }
}

void ABlasterPlayerController::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(ABlasterPlayerController, MatchState);
}

void ABlasterPlayerController::OnPossess(APawn* InPawn)
{
    Super::OnPossess(InPawn);

    ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(InPawn);
    if (BlasterCharacter)
    {
        // Sometimes the character 's BeginPlay function is excuted eariler than OnPossess,
        // in this condition character will failed to get Controller and load mapping context.
        BlasterCharacter->SetMappingContextInput();
        SetHUDHealthValue(BlasterCharacter->GetHealth(), BlasterCharacter->GetMaxHealth());
    }
}


void ABlasterPlayerController::CheckTimeSync(float DeltaTime)
{
    TimeSyncRunningTime += DeltaTime;
    if (IsLocalController() && TimeSyncRunningTime > SyncTimeFrequency)
    {
        ServerRequestServerTime(GetWorld()->GetTimeSeconds());
        TimeSyncRunningTime = 0.f;
    }
}


void ABlasterPlayerController::ServerCheckMatchState_Implementation()
{
    ABlasterGameMode* GameMode = Cast<ABlasterGameMode>(UGameplayStatics::GetGameMode(this));
    if (GameMode)
    {
        WarmUpTime = GameMode->WarmUpTime;
        MatchTime = GameMode->MatchTime;
        CoolDownTime = GameMode->CoolDownTime;
        StartingLevelTime = GameMode->StartingLevelTime;
        MatchState = GameMode->GetMatchState();
        ClientJoinMidGame(MatchState, WarmUpTime, MatchTime, CoolDownTime, StartingLevelTime);

        if (BlasterHUD && BlasterHUD->Announcement == nullptr && MatchState == MatchState::WaitingToStart)
        {
            BlasterHUD->AddAnnouncement();
        }
    }
}

void ABlasterPlayerController::ClientJoinMidGame_Implementation(FName StateOfMatch, float TimeOfWarmUp, float TimeOfMatch, float TimeOfCoolDown, float TimeOfStartingLevel)
{
    MatchState = StateOfMatch;
    WarmUpTime = TimeOfWarmUp;
    MatchTime = TimeOfMatch;
    CoolDownTime = TimeOfCoolDown;
    StartingLevelTime = TimeOfStartingLevel;
    OnMatchStateSet(MatchState);
    if (BlasterHUD && MatchState == MatchState::WaitingToStart)
    {
        BlasterHUD->AddAnnouncement();
    }
}

float ABlasterPlayerController::GetServerTime()
{
    return GetWorld()->GetTimeSeconds() + ClientServerDelta;
}


void ABlasterPlayerController::SetHUDHealthValue(float Health, float MaxHealth)
{
    BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;
    bool bHUDVaild = BlasterHUD &&
        BlasterHUD->CharacterOverlay &&
        BlasterHUD->CharacterOverlay->HealthBar &&
        BlasterHUD->CharacterOverlay->HealthValue;
    if (bHUDVaild)
    {
        const float HealthPercent = Health / MaxHealth;
        BlasterHUD->CharacterOverlay->HealthBar->SetPercent(HealthPercent);
        FString HealthText = FString::Printf(TEXT("%d/%d"), FMath::CeilToInt(Health), FMath::CeilToInt(MaxHealth));
        BlasterHUD->CharacterOverlay->HealthValue->SetText(FText::FromString(HealthText));
    }
    else
    {
        bCharacterOverlayInitialized = true;
        HUDHealth = Health;
        HUDMaxHealth = MaxHealth;
    }
}

void ABlasterPlayerController::SetHUDScoreValue(float Score)
{
    BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;
    bool bHUDVaild = BlasterHUD &&
        BlasterHUD->CharacterOverlay &&
        BlasterHUD->CharacterOverlay->ScoreValue;
    if (bHUDVaild)
    {
        FString ScoreText = FString::Printf(TEXT("%d"), FMath::FloorToInt(Score));
        BlasterHUD->CharacterOverlay->ScoreValue->SetText(FText::FromString(ScoreText));
    }
    else
    {
        bCharacterOverlayInitialized = true;
        HUDScore = Score;
    }
}

void ABlasterPlayerController::SetHUDDefeatsValue(int32 Defeats)
{
    BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;
    bool bHUDVaild = BlasterHUD &&
        BlasterHUD->CharacterOverlay &&
        BlasterHUD->CharacterOverlay->DefeatsValue;
    if (bHUDVaild)
    {
        FString DefeatsText = FString::Printf(TEXT("%d"), Defeats);
        BlasterHUD->CharacterOverlay->DefeatsValue->SetText(FText::FromString(DefeatsText));
    }
    else
    {
        bCharacterOverlayInitialized = true;
        HUDDefeats = Defeats;
    }
}

void ABlasterPlayerController::SetHUDAmmoValue(int32 Ammo)
{
    BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;
    bool bHUDVaild = BlasterHUD &&
        BlasterHUD->CharacterOverlay &&
        BlasterHUD->CharacterOverlay->AmmoValue;
    if (bHUDVaild)
    {
        FString AmmoText = FString::Printf(TEXT("%d"), Ammo);
        BlasterHUD->CharacterOverlay->AmmoValue->SetText(FText::FromString(AmmoText));
    }
}

void ABlasterPlayerController::SetHUDCarriedAmmo(int32 CarriedAmmo)
{
    BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;
    bool bHUDVaild = BlasterHUD &&
        BlasterHUD->CharacterOverlay &&
        BlasterHUD->CharacterOverlay->CarriedAmmo;
    if (bHUDVaild)
    {
        FString CarriedText = FString::Printf(TEXT("%d"), CarriedAmmo);
        BlasterHUD->CharacterOverlay->CarriedAmmo->SetText(FText::FromString(CarriedText));
    }
}

void ABlasterPlayerController::SetHUDMatchCountDown(float CountDownTime)
{
    BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;
    bool bHUDVaild = BlasterHUD &&
        BlasterHUD->CharacterOverlay &&
        BlasterHUD->CharacterOverlay->CountingGameTime;
    if (bHUDVaild)
    {
        if (CountDownTime < 0.f)
        {
            BlasterHUD->CharacterOverlay->CountingGameTime->SetText(FText());
            return;
        }

        int32 Mintues = FMath::FloorToInt(CountDownTime / 60.f);
        int32 Seconds = CountDownTime - Mintues * 60;

        FString GameTimeText = FString::Printf(TEXT("%02d:%02d"), Mintues, Seconds);
        BlasterHUD->CharacterOverlay->CountingGameTime->SetText(FText::FromString(GameTimeText));
    }
}

void ABlasterPlayerController::SetHUDAnnouncementCountDowm(float CountDownTime)
{
    BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;
    bool bHUDVaild = BlasterHUD &&
        BlasterHUD->Announcement &&
        BlasterHUD->Announcement->WarmUpTime;
    if (bHUDVaild)
    {
        if (CountDownTime < 0.f)
        {
            BlasterHUD->Announcement->WarmUpTime->SetText(FText());
            return;
        }

        int32 Mintues = FMath::FloorToInt(CountDownTime / 60.f);
        int32 Seconds = CountDownTime - Mintues * 60;

        FString GameTimeText = FString::Printf(TEXT("%02d:%02d"), Mintues, Seconds);
        BlasterHUD->Announcement->WarmUpTime->SetText(FText::FromString(GameTimeText));
    }
}

void ABlasterPlayerController::SetHUDTime()
{
    float TimeLeft = 0.f;
    if (MatchState == MatchState::WaitingToStart)
        TimeLeft = WarmUpTime - GetServerTime() + StartingLevelTime;
    else if(MatchState == MatchState::InProgress)
        TimeLeft = WarmUpTime + MatchTime - GetServerTime() + StartingLevelTime;
    else if(MatchState == MatchState::CoolDown)
        TimeLeft = CoolDownTime + WarmUpTime + MatchTime - GetServerTime() + StartingLevelTime;

    uint32 SecondsLeft = FMath::CeilToInt(TimeLeft);

    if (HasAuthority())
    {
        BlasterGameMode = BlasterGameMode == nullptr ? Cast<ABlasterGameMode>(UGameplayStatics::GetGameMode(this)) : BlasterGameMode;
        if (BlasterGameMode)
        {
            SecondsLeft = FMath::CeilToInt(BlasterGameMode->GetCountDownTime() + StartingLevelTime);
        }
    }

    if (SecondsLeft != CountDownInt)
    {
        if (MatchState == MatchState::WaitingToStart || MatchState == MatchState::CoolDown)
        {
            SetHUDAnnouncementCountDowm(TimeLeft);
        }
        if (MatchState == MatchState::InProgress)
        {
            SetHUDMatchCountDown(TimeLeft);
        }
    }
    CountDownInt = SecondsLeft;
}

void ABlasterPlayerController::ServerRequestServerTime_Implementation(float TimeOfClientRequest)
{
    // Server time.
    float ServerTimeOfReceipt = GetWorld()->GetTimeSeconds();
    ClientReportServerTime(TimeOfClientRequest, ServerTimeOfReceipt);

}

void ABlasterPlayerController::ClientReportServerTime_Implementation(float TimeOfClientRequest, float TimeServerReceivedClientRequest)
{
    float RoundTripTime = GetWorld()->GetTimeSeconds() - TimeOfClientRequest;
    float CurrentServerTime = TimeServerReceivedClientRequest + (0.5f * RoundTripTime);
    ClientServerDelta = CurrentServerTime - GetWorld()->GetTimeSeconds();
}

void ABlasterPlayerController::ReceivedPlayer()
{
    Super::ReceivedPlayer();

    if (IsLocalController())
    {
        ServerRequestServerTime(GetWorld()->GetTimeSeconds());
    }
}

void ABlasterPlayerController::OnMatchStateSet(FName State)
{
    MatchState = State;

    if (MatchState == MatchState::InProgress)
    {
        // if the game has already start
        HandleMatchHasStarted();
    }
    else if (MatchState == MatchState::CoolDown)
    {
        HandleCoolDown();
    }
}

void ABlasterPlayerController::OnRep_MatchState()
{
    if (MatchState == MatchState::InProgress)
    {
        HandleMatchHasStarted();
    }
    else if (MatchState == MatchState::CoolDown)
    {
        HandleCoolDown();
    }
}

void ABlasterPlayerController::HandleMatchHasStarted()
{
    // Set the HUD of game, and hide announcement.
    BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;
    if (BlasterHUD)
    {
        BlasterHUD->AddCharacterOverlay();
        if (BlasterHUD->Announcement)
        {
            BlasterHUD->Announcement->SetVisibility(ESlateVisibility::Hidden);
        }
    }
}

void ABlasterPlayerController::HandleCoolDown()
{
    BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;
    if (BlasterHUD)
    {
        if (BlasterHUD->CharacterOverlay)
        {
            BlasterHUD->CharacterOverlay->RemoveFromParent();
        }

        bool bHUDValid = BlasterHUD->Announcement &&
            BlasterHUD->Announcement->AnnouncementText &&
            BlasterHUD->Announcement->InfoText;

        if (bHUDValid)
        {
            BlasterHUD->Announcement->SetVisibility(ESlateVisibility::Visible);
            FString AnnouncementText("New Match Starts In:");
            BlasterHUD->Announcement->AnnouncementText->SetText(FText::FromString(AnnouncementText));

            ABlasterGameState* BlasterGameState = Cast<ABlasterGameState>(UGameplayStatics::GetGameState(this));
            ABlasterPlayerState* BlasterPlayerState = GetPlayerState<ABlasterPlayerState>();
            if (BlasterGameState && BlasterPlayerState)
            {
                TArray<ABlasterPlayerState*> TopPlayers = BlasterGameState->TopScoringPlayers;
                FString InfoTextString;
                if (TopPlayers.Num() == 0)
                {
                    InfoTextString = FString("There is no winner.");
                }
                else if (TopPlayers.Num() == 1 && TopPlayers[0] == BlasterPlayerState)
                {
                    InfoTextString = FString("You are the winner!");
                }
                else if (TopPlayers.Num() == 1)
                {
                    InfoTextString = FString::Printf(TEXT("Winner: \n%s"), *TopPlayers[0]->GetPlayerName());
                }
                else if (TopPlayers.Num() > 1)
                {
                    InfoTextString = FString("Players tied for the win:\n");
                    for (auto TiedPlayer : TopPlayers)
                    {
                        InfoTextString.Append(FString::Printf(TEXT("%s\n"), *TiedPlayer->GetPlayerName()));
                    }
                }
                BlasterHUD->Announcement->InfoText->SetText(FText::FromString(InfoTextString));
            }
        }
    }
    ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(GetPawn());
    if (BlasterCharacter)
    {
        BlasterCharacter->bDisableGameplay = true;
        BlasterCharacter->GetCombatComponent()->FireButtonPressed(false);
    }
}
