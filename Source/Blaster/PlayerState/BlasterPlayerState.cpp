// Fill out your copyright notice in the Description page of Project Settings.


#include "BlasterPlayerState.h"
#include "Blaster/Characters/BlasterCharacter.h"
#include "Blaster/PlayerController/BlasterPlayerController.h"

#include "Net/UnrealNetwork.h"



void ABlasterPlayerState::GetLifetimeReplicatedProps(TArray< FLifetimeProperty > & OutLifetimeProps) const 
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(ABlasterPlayerState, Deaths);
}


void ABlasterPlayerState::AddToScore(float ScoreAmount)
{
    SetScore(Score + ScoreAmount);

    Character = Character == nullptr ? Cast<ABlasterCharacter>(GetPawn()) : Character;
    if(!Character) return;

    Controller = Controller == nullptr ? Cast<ABlasterPlayerController>(Character->Controller) : Controller;
    if(!Controller) return;
    
    Controller->SetHUDScore(Score);
}


void ABlasterPlayerState::OnRep_Score()
{
    Super::OnRep_Score();

    Character = Character == nullptr ? Cast<ABlasterCharacter>(GetPawn()) : Character;
    if(!Character) {UE_LOG(LogTemp, Error, TEXT("BlasterPlayerState.OnRep_Score: Character is null")); return;}

    Controller = Controller == nullptr ? Cast<ABlasterPlayerController>(Character->Controller) : Controller;
    if(!Controller) {UE_LOG(LogTemp, Error, TEXT("BlasterPlayerState.OnRep_Score: Controller is null")); return;}

    Controller->SetHUDScore(Score);
}

void ABlasterPlayerState::AddToDeaths(int32 DeathsAmount)
{
    Deaths += DeathsAmount;

    Character = Character == nullptr ? Cast<ABlasterCharacter>(GetPawn()) : Character;
    if(!Character) return;

    Controller = Controller == nullptr ? Cast<ABlasterPlayerController>(Character->Controller) : Controller;
    if(!Controller) return;
    
    Controller->SetHUDDeaths(Deaths);
}

void ABlasterPlayerState::OnRep_Deaths()
{
    Character = Character == nullptr ? Cast<ABlasterCharacter>(GetPawn()) : Character;
    if(!Character) {UE_LOG(LogTemp, Error, TEXT("BlasterPlayerState.OnRep_Deaths: Character is null")); return;}

    Controller = Controller == nullptr ? Cast<ABlasterPlayerController>(Character->Controller) : Controller;
    if(!Controller) {UE_LOG(LogTemp, Error, TEXT("BlasterPlayerState.OnRep_Deaths: Controller is null")); return;}
    
    Controller->SetHUDDeaths(Deaths);
}