// Fill out your copyright notice in the Description page of Project Settings.


#include "BlasterPlayerController.h"
#include "Blaster/HUD/BlasterHUD.h"
#include "Blaster/HUD/CharacterOverlay.h"
#include "Blaster/HUD/AnnouncementOverlay.h"
#include "Blaster/Characters/BlasterCharacter.h"
#include "Blaster/GameMode/BlasterGameMode.h"
#include "Blaster/GameState/BlasterGameState.h"
#include "Blaster/PlayerState/BlasterPlayerState.h"
#include "Blaster/BlasterComponents/CombatComponent.h"

#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"

void ABlasterPlayerController::BeginPlay()
{
    Super::BeginPlay();

    // BlasterCharacter = Cast<ABlasterCharacter>(GetPawn());
    BlasterHUD = Cast<ABlasterHUD>(GetHUD());
    BlasterGameMode = BlasterGameMode == nullptr ? Cast<ABlasterGameMode>(UGameplayStatics::GetGameMode(this)) : BlasterGameMode;

    Server_CheckMatchState();    
}

void ABlasterPlayerController::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const 
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ABlasterPlayerController, MatchState); // Replicate only to the owner of this instance class (one client)
}

void ABlasterPlayerController::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    SetHUDTime();
    TimeSyncRunningTime += DeltaTime;
    CheckTimeSync(DeltaTime); // Every TimeSyncFrequency seconds, we resync the client time with that of the server
    PollInit();
}

// Set the character overlay only when it is ready (after warmup time)
void ABlasterPlayerController::PollInit()
{
    if(CharacterOverlay == nullptr)
    {
        if(BlasterHUD && BlasterHUD->CharacterOverlay)
        {
            CharacterOverlay = BlasterHUD->CharacterOverlay;
            if(CharacterOverlay)
            {   
                SetHUDHealth(CacheHUDHealth, CacheHUDMaxHealth);
                SetHUDScore(CacheHUDKill);
                SetHUDDeaths(CacheHUDDeaths);
            }
        }
    }
}

// Every TimeSyncFrequency seconds, we resync the client time with that of the server
void ABlasterPlayerController::CheckTimeSync(float DeltaTime)
{
    
    if(IsLocalController() && TimeSyncRunningTime > TimeSyncFrequency)
    {
        Server_RequestServerTime(GetWorld()->GetTimeSeconds());
        TimeSyncRunningTime = 0.f;
    }
}

void ABlasterPlayerController::OnPossess(APawn* InPawn)
{
    Super::OnPossess(InPawn);
    BlasterCharacter = Cast<ABlasterCharacter>(InPawn);
    if(!BlasterCharacter) return;
    SetHUDHealth(BlasterCharacter->GetHealth(), BlasterCharacter->GetMaxHealth());
}

void ABlasterPlayerController::SetHUDHealth(float Health, float MaxHealth)
{
    BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;
    if(BlasterHUD && BlasterHUD->CharacterOverlay && BlasterHUD->CharacterOverlay->HealthBar && BlasterHUD->CharacterOverlay->HealthText)
    {
        const float HealthPercent = Health / MaxHealth;
        BlasterHUD->CharacterOverlay->HealthBar->SetPercent(HealthPercent);
        
        FString HealthText = FString::Printf(TEXT("%d/%d"), FMath::CeilToInt(Health), FMath::CeilToInt(MaxHealth));
        BlasterHUD->CharacterOverlay->HealthText->SetText(FText::FromString(HealthText));
    }
    else
    {
        bInitializeCharacterOverlay = true;
        CacheHUDHealth = Health;
	    CacheHUDMaxHealth = MaxHealth;
    }
}

void ABlasterPlayerController::SetHUDScore(float Score)
{
    BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;
    if(BlasterHUD && BlasterHUD->CharacterOverlay && BlasterHUD->CharacterOverlay->ScoreAmount)
    {
        FString ScoreText = FString::Printf(TEXT("%d"), FMath::FloorToInt(Score));
        BlasterHUD->CharacterOverlay->ScoreAmount->SetText(FText::FromString(ScoreText));
    }
    else
    {
        bInitializeCharacterOverlay = true;
        CacheHUDKill = Score;
    }
}

void ABlasterPlayerController::SetHUDDeaths(int32 Deaths)
{
    BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;
    if(BlasterHUD && BlasterHUD->CharacterOverlay && BlasterHUD->CharacterOverlay->DeathsAmount)
    {
        FString DeathsText = FString::Printf(TEXT("%d"), Deaths);
        BlasterHUD->CharacterOverlay->DeathsAmount->SetText(FText::FromString(DeathsText));
    } 
    else
    {
        bInitializeCharacterOverlay = true;
        CacheHUDDeaths = Deaths;
    }
}

