// Fill out your copyright notice in the Description page of Project Settings.


#include "W_Menu.h"
#include "Components/Button.h"
#include "MultiplayerSessionsSubsystem.h"
#include "OnlineSessionSettings.h" 
#include "OnlineSubsystem.h"
#include "Interfaces/OnlineSessionInterface.h"


/**
 * @brief Constructor.
 * Bind buttons to functions.
 */
bool UW_Menu::Initialize()
{
    if(!Super::Initialize())
    {
        return false;
    }

    if(HostButton)
    {
        HostButton->OnClicked.AddDynamic(this, &ThisClass::HostButtonClicked);
    }

    if(JoinButton)
    {
        JoinButton->OnClicked.AddDynamic(this, &ThisClass::JoinButtonClicked);
    }

    return true;
}

/**
 * @brief Configure the session.
 * @param _NumPublicConnections Number of allowed connections
 * @param _MatchType Kind of game
 */
void UW_Menu::MenuSetup(int32 _NumPublicConnections, FString _MatchType, FString _LobbyPath)
{
    PathToLobby = FString::Printf(TEXT("%s?listen"), *_LobbyPath);
    NumPublicConnections = _NumPublicConnections;
    MatchType = _MatchType;

    AddToViewport();
    SetVisibility(ESlateVisibility::Visible);
    bIsFocusable = true;
    
    // Configure menu
    UWorld* World = GetWorld();
    if(World)
    {
        APlayerController* PlayerController = World->GetFirstPlayerController();
        if(PlayerController)
        {
            FInputModeUIOnly InputModeData;
            InputModeData.SetWidgetToFocus(TakeWidget());
            InputModeData.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock); // The player can get out the mouse from the screen
            PlayerController->SetInputMode(InputModeData);
            PlayerController->SetShowMouseCursor(true);
        }
    }

    UGameInstance* GameInstance = GetGameInstance();
    if(GameInstance)
    {
        MultiplayerSessionsSubsystem = GameInstance->GetSubsystem<UMultiplayerSessionsSubsystem>();
    }

    // Bind our custom delegates
    if(MultiplayerSessionsSubsystem)
    {
        MultiplayerSessionsSubsystem->CustomOnCreateSessionCompleteDelegate.AddDynamic(this, &ThisClass::OnCreateSession);
        MultiplayerSessionsSubsystem->CustomOnDestroySessionCompleteDelegate.AddDynamic(this, &ThisClass::OnDestroySession);
        MultiplayerSessionsSubsystem->CustomOnStartSessionCompleteDelegate.AddDynamic(this, &ThisClass::OnStartSession);
        
        // For t
        MultiplayerSessionsSubsystem->CustomOnFindSessionsCompleteDelegate.AddUObject(this, &ThisClass::OnFindSessions);
        MultiplayerSessionsSubsystem->CustomOnJoinSessionCompleteDelegate.AddUObject(this, &ThisClass::OnJoinSession);
    }
}

/**
 * @brief Host button.
 */
void UW_Menu::HostButtonClicked()
{
    HostButton->SetIsEnabled(false);
    if(!MultiplayerSessionsSubsystem)
    {
        HostButton->SetIsEnabled(true);
        if(GEngine){GEngine->AddOnScreenDebugMessage(-1, 10, FColor::Red, FString::Printf(TEXT("MultiplayerSessionsSubsystem plugin is Invalid.")));}
		return;    
    }
    MultiplayerSessionsSubsystem->CreateSession(NumPublicConnections, MatchType);
}

/**
 * @brief Join button.
 */
void UW_Menu::JoinButtonClicked()
{
    JoinButton->SetIsEnabled(false);
    if(!MultiplayerSessionsSubsystem)
    {
        JoinButton->SetIsEnabled(true);
        if(GEngine){GEngine->AddOnScreenDebugMessage(-1, 10, FColor::Red, FString::Printf(TEXT("MultiplayerSessionsSubsystem plugin is Invalid.")));}
		return;    
    }
    
    MultiplayerSessionsSubsystem->FindSessions(10000);
}


