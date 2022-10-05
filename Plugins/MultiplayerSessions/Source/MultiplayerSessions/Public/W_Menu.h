// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MultiplayerSessionsSubsystem.h"
#include "W_Menu.generated.h"

UCLASS()
class MULTIPLAYERSESSIONS_API UW_Menu : public UUserWidget
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintCallable)
	void MenuSetup(int32 _NumPublicConnections = 4, FString _MatchType = FString(TEXT("FreeForAll")), FString _PathToLobby = "/Game/ThirdPerson/Maps/Lobby");

protected:

	// Constructor
	virtual bool Initialize() override;
	virtual void OnLevelRemovedFromWorld(ULevel* InLevel, UWorld* InWorld) override;

	/**
	 * @brief Callbacks for the custom delegates on the MultiplayerSessionsSubsystem
	 */
	UFUNCTION()
	void OnCreateSession(bool bWasSuccessful);
	void OnFindSessions(const TArray<FOnlineSessionSearchResult>& SessionResult, bool bWasSuccessful);
	void OnJoinSession(EOnJoinSessionCompleteResult::Type Result);

	UFUNCTION()
	void OnDestroySession(bool bWasSuccessful);

	UFUNCTION()
	void OnStartSession(bool bWasSuccessful);

private:

	UPROPERTY(meta = (BindWidget)) // Link to the variable HostButton in the BP widget children
	class UButton* HostButton;

	UPROPERTY(meta = (BindWidget)) // Link to the variable JoinButton in the BP widget children
	class UButton* JoinButton;

	UFUNCTION()
	void HostButtonClicked();

	UFUNCTION()
	void JoinButtonClicked();

	void MenuTearDown();

	// Our custom Subsystem designed to handle all online session functioality
	UMultiplayerSessionsSubsystem* MultiplayerSessionsSubsystem;

	int32 NumPublicConnections{4};
	FString MatchType{TEXT("FreeForAll")};
	FString PathToLobby{TEXT("")};

};