void ABlasterPlayerController::SetHUDWeaponAmmo(int32 Ammo)
{
    BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;
    if(!BlasterHUD || !BlasterHUD->CharacterOverlay || !BlasterHUD->CharacterOverlay->WeaponAmmoAmount) return;
    
    FString AmmoText = FString::Printf(TEXT("%d"), Ammo);
    BlasterHUD->CharacterOverlay->WeaponAmmoAmount->SetText(FText::FromString(AmmoText));
}

void ABlasterPlayerController::SetHUDCarriedAmmo(int32 CarriedAmmo)
{
    BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;
    if(!BlasterHUD || !BlasterHUD->CharacterOverlay || !BlasterHUD->CharacterOverlay->CarriedAmmoAmount) return;
    
    FString CarriedAmmoText = FString::Printf(TEXT("%d"), CarriedAmmo);
    BlasterHUD->CharacterOverlay->CarriedAmmoAmount->SetText(FText::FromString(CarriedAmmoText));
}

void ABlasterPlayerController::SetHUDMatchCountDown(float CountDownTime)
{
    BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;
    if(!BlasterHUD || !BlasterHUD->CharacterOverlay || !BlasterHUD->CharacterOverlay->MatchCountDownText) return;

    if(CountDownTime < 0.f)
    {
        BlasterHUD->AnnouncementOverlay->MatchStartsInText->SetText(FText());
        return;
    }

    int32 Minutes = FMath::FloorToInt(CountDownTime / 60);
    int32 Seconds = CountDownTime - Minutes * 60;

    FString CooldownTimeText = FString::Printf(TEXT("%02d:%02d"), Minutes, Seconds); // 02 pour 2 digit
    BlasterHUD->CharacterOverlay->MatchCountDownText->SetText(FText::FromString(CooldownTimeText));
}

void ABlasterPlayerController::SetHUDAnnouncementCountdown(float CountDownTime)
{
    BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;
    if(!BlasterHUD || !BlasterHUD->AnnouncementOverlay|| !BlasterHUD->AnnouncementOverlay->WarmTime) return;
    
    if(CountDownTime < 0.f)
    {
        BlasterHUD->AnnouncementOverlay->WarmTime->SetText(FText());    
        return;
    }

    int32 Minutes = FMath::FloorToInt(CountDownTime / 60);
    int32 Seconds = CountDownTime - Minutes * 60;

    FString CooldownTimeText = FString::Printf(TEXT("%02d:%02d"), Minutes, Seconds); // 02 pour 2 digit
    BlasterHUD->AnnouncementOverlay->WarmTime->SetText(FText::FromString(CooldownTimeText));
}

void ABlasterPlayerController::SetHUDTime()
{   
    uint32 SecondsLeft = 0.f;
    float TimeLeft = 0.f;

    if(HasAuthority())
    {
        if(BlasterGameMode)
        {
            TimeLeft = BlasterGameMode->GetCountdownTime() + LevelStartingTime;
            SecondsLeft = FMath::CeilToInt(BlasterGameMode->GetCountdownTime() + LevelStartingTime);
        }
    }
    else
    {
        if(MatchState == MatchState::WaitingToStart) TimeLeft = LevelStartingTime + WarmupTime - GetServerTime();
        else if(MatchState == MatchState::InProgress) TimeLeft = LevelStartingTime + WarmupTime + MatchTime - GetServerTime();
        else if(MatchState == MatchState::Cooldown) TimeLeft = LevelStartingTime + WarmupTime + MatchTime + CooldownTime - GetServerTime();
        SecondsLeft = FMath::CeilToInt(TimeLeft);
    }
    
    if(CountdownInt != SecondsLeft) // We update only when there is one second of difference
    {
        if(MatchState == MatchState::WaitingToStart || MatchState == MatchState::Cooldown)
        {
            SetHUDAnnouncementCountdown(TimeLeft);
        }
        else if(MatchState == MatchState::InProgress)
        {
            SetHUDMatchCountDown(TimeLeft);
        }
    }

    CountdownInt = SecondsLeft;
}

// Requests the current server time, passing in the client's time when the request was sent
void ABlasterPlayerController::Server_RequestServerTime_Implementation(float TimeOfClientRequest)
{
    float ServerTimeOfReceipt = GetWorld()->GetTimeSeconds();

    Client_ReportServerTime(TimeOfClientRequest, ServerTimeOfReceipt);
}
	
// Reports the current server time, to the client and response to Server_RequestServerTime()
void ABlasterPlayerController::Client_ReportServerTime_Implementation(float TimeOfClientRequest, float TimeOfServerRequest)
{
    // Corresponds to the travel time of the Server_RequestServerTime(), 
    // from the client to the server, and then the travel time of the response from the server to the client.
   float RoundTripTime = GetWorld()->GetTimeSeconds() - TimeOfClientRequest;
   
   // We use "RoundTripTime / 2" because we add the time to make travel from the server to the client
   float CurrentServerTime = TimeOfServerRequest + RoundTripTime / 2;

   ClientServerDelta = CurrentServerTime - GetWorld()->GetTimeSeconds();
}