void UW_Menu::OnCreateSession(bool bWasSuccessful)
{
    if(bWasSuccessful)
    {
        UWorld* World = GetWorld();
        if(World)
        {   
            if(World->ServerTravel(PathToLobby))
            {
                if(GEngine){GEngine->AddOnScreenDebugMessage(-1, 10, FColor::Green, FString::Printf(TEXT("In the lobby")));}
            }
            else
            {
                JoinButton->SetIsEnabled(true);
                if(GEngine){GEngine->AddOnScreenDebugMessage(-1, 10, FColor::Red, FString::Printf(TEXT("Impossible to travel to the lobby")));}
                return;
            }
        }
        if(GEngine){GEngine->AddOnScreenDebugMessage(-1, 10, FColor::Red, FString::Printf(TEXT("Impossible to get the world")));}
        JoinButton->SetIsEnabled(true);
    }
    else
    {
        JoinButton->SetIsEnabled(true);
        if(GEngine){GEngine->AddOnScreenDebugMessage(-1, 10, FColor::Red, FString::Printf(TEXT("Failed to create the session")));}
        return;    
    }

}

void UW_Menu::OnFindSessions(const TArray<FOnlineSessionSearchResult>& SessionResult, bool bWasSuccessful)
{
    if(!MultiplayerSessionsSubsystem)
	{
        JoinButton->SetIsEnabled(true);
		if(GEngine){GEngine->AddOnScreenDebugMessage(-1, 10, FColor::Red, FString::Printf(TEXT("Custom MultiplayerSessionsSubsystem plugin is invalid.")));}
		return;
	}

	if(bWasSuccessful)
	{
		FString SessionFound_MatchType;
		for(auto Result : SessionResult)
		{
			Result.Session.SessionSettings.Get(FName("MatchType"), SessionFound_MatchType); // MatchType is an OutParameter
			if (SessionFound_MatchType == MatchType)
			{
				MultiplayerSessionsSubsystem->JoinSession(Result);
                return;
			}
		}
        JoinButton->SetIsEnabled(true);

	}
	else
	{
        JoinButton->SetIsEnabled(true);
		if(GEngine){GEngine->AddOnScreenDebugMessage(-1, 10, FColor::Red, FString::Printf(TEXT("No sessions found.")));}
	}
}

void UW_Menu::OnJoinSession(EOnJoinSessionCompleteResult::Type Result)
{
    IOnlineSubsystem* Subsystem = IOnlineSubsystem::Get();
    if(Subsystem && Result == EOnJoinSessionCompleteResult::Success)
    {
        IOnlineSessionPtr SessionInterface = Subsystem->GetSessionInterface();
        APlayerController* PlayerController = GetGameInstance()->GetFirstLocalPlayerController();
        FString Address;

        if(SessionInterface->GetResolvedConnectString(NAME_GameSession, Address) && PlayerController && SessionInterface)
        {
            PlayerController->ClientTravel(Address, ETravelType::TRAVEL_Absolute);
            if(GEngine){GEngine->AddOnScreenDebugMessage(-1, 10, FColor::Green, FString::Printf(TEXT("URL Connection : %s."), *Address));}
            return;
        }
        else 
        {
            if(GEngine){GEngine->AddOnScreenDebugMessage(-1, 10, FColor::Red, FString::Printf(TEXT("Impossible to join the session.")));}
        }
    }
    else if(!Subsystem)
    {
        if(GEngine){GEngine->AddOnScreenDebugMessage(-1, 10, FColor::Red, FString::Printf(TEXT("Impossible to get the IOnlineSubsystem")));}
    } 
    else if(!Result == EOnJoinSessionCompleteResult::Success)
    {
        if(GEngine){GEngine->AddOnScreenDebugMessage(-1, 10, FColor::Red, FString::Printf(TEXT("Impossible to join the session : %s"), EOnJoinSessionCompleteResult::Success));}
    }
    
    JoinButton->SetIsEnabled(true);
}

void UW_Menu::OnDestroySession(bool bWasSuccessful)
{
    
}

void UW_Menu::OnStartSession(bool bWasSuccessful)
{
    
}

/**
 * @brief Called when the player tavel into another map.
 * Clear the menu.
 * @param InLevel New map
 * @param InWorld New world
 */
void UW_Menu::OnLevelRemovedFromWorld(ULevel* InLevel, UWorld* InWorld)
{
    UW_Menu::MenuTearDown();
    Super::OnLevelRemovedFromWorld(InLevel, InWorld);
}

/**
 * @brief Clear the menu, called when we don't need it anymore.
 * 
 */
void UW_Menu::MenuTearDown()
{
    RemoveFromParent();
            UWorld* World = GetWorld();
        if(World)
        {
            APlayerController* PlayerController = World->GetFirstPlayerController();
            if(PlayerController)
            {
                FInputModeGameOnly InputModeGameOnly;
                PlayerController->SetInputMode(InputModeGameOnly);
                PlayerController->SetShowMouseCursor(false);
            }
            
        }
}
