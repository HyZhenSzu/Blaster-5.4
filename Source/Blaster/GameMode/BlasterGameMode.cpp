// Fill out your copyright notice in the Description page of Project Settings.


#include "BlasterGameMode.h"
#include "Blaster/Character/BlasterCharacter.h"
#include "Blaster/PlayerController/BlasterPlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerStart.h"
#include "Blaster/PlayerStates/BlasterPlayerState.h"
#include "Blaster/GameState/BlasterGameState.h"

namespace MatchState
{
    const FName CoolDown = FName("CoolDown");
}

ABlasterGameMode::ABlasterGameMode()
{
    bDelayedStart = true;
}

void ABlasterGameMode::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (MatchState == MatchState::WaitingToStart)
    {
        CountDownTime = WarmUpTime - GetWorld()->GetTimeSeconds() + StartingLevelTime;
        if (CountDownTime <= 0.f)
        {
            StartMatch();
        }
    }
    else if (MatchState == MatchState::InProgress)
    {
        CountDownTime = WarmUpTime + MatchTime - GetWorld()->GetTimeSeconds() + StartingLevelTime;
        if (CountDownTime <= 0.f)
        {
            SetMatchState(MatchState::CoolDown);
        }
    }
    else if (MatchState == MatchState::CoolDown)
    {
        CountDownTime = CoolDownTime + WarmUpTime + MatchTime - GetWorld()->GetTimeSeconds() + StartingLevelTime;
        if (CountDownTime <= 0.f)
        {
            RestartGame();
        }
    }
}

void ABlasterGameMode::BeginPlay()
{
    Super::BeginPlay();

    StartingLevelTime = GetWorld()->GetTimeSeconds();
}

void ABlasterGameMode::PlayerEliminated(ABlasterCharacter* ElimmedCharacter, ABlasterPlayerController* VictimPlayerController, ABlasterPlayerController* AttackerPlayerController)
{
    if (AttackerPlayerController == nullptr || AttackerPlayerController->PlayerState == nullptr) return;
    if (VictimPlayerController == nullptr || VictimPlayerController->PlayerState == nullptr) return;
	ABlasterPlayerState* AttackerPlayerState = AttackerPlayerController ? Cast<ABlasterPlayerState>(AttackerPlayerController->PlayerState) : nullptr;
	ABlasterPlayerState* VictimPlayerState = VictimPlayerController ? Cast<ABlasterPlayerState>(VictimPlayerController->PlayerState) : nullptr;

    ABlasterGameState* BlasterGameState = GetGameState<ABlasterGameState>();

	if (AttackerPlayerState && AttackerPlayerState != VictimPlayerState && BlasterGameState)
	{
		AttackerPlayerState->AddToScore(1.f);
        BlasterGameState->UpdateTopScore(AttackerPlayerState);
	}

    if (VictimPlayerState)
    {
        VictimPlayerState->AddToDefeats(1);
    }

	if (ElimmedCharacter)
	{
		ElimmedCharacter->Elim();
	}
}

void ABlasterGameMode::RequestRespawn(ABlasterCharacter* ElimmedCharacter, AController* ElimmedController)
{
	if (ElimmedCharacter)
	{
		ElimmedCharacter->Reset();
		ElimmedCharacter->Destroy();
	}
	if (ElimmedController)
	{
		TArray<AActor*> PlayerStarts;
		UGameplayStatics::GetAllActorsOfClass(this, APlayerStart::StaticClass(), PlayerStarts);
		int32 Selection = FMath::RandRange(0, PlayerStarts.Num() - 1);
		RestartPlayerAtPlayerStart(ElimmedController, PlayerStarts[Selection]);
	}
}

void ABlasterGameMode::OnMatchStateSet()
{
    Super::OnMatchStateSet();

    for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
    {
        ABlasterPlayerController* BlasterController = Cast<ABlasterPlayerController>(*It);
        if (BlasterController)
        {
            BlasterController->OnMatchStateSet(MatchState);
        }
    }
}