// Synced with server world clock
float ABlasterPlayerController::GetServerTime()
{
    if(HasAuthority())
    {
        return GetWorld()->GetTimeSeconds();
    }
    else 
    {
        return GetWorld()->GetTimeSeconds() + ClientServerDelta;
    }
}

// One of earliest function called by the instance
void ABlasterPlayerController::ReceivedPlayer()
{
    Super::ReceivedPlayer();
    if(IsLocalController())
    {
        Server_RequestServerTime(GetWorld()->GetTimeSeconds());
    }
}

void ABlasterPlayerController::OnMatchStateSet(FName State)
{
    MatchState = State;

    if(MatchState == MatchState::InProgress)
    {
        HandleMatchHasStarted();
    }
    else if(MatchState == MatchState::Cooldown)
    {
        HandleCooldown();
    }
}

void ABlasterPlayerController::OnRep_MatchState()
{
    if(MatchState == MatchState::InProgress)
    {
        HandleMatchHasStarted();
    }
    else if(MatchState == MatchState::Cooldown)
    {
        HandleCooldown();
    }
}

void ABlasterPlayerController::Server_CheckMatchState_Implementation()
{
    // ABlasterGameMode* BlasterGameMode = Cast<ABlasterGameMode>(UGameplayStatics::GetGameMode(this));
    if(BlasterGameMode)
    {
        WarmupTime = BlasterGameMode->WarmupTime;
        MatchTime = BlasterGameMode->MatchTime;
        LevelStartingTime = BlasterGameMode->LevelStartingTime;
        CooldownTime = BlasterGameMode->CooldownTime;
        MatchState = BlasterGameMode->GetMatchState();
        Client_JoinMidGame(MatchState, LevelStartingTime, WarmupTime, MatchTime, CooldownTime);
    }
}

void ABlasterPlayerController::Client_JoinMidGame_Implementation(FName _MatchState, float _LevelStartingTime, float _WarmupTime, float _MatchTime, float _CooldownTime)
{
    LevelStartingTime = _LevelStartingTime;
    WarmupTime = _WarmupTime;
    MatchTime = _MatchTime;
    CooldownTime = _CooldownTime;
    OnMatchStateSet(MatchState);
    
    if (BlasterHUD && MatchState == MatchState::WaitingToStart)
	{
		BlasterHUD->AddAnnouncementOverlay();
	}
}

void ABlasterPlayerController::HandleMatchHasStarted()
{
    BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;
    if(BlasterHUD)
    {
        BlasterHUD->AddCharacterOverlay();
        if(!BlasterHUD->AnnouncementOverlay) return;
        BlasterHUD->AnnouncementOverlay->SetVisibility(ESlateVisibility::Hidden);
    }
}

