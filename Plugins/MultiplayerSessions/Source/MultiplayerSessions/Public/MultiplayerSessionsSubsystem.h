// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "MultiplayerSessionsSubsystem.generated.h"

/**
 * @brief Declaring our own custom delegates for the Menu class to bind callbacks to
 * MULTICAST => Once it's broadcast, multiple classes can bind their functions to it
 * DYNAMIC => The Delegate can be serialized and can be savec or loaded from within a BP graph
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FCustomOnCreateSessionCompleteDelegate, bool, bWasSuccessul);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FCustomOnDestroySessionCompleteDelegate, bool, bWasSuccessul);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FCustomOnStartSessionCompleteDelegate, bool, bWasSuccessul);

// These can't be DYNAMIC because the array of online sessions search result is not a UClass
DECLARE_MULTICAST_DELEGATE_TwoParams(FCustomOnFindSessionsCompleteDelegate, const TArray<FOnlineSessionSearchResult>& SessionResults, bool bWasSuccessul); 
DECLARE_MULTICAST_DELEGATE_OneParam(FCustomOnJoinSessionCompleteDelegate, EOnJoinSessionCompleteResult::Type Result);


/**
 * @brief Main class of the plugin. This class is build in a way that it is independant of the W_Menu class.
 */
UCLASS()
class MULTIPLAYERSESSIONS_API UMultiplayerSessionsSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:

	UMultiplayerSessionsSubsystem();
	
	/**
	* @brief To handle session functionality. The menu class will call these.
	**/
	void CreateSession(int32 _NumPublicConnections, FString _MatchType);
	void FindSessions(int32 _MaxSearchResult);
	void JoinSession(const FOnlineSessionSearchResult& _SessionResult);
	void DestroySession();
	void StartSession();

	/**
	 * @brief Declaring our own custom delegates for the Menu class to bind callbacks to.
	*/
	FCustomOnCreateSessionCompleteDelegate CustomOnCreateSessionCompleteDelegate;
	FCustomOnFindSessionsCompleteDelegate CustomOnFindSessionsCompleteDelegate;
	FCustomOnJoinSessionCompleteDelegate CustomOnJoinSessionCompleteDelegate;
	FCustomOnDestroySessionCompleteDelegate CustomOnDestroySessionCompleteDelegate;
	FCustomOnStartSessionCompleteDelegate CustomOnStartSessionCompleteDelegate;

protected:


	/**
	 * @brief Internal callbacks for the delegates we'll add to the Online Session Interface delegate list.
	 */
	void OnCreateSessionComplete(FName SessionName, bool bWasSuccessfull);
	void OnFindSessionsComplete(bool bWasSuccessfull);
	void OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result);
	void OnDestroySessionComplete(FName SessionName, bool bWasSuccessfull);
	void OnStartSessionComplete(FName SessionName, bool bWasSuccessfull);


private:

	IOnlineSessionPtr SessionInterface;
	TSharedPtr<FOnlineSessionSettings> LastSessionSettings;

	/**
	 * @brief To add to the Online Session Interface delegate list.
	 * We'll bind our MultiplayerSessionsSubsystem internal callbacks menu to these.
	 * The first object is passed to the list of delegates of the interface.
	 * The second object is returned by the list of delegates of the interface.
	 */
	FOnCreateSessionCompleteDelegate CreateSessionCompleteDelegate;
	FDelegateHandle CreateSessionCompleteDelegateHandle;

	FOnFindSessionsCompleteDelegate FindSessionsCompleteDelegate;
	FDelegateHandle FindSessionsCompleteDelegateHandle;

	FOnJoinSessionCompleteDelegate JoinSessionCompleteDelegate;
	FDelegateHandle JoinSessionCompleteDelegateHandle;

	FOnDestroySessionCompleteDelegate DestroySessionCompleteDelegate;
	FDelegateHandle DestroySessionCompleteDelegateHandle;

	FOnStartSessionCompleteDelegate StartSessionCompleteDelegate;
	FDelegateHandle StartSessionCompleteDelegateHandle;

	TSharedPtr<FOnlineSessionSearch> LastSessionSearch;

	bool bCreateSessionOnDestroy{false};
	int32 LastNumPublicConnections;
	FString LastMatchType;

};
