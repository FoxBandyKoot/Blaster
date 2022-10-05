// Fill out your copyright notice in the Description page of Project Settings.


#include "MultiplayerSessionsSubsystem.h"
#include "OnlineSubsystem.h"
#include "OnlineSessionSettings.h"

UMultiplayerSessionsSubsystem::UMultiplayerSessionsSubsystem():
	// Initialise .h variables (Construct delegates which bind action functions to callback functions)
    CreateSessionCompleteDelegate(FOnCreateSessionCompleteDelegate::CreateUObject(this, &ThisClass::OnCreateSessionComplete)),
    FindSessionsCompleteDelegate(FOnFindSessionsCompleteDelegate::CreateUObject(this, &ThisClass::OnFindSessionsComplete)),
    JoinSessionCompleteDelegate(FOnJoinSessionCompleteDelegate::CreateUObject(this, &ThisClass::OnJoinSessionComplete)),
    DestroySessionCompleteDelegate(FOnDestroySessionCompleteDelegate::CreateUObject(this, &ThisClass::OnDestroySessionComplete)),
    StartSessionCompleteDelegate(FOnStartSessionCompleteDelegate::CreateUObject(this, &ThisClass::OnStartSessionComplete))
{
    IOnlineSubsystem* Subsystem = IOnlineSubsystem::Get();
    if(Subsystem)
    {
        SessionInterface = Subsystem->GetSessionInterface();
    }	
}


void UMultiplayerSessionsSubsystem::CreateSession(int32 _NumPublicConnections, FString _MatchType)
{
    if(!SessionInterface.IsValid())
    {
        return;
    }

    // Before to create a new session, we need to delete a session with the same name, if she exists
	FNamedOnlineSession* ExistingSession = SessionInterface->GetNamedSession(NAME_GameSession);
	if(ExistingSession)
	{
        if(GEngine){GEngine->AddOnScreenDebugMessage(-1, 10, FColor::Red, FString::Printf(TEXT("Start destroying an old session.")));}
        bCreateSessionOnDestroy = true;
	    LastNumPublicConnections = _NumPublicConnections;
	    LastMatchType = _MatchType;
        DestroySession();
	} 
    else
    {
        // The interface has a list of "delegates", which are objects which react to events and trigger callback functions
        CreateSessionCompleteDelegateHandle = SessionInterface->AddOnCreateSessionCompleteDelegate_Handle(CreateSessionCompleteDelegate);

        LastSessionSettings = MakeShareable(new FOnlineSessionSettings()); // MakeShareable allows to init SharedPointer, we give it a constructor as a parameter
        LastSessionSettings->bIsLANMatch = IOnlineSubsystem::Get()->GetSubsystemName() == "NULL" ? true : false;
        LastSessionSettings->NumPublicConnections = _NumPublicConnections;
        LastSessionSettings->bAllowJoinInProgress = true; // Allow player s to join even if the session started
        LastSessionSettings->bAllowJoinViaPresence = true; // Allows Steam to let players of the closest region join the server
        LastSessionSettings->bShouldAdvertise = true; // Allows Steam to advertise the session
        LastSessionSettings->bUsesPresence = true; // Allows Steam to search players which belongs to the region of the server in priority
        LastSessionSettings->bUseLobbiesIfAvailable = true; 
        LastSessionSettings->Set(FName("MatchType"), _MatchType, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
        LastSessionSettings->BuildUniqueId = 1; // Allow to find other hosted sessions

        const ULocalPlayer* LocalPlayer = GetWorld()->GetFirstLocalPlayerFromController();
        if(!SessionInterface->CreateSession(*LocalPlayer->GetPreferredUniqueNetId(), NAME_GameSession, *LastSessionSettings))
        {
            // Remove the delegate handle from the list if the creation failed
            SessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(CreateSessionCompleteDelegateHandle);

            // Our own custom delegate broadcast to the UW_Menu
            CustomOnCreateSessionCompleteDelegate.Broadcast(false);
        }
    }
}

void UMultiplayerSessionsSubsystem::OnCreateSessionComplete(FName SessionName, bool bWasSuccessfull)
{
    if(SessionInterface)
    {
        SessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(CreateSessionCompleteDelegateHandle);
    }

    // Broadcast our own custom delegate to the UW_Menu
    CustomOnCreateSessionCompleteDelegate.Broadcast(bWasSuccessfull);
}


void UMultiplayerSessionsSubsystem::FindSessions(int32 _MaxSearchResult)
{
    if(!SessionInterface)
    {
        if(GEngine){GEngine->AddOnScreenDebugMessage(-1, 10, FColor::Red, FString::Printf(TEXT("Session Interface  is Invalid.")));}
		return;    
    }

    FindSessionsCompleteDelegateHandle = SessionInterface->AddOnFindSessionsCompleteDelegate_Handle(FindSessionsCompleteDelegate);

	//Find Game Sessions
	LastSessionSearch = MakeShareable(new FOnlineSessionSearch());
	LastSessionSearch->MaxSearchResults = _MaxSearchResult;
	LastSessionSearch->bIsLanQuery = false;
	LastSessionSearch->QuerySettings.Set(SEARCH_PRESENCE, true, EOnlineComparisonOp::Equals);

	const ULocalPlayer* LocalPlayer = GetWorld()->GetFirstLocalPlayerFromController();
	if(!SessionInterface->FindSessions(*LocalPlayer->GetPreferredUniqueNetId(), LastSessionSearch.ToSharedRef()))
    {
        // Remove the delegate handle from the list if the creation failed
        SessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(FindSessionsCompleteDelegateHandle);

        // Our own custom delegate broadcast to the UW_Menu
        CustomOnFindSessionsCompleteDelegate.Broadcast(TArray<FOnlineSessionSearchResult>(), false);
    }

	if(GEngine){GEngine->AddOnScreenDebugMessage(-1, 10, FColor::Red, FString::Printf(TEXT("FindSessions called.")));}
}

void UMultiplayerSessionsSubsystem::OnFindSessionsComplete(bool bWasSuccessfull)
{
    if(SessionInterface)
    {
        SessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(FindSessionsCompleteDelegateHandle);
    }

    if(LastSessionSearch->SearchResults.Num() == 0)
    {
        // Our own custom delegate broadcast to the UW_Menu
        CustomOnFindSessionsCompleteDelegate.Broadcast(TArray<FOnlineSessionSearchResult>(), false);
        return;
    }

    // Our own custom delegate broadcast to the UW_Menu
    CustomOnFindSessionsCompleteDelegate.Broadcast(LastSessionSearch->SearchResults, bWasSuccessfull);
}

void UMultiplayerSessionsSubsystem::JoinSession(const FOnlineSessionSearchResult& Session)
{

     if(!SessionInterface)
    {
        if(GEngine){GEngine->AddOnScreenDebugMessage(-1, 10, FColor::Red, FString::Printf(TEXT("Session Interface  is invalid.")));}
        CustomOnJoinSessionCompleteDelegate.Broadcast(EOnJoinSessionCompleteResult::UnknownError);
		return;    
    }

    JoinSessionCompleteDelegateHandle = SessionInterface->AddOnJoinSessionCompleteDelegate_Handle(JoinSessionCompleteDelegate);

    if(GEngine)
    {		
        FString SessionFound_MatchType;
        Session.Session.SessionSettings.Get(FName("MatchType"), SessionFound_MatchType); // MatchType is an OutParameter
    	GEngine->AddOnScreenDebugMessage(-1, 10, FColor::Green, FString::Printf(TEXT("My User Id : %s."), *Session.GetSessionIdStr()));
    	GEngine->AddOnScreenDebugMessage(-1, 10, FColor::Green, FString::Printf(TEXT("My User Name : %s."), *Session.Session.OwningUserName));
    	GEngine->AddOnScreenDebugMessage(-1, 10, FColor::Green, FString::Printf(TEXT("Joining Match type : %s."), *SessionFound_MatchType));
    }
				
    const ULocalPlayer* LocalPlayer = GetWorld()->GetFirstLocalPlayerFromController();
    if(!SessionInterface->JoinSession(*LocalPlayer->GetPreferredUniqueNetId(), NAME_GameSession, Session))
    {
        SessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(JoinSessionCompleteDelegateHandle);
        if(GEngine){GEngine->AddOnScreenDebugMessage(-1, 10, FColor::Red, FString::Printf(TEXT("Session Interface  is invalid.")));}
        CustomOnJoinSessionCompleteDelegate.Broadcast(EOnJoinSessionCompleteResult::UnknownError);
    }

}

void UMultiplayerSessionsSubsystem::OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result)
{
    if(!SessionInterface.IsValid())
	{
        SessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(JoinSessionCompleteDelegateHandle);
        if(GEngine){GEngine->AddOnScreenDebugMessage(-1, 10, FColor::Red, FString::Printf(TEXT("SessionInterface is invalid.")));}
        CustomOnJoinSessionCompleteDelegate.Broadcast(EOnJoinSessionCompleteResult::UnknownError);
		return;
	}
    CustomOnJoinSessionCompleteDelegate.Broadcast(Result);
}