// When the games has finished, manage our HUD with our custom Match state "Cooldown"
void ABlasterPlayerController::HandleCooldown()
{
    BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;
    if(BlasterHUD && BlasterHUD->CharacterOverlay && BlasterHUD->AnnouncementOverlay && BlasterHUD->AnnouncementOverlay->MatchStartsInText)
    {
        BlasterHUD->CharacterOverlay->RemoveFromParent();
        BlasterHUD->AnnouncementOverlay->SetVisibility(ESlateVisibility::Visible);
        BlasterHUD->AnnouncementOverlay->MatchStartsInText->SetText(FText::FromString("New match in :"));
        BlasterHUD->AnnouncementOverlay->FlyArroundText->SetVisibility(ESlateVisibility::Hidden);
        ABlasterGameState* BlasterGameState = Cast<ABlasterGameState>(UGameplayStatics::GetGameState(this));
        ABlasterPlayerState* BlasterPlayerState = GetPlayerState<ABlasterPlayerState>();
        
        if(BlasterGameState && BlasterPlayerState)
        {
            BlasterHUD->AnnouncementOverlay->FlyArroundText->SetVisibility(ESlateVisibility::Visible);
            TArray<ABlasterPlayerState*> TopPlayers = BlasterGameState->TopScoringPlayers;

            // NO WINNERS
            if(TopPlayers.Num() == 0)
            {
                // BlasterHUD->AnnouncementOverlay->FlyArroundText->SetText(FText::FromString("No winners, all loosers."));
                BlasterHUD->AnnouncementOverlay->FlyArroundText->SetText(FText::FromString("Aucun gagnant, que des perdants."));
            }
            // YOU WIN
            else if (TopPlayers.Num() == 1 && TopPlayers[0] == BlasterPlayerState)
            {
                int RandomText = FMath::RandRange(1, 5);
                RandomText = 4;
                if(RandomText == 1) BlasterHUD->AnnouncementOverlay->FlyArroundText->SetText(FText::FromString("Felicitations, vous avez ete meilleur\n que tous les joueurs de cette partie !"));
                else if (RandomText == 2) BlasterHUD->AnnouncementOverlay->FlyArroundText->SetText(FText::FromString("Continue comme ça fiston ! T'es le meilleur !"));
                else if (RandomText == 3) BlasterHUD->AnnouncementOverlay->FlyArroundText->SetText(FText::FromString("T'es le GOAT. Rien d'autre a dire."));
                else if (RandomText == 4) BlasterHUD->AnnouncementOverlay->FlyArroundText->SetText(FText::FromString("T'as pense a faire les Worlds ?\n Parce que t'es le meilleur."));
                else if (RandomText == 5) BlasterHUD->AnnouncementOverlay->FlyArroundText->SetText(FText::FromString("Avec ton niveau, c'est l'heure de stream, champion."));
                // if(RandomText == 1) BlasterHUD->AnnouncementOverlay->FlyArroundText->SetText(FText::FromString("Congratulations, you were better than all the players in this game!"));
                // else if (RandomText == 2) BlasterHUD->AnnouncementOverlay->FlyArroundText->SetText(FText::FromString("Keep it up son! You are the best!"));
                // else if (RandomText == 3) BlasterHUD->AnnouncementOverlay->FlyArroundText->SetText(FText::FromString("You're the GOAT. Nothing more to say."));
                // else if (RandomText == 4) BlasterHUD->AnnouncementOverlay->FlyArroundText->SetText(FText::FromString("Have you thought about doing Worlds competion? Because you are the best."));
                // else if (RandomText == 5) BlasterHUD->AnnouncementOverlay->FlyArroundText->SetText(FText::FromString("With your level, it's time for stream, champion."));
            }
            // YOU LOOSE
            else if (TopPlayers.Num() == 1 && TopPlayers[0] != BlasterPlayerState)
            {
                int RandomText = FMath::RandRange(1, 5);
                RandomText = 5;
                if(RandomText == 1) BlasterHUD->AnnouncementOverlay->FlyArroundText->SetText(FText::FromString("T'es pas le meilleur, mais c'est surement qu'ils ont triche..."));
                else if (RandomText == 2) BlasterHUD->AnnouncementOverlay->FlyArroundText->SetText(FText::FromString("Si tu perds, c'est qu'ils sont dopes."));
                else if (RandomText == 3) BlasterHUD->AnnouncementOverlay->FlyArroundText->SetText(FText::FromString("T'as perdu, mais tu jouais en clignant des yeux..."));
                else if (RandomText == 4) BlasterHUD->AnnouncementOverlay->FlyArroundText->SetText(FText::FromString("C'est pas grave de perdre contre un no life."));
                else if (RandomText == 5) BlasterHUD->AnnouncementOverlay->FlyArroundText->SetText(FText::FromString("Change ton mot de passe.\n La seule raison possible pour que tu aies perdu\n c'est qu'ils t'aient pirate..."));
                // if(RandomText == 1) BlasterHUD->AnnouncementOverlay->FlyArroundText->SetText(FText::FromString("You're not the best, but it's probably because they cheated..."));
                // else if (RandomText == 2) BlasterHUD->AnnouncementOverlay->FlyArroundText->SetText(FText::FromString("If you lose, they are doped."));
                // else if (RandomText == 3) BlasterHUD->AnnouncementOverlay->FlyArroundText->SetText(FText::FromString("You lost, but you were playing blinking..."));
                // else if (RandomText == 4) BlasterHUD->AnnouncementOverlay->FlyArroundText->SetText(FText::FromString("It's okay to lose against a no life."));
                // else if (RandomText == 5) BlasterHUD->AnnouncementOverlay->FlyArroundText->SetText(FText::FromString("Change your password. The only possible reason you lost it is because they hacked you..."));
            }
            else if (TopPlayers.Num() > 1 && TopPlayers.Contains(BlasterPlayerState))
            {
                // FString InfoTextString = "Players tied for the win:\n";
                FString InfoTextString = "On est sur une égalité, c'est pas bon du tout ça, comment tu veux être champion du monde ?\n Regarde tes concurrents :\n";
                for(auto TiedPlayer : TopPlayers)
                {
                    InfoTextString.Append(FString::Printf(TEXT("%s\n"), *TiedPlayer->GetPlayerName()));
                }
            }            
        }        
    }
    
    if(BlasterCharacter && BlasterCharacter->GetCombatComponent())
    {
        BlasterCharacter->bDisableGameplay = true;
        BlasterCharacter->GetCombatComponent()->FireButtonPressed(false);
    }   
}