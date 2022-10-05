// Fill out your copyright notice in the Description page of Project Settings.



#include "BlasterGameMode.h"
#include "Blaster/Characters/BlasterCharacter.h"
#include "Blaster/PlayerController/BlasterPlayerController.h"
#include "Blaster/PlayerState/BlasterPlayerState.h"
#include "Blaster/GameState/BlasterGameState.h"

#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerStart.h"

namespace MatchState
{
    const FName Cooldown = FName("CoolDown");
}

/**
 * Class only present on the server
 */
ABlasterGameMode::ABlasterGameMode()
{
    bDelayedStart = true; // Stay in the WaitingToStart State
}

void ABlasterGameMode::BeginPlay()
{
    Super::BeginPlay();
    LevelStartingTime = GetWorld()->GetTimeSeconds();
}

void ABlasterGameMode::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    if(MatchState == MatchState::WaitingToStart)
    {
        CountdownTime = LevelStartingTime + WarmupTime - GetWorld()->GetTimeSeconds();
        if(CountdownTime <= 0.f) StartMatch();
    }

    else if(MatchState == MatchState::InProgress)
    {
       CountdownTime = LevelStartingTime + WarmupTime + MatchTime - GetWorld()->GetTimeSeconds();
       if(CountdownTime <= 0.f) SetMatchState(MatchState::Cooldown);
    }

    else if(MatchState == MatchState::Cooldown)
    {
       CountdownTime = LevelStartingTime + WarmupTime + MatchTime + CooldownTime - GetWorld()->GetTimeSeconds();
        // BlasterCharacter->bDisableGameplay = true;
       if(CountdownTime <= 0) RestartGame();
    }
}

// Called at each changement of the game state
void ABlasterGameMode::OnMatchStateSet()
{
    Super::OnMatchStateSet();
    
    // Update the match state of each player controller
    for(FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
    {
        ABlasterPlayerController* BlasterPlayer = Cast<ABlasterPlayerController>(*It);
        if(BlasterPlayer)
        {
            BlasterPlayer->OnMatchStateSet(MatchState);
        }
    }
}

void ABlasterGameMode::PlayerEliminated(class ABlasterCharacter* EliminatedCharacter, class ABlasterPlayerController* VictimController, ABlasterPlayerController* AttackerController)
{
    ABlasterPlayerState* AttackerPlayerState = AttackerController ? Cast<ABlasterPlayerState>(AttackerController->PlayerState) : nullptr;
    ABlasterPlayerState* VictimPlayerState = VictimController ? Cast<ABlasterPlayerState>(VictimController->PlayerState) : nullptr;
    ABlasterGameState* BlasterGameState = GetGameState<ABlasterGameState>();

    // Check if it is NOT a suicide
    if(AttackerPlayerState && AttackerPlayerState != VictimPlayerState && BlasterGameState)
    {
        AttackerPlayerState->AddToScore(1.f);
        BlasterGameState->UpdateTopScore(AttackerPlayerState);
    }
    if(VictimPlayerState)
    {
        VictimPlayerState->AddToDeaths(1);
    }

    if(!EliminatedCharacter) {UE_LOG(LogTemp, Error, TEXT("BlasterGameMode.PlayerEliminated: EliminatedCharacter is null")); return;}
    EliminatedCharacter->Elim();
    
}

void ABlasterGameMode::RequestRespawn(ACharacter* EliminatedCharacter, AController* EliminatedController)
{
    if(!EliminatedCharacter) {UE_LOG(LogTemp, Error, TEXT("BlasterGameMode.RequestRespawn: EliminatedCharacter is null")); return;}
    
    EliminatedCharacter->Reset(); // Detach the controller from the character
    EliminatedCharacter->Destroy();

    if(!EliminatedController) {UE_LOG(LogTemp, Error, TEXT("BlasterGameMode.RequestRespawn: EliminatedController is null")); return;}
    
    TArray<AActor*> ArrayPlayerStart;
    UGameplayStatics::GetAllActorsOfClass(this, APlayerStart::StaticClass(), ArrayPlayerStart);
    int32 IndexPlayerStart = FMath::RandRange(0, ArrayPlayerStart.Num() - 1);
    RestartPlayerAtPlayerStart(EliminatedController, ArrayPlayerStart[IndexPlayerStart]);
}