void UMultiplayerSessionsSubsystem::DestroySession()
{
    if(!SessionInterface)
    {
        CustomOnDestroySessionCompleteDelegate.Broadcast(false);
        if(GEngine){GEngine->AddOnScreenDebugMessage(-1, 10, FColor::Red, FString::Printf(TEXT("SessionInterface is invalid.")));}
        return;
    }

    DestroySessionCompleteDelegateHandle = SessionInterface->AddOnDestroySessionCompleteDelegate_Handle(DestroySessionCompleteDelegate);
    
    if(!SessionInterface->DestroySession(NAME_GameSession))
    {
        CustomOnDestroySessionCompleteDelegate.Broadcast(false);
        SessionInterface->ClearOnDestroySessionCompleteDelegate_Handle(DestroySessionCompleteDelegateHandle);
        if(GEngine){GEngine->AddOnScreenDebugMessage(-1, 10, FColor::Red, FString::Printf(TEXT("Failed to destroy the session : %s."), NAME_GameSession));}
    }
}

void UMultiplayerSessionsSubsystem::OnDestroySessionComplete(FName SessionName, bool bWasSuccessfull)
{
    if(!SessionInterface)
    {
        SessionInterface->ClearOnDestroySessionCompleteDelegate_Handle(DestroySessionCompleteDelegateHandle);
        if(GEngine){GEngine->AddOnScreenDebugMessage(-1, 10, FColor::Red, FString::Printf(TEXT("SessionInterface is invalid.")));}
        return;
    }
    if(bWasSuccessfull && bCreateSessionOnDestroy)
    {
        bCreateSessionOnDestroy = false;
        CreateSession(LastNumPublicConnections, LastMatchType);
    }
    CustomOnDestroySessionCompleteDelegate.Broadcast(bWasSuccessfull);
}

void UMultiplayerSessionsSubsystem::StartSession()
{
    
}

void UMultiplayerSessionsSubsystem::OnStartSessionComplete(FName SessionName, bool bWasSuccessfull)
{
    
}