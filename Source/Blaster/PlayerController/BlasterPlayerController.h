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

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	void SetHUDHealth(float Health, float MaxHealth);
	void SetHUDScore(float Score);
	void SetHUDDeaths(int32 Deaths);
	void SetHUDWeaponAmmo(int32 Ammo);
	void SetHUDCarriedAmmo(int32 CarriedAmmo);
	void SetHUDMatchCountDown(float CountDownTime);
	void SetHUDAnnouncementCountdown(float CountDownTime);

	virtual void OnPossess(APawn* InPawn) override;

	float ClientServerDelta = 0;  // difference between client and server time
	
	UPROPERTY(EditAnywhere, Category = Custom)
	float TimeSyncFrequency = 5.f; // Time to resync the client time with that of the server
	float TimeSyncRunningTime = 0;

	virtual float GetServerTime(); // Synced with server world clock
	virtual void ReceivedPlayer() override; // Synced with server clock as soon as possible
	void CheckTimeSync(float DeltaTime);

	void OnMatchStateSet(FName State);

protected:

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	void SetHUDTime();
	void PollInit(); // Set the character overlay only when it is ready (after warmup time)
	void HandleMatchHasStarted();
	void HandleCooldown();
	
	/**
	 * Sync time between client and server
	 */
	
	// Requests the current server time, passing in the client's time when the request was sent
	UFUNCTION(Server, Reliable)
	void Server_RequestServerTime(float TimeOfClientRequest);
	
	// Reports the current server time, to the client and response to Server_RequestServerTime()
	UFUNCTION(Client, Reliable)
	void Client_ReportServerTime(float TimeOfClientRequest, float TimeOfServerRequest);

	UFUNCTION(Server, Reliable)
	void Server_CheckMatchState();

	UFUNCTION(Client, Reliable)
	void Client_JoinMidGame(FName _MatchState, float _LevelStartingTime, float _WarmupTime, float _MatchTime, float _CooldownTime);

private:

	UPROPERTY()
	class ABlasterHUD* BlasterHUD;

	UPROPERTY()
	class ABlasterGameMode* BlasterGameMode;

	float MatchTime;
	float WarmupTime;
	float LevelStartingTime;
	float CooldownTime;
	uint32 CountdownInt = 0.f;

	UPROPERTY(ReplicatedUsing = OnRep_MatchState)
	FName MatchState;

	UFUNCTION()
	void OnRep_MatchState();

	/**
	 * Handle a cache for the HUD, the time that the game start
	 * 
	 */

	UPROPERTY()
	class UCharacterOverlay* CharacterOverlay;
	bool bInitializeCharacterOverlay = false;
	float CacheHUDHealth;
	float CacheHUDMaxHealth;
	float CacheHUDKill;
	int32 CacheHUDDeaths;

	UPROPERTY()
	class ABlasterCharacter* BlasterCharacter;

};
