// Fill out your copyright notice in the Description page of Project Settings.


#include "Menu.h"
#include "Components/Button.h"
#include "MultiplayerSessionSubsystem.h"
#include "OnlineSessionSettings.h"
#include "Interfaces/OnlineSessionInterface.h"


void UMenu::MenuSetup(int32 NumberOfPublicConnnections, FString TypeOfMatch, FString LobbyPath)
{
    PathToLobby = FString::Printf(TEXT("%s?listen"), *LobbyPath);
    NumPublicConnections = NumberOfPublicConnnections;
    MatchType = TypeOfMatch;
    AddToViewport();
    SetVisibility(ESlateVisibility::Visible);
    bIsFocusable = true;

    UWorld* World = GetWorld();
    if (World)
    {
        APlayerController* PlayerController = World->GetFirstPlayerController();
        if (PlayerController)
        {
            FInputModeUIOnly InputModeData;
            InputModeData.SetWidgetToFocus(TakeWidget());
            InputModeData.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
            PlayerController->SetInputMode(InputModeData);
            PlayerController->SetShowMouseCursor(true);
        }
    }
    UGameInstance* GameInstance = GetGameInstance();
    if (GameInstance)
    {
        MultiplayerSessionSubsystem = GameInstance->GetSubsystem<UMultiplayerSessionSubsystem>();
    }

    if (MultiplayerSessionSubsystem)
    {
        MultiplayerSessionSubsystem->MultiplayerOnCreateSessionComplete.AddDynamic(this, &ThisClass::OnCreateSession);
        MultiplayerSessionSubsystem->MultiplayerOnFindSessionComplete.AddUObject(this, &ThisClass::OnFindSession);
        MultiplayerSessionSubsystem->MultiplayerOnJoinSessionComplete.AddUObject(this, &ThisClass::OnJoinSession);
        MultiplayerSessionSubsystem->MultiplayerOnDestorySessionComplete.AddDynamic(this, &ThisClass::OnDestorySession);
        MultiplayerSessionSubsystem->MultiplayerOnStartSessionComplete.AddDynamic(this, &ThisClass::OnStartSession);
    }
}

bool UMenu::Initialize()
{
    if (!Super::Initialize())
    {
        return false;
    }
    if (HostButton)
    {
        HostButton->OnClicked.AddDynamic(this, &ThisClass::HostButtonClicked);
    }
    if (JoinButton)
    {
        JoinButton->OnClicked.AddDynamic(this, &ThisClass::JoinButtonClicked);
    }
    return true;
}

void UMenu::NativeDestruct()
{
    MenuTearDown();
    Super::NativeDestruct();
}

void UMenu::OnCreateSession(bool bWasSuccessful)
{
    if (bWasSuccessful)
    {
        if (GEngine)
        {
            GEngine->AddOnScreenDebugMessage(
                -1,
                15.f,
                FColor::Yellow,
                FString(TEXT("Session created successfully!"))
            );
        }
        UWorld* World = GetWorld();
        if (World)
        {
            World->ServerTravel(PathToLobby);
        }

        IOnlineSubsystem* Subsystem = IOnlineSubsystem::Get();
        if (Subsystem)
        {
            IOnlineSessionPtr SessionInterface = Subsystem->GetSessionInterface();
            if (SessionInterface.IsValid())
            {
                FNamedOnlineSession* Session = SessionInterface->GetNamedSession(NAME_GameSession);
                if (Session)
                {
                    if (GEngine)
                    {
                        GEngine->AddOnScreenDebugMessage(
                            -1,
                            15.f,
                            FColor::Red,
                            FString::Printf(TEXT("Session state: %s"), EOnlineSessionState::ToString(Session->SessionState))
                        );
                    }
                }
            }
        }
    }
    else
    {
        if (GEngine)
        {
            GEngine->AddOnScreenDebugMessage(
                -1,
                15.f,
                FColor::Yellow,
                FString(TEXT("Session created falled!"))
            );
        }
        HostButton->SetIsEnabled(true);
    }
}

void UMenu::OnFindSession(const TArray<FOnlineSessionSearchResult>& SessionResults, bool bWasSuccessful)
{
    if (MultiplayerSessionSubsystem == nullptr)
    {
        if (GEngine)
        {
            GEngine->AddOnScreenDebugMessage(
                -1,
                15.f,
                FColor::Red,
                FString(TEXT("MultiplayerSessionSubsystem Do Not Exist!"))
            );
        }
        return;
    }

    if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage(
            -1,
            15.f,
            FColor::Blue,
            FString(TEXT("Searching for Host......"))
        );
    }
    if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage(
            -1,
            15.f,
            FColor::Yellow,
            FString::Printf(TEXT("Nums of SessionResults: %d"), SessionResults.Num())
        );
    }
    for (auto Result : SessionResults)
    {
        FString SettingsValue;
        Result.Session.SessionSettings.Get(FName("MatchType"), SettingsValue);

        FString ParamDump;
        for (auto& Setting : Result.Session.SessionSettings.Settings)
        {
            FString ValueStr;
            Setting.Value.Data.GetValue(ValueStr);
            ParamDump += FString::Printf(TEXT("\n  %s: %s"),
                *Setting.Key.ToString(), *ValueStr);
        }

        GEngine->AddOnScreenDebugMessage(
            -1,
            30.f,
            FColor::Cyan,
            FString::Printf(TEXT("Find Session Result Settings:%s"), *ParamDump)
        );

        GEngine->AddOnScreenDebugMessage(
            -1,
            30.f,
            FColor::Cyan,
            FString::Printf(TEXT("Session Owner: %s"), *Result.Session.OwningUserName)
        );

        if (SettingsValue == MatchType)
        {
            if (GEngine)
            {
                GEngine->AddOnScreenDebugMessage(
                    -1,
                    15.f,
                    FColor::Blue,
                    FString(TEXT("Try to Join a Session......"))
                );
            }
            MultiplayerSessionSubsystem->JoinSession(Result);
            return;
        }
    }
    if (!bWasSuccessful || SessionResults.Num() == 0)
    {
        if (GEngine)
        {
            GEngine->AddOnScreenDebugMessage(
                -1,
                15.f,
                FColor::Yellow,
                FString(TEXT("Find Host Failed!"))
            );
        }
        JoinButton->SetIsEnabled(true);
    }
}

void UMenu::OnJoinSession(EOnJoinSessionCompleteResult::Type Result)
{
    if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage(
            -1,
            15.f,
            FColor::Yellow,
            FString(TEXT("On Join Session!"))
        );
    }
    IOnlineSubsystem* Subsystem = IOnlineSubsystem::Get();
    if (Subsystem)
    {
        IOnlineSessionPtr SessionInterface = Subsystem->GetSessionInterface();
        if (SessionInterface.IsValid())
        {
            FString Address;
            SessionInterface->GetResolvedConnectString(NAME_GameSession, Address);
            if (GEngine)
            {
                GEngine->AddOnScreenDebugMessage(
                    -1,
                    15.f,
                    FColor::Yellow,
                    FString::Printf(TEXT("Get Travel Address %s"), *Address)
                );
            }

            APlayerController* PlayerController = GetGameInstance()->GetFirstLocalPlayerController();
            if (PlayerController)
            {
                PlayerController->ClientTravel(Address, ETravelType::TRAVEL_Absolute);
            }
        }
    }
    if (Result != EOnJoinSessionCompleteResult::Success)
    {
        JoinButton->SetIsEnabled(true);
    }
    if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage(
            -1,
            15.f,
            FColor::Red,
            FString::Printf(TEXT("Join Session Successful!"))
        );
    }
}

void UMenu::OnDestorySession(bool bWasSuccessful)
{
}

void UMenu::OnStartSession(bool bWasSuccessful)
{
}

void UMenu::HostButtonClicked()
{
    HostButton->SetIsEnabled(false);
    if (MultiplayerSessionSubsystem)
    {
        MultiplayerSessionSubsystem->CreateSession(NumPublicConnections, MatchType);
    }
}

void UMenu::JoinButtonClicked()
{
    JoinButton->SetIsEnabled(false);
    if (MultiplayerSessionSubsystem)
    {
        MultiplayerSessionSubsystem->FindSessions(10000);
    }
}

void UMenu::MenuTearDown()
{
    RemoveFromParent();
    UWorld* World = GetWorld();
    if (World)
    {
        APlayerController* PlayerController = World->GetFirstPlayerController();
        if (PlayerController)
        {
            FInputModeGameOnly InputModeData;
            PlayerController->SetInputMode(InputModeData);
            PlayerController->SetShowMouseCursor(false);
        }
    }
}
