// Fill out your copyright notice in the Description page of Project Settings.


#include "OverheadWidget.h"
#include "Components/TextBlock.h"


void UOverheadWidget::OnLevelRemovedFromWorld(ULevel* InLevel, UWorld* InWorld)
{
    RemoveFromParent();
    Super::OnLevelRemovedFromWorld(InLevel, InWorld);
}

FString UOverheadWidget::GetLocalRole(APawn* InPawn)
{
    // if(GEngine) GEngine->AddOnScreenDebugMessage(-1, 10, FColor::Green, FString::Printf(TEXT("GetLocalRole !")));
    // UE_LOG(LogTemp, Warning, TEXT("GetLocalRole"));

    ENetRole LocalRole = InPawn->GetLocalRole();
    FString Role;
    switch(LocalRole)
    {
        case ENetRole::ROLE_Authority:
            Role = FString("Authority"); // Player replicated on server
            break;
        
        case ENetRole::ROLE_AutonomousProxy:
            Role = FString("Autonomous Proxy"); // Player
            break;

        case ENetRole::ROLE_SimulatedProxy:
            Role = FString("Simulated Proxy"); // Player replicated on other clients
            break;
        
        case ENetRole::ROLE_None:
            Role = FString("None"); // Player replicated on other clients
            break;
    }
    Role = FString::Printf(TEXT("Local Role : %s"), *Role);
    return Role;
}

FString UOverheadWidget::GetRemoteRole(APawn* InPawn)
{
    // if(GEngine) GEngine->AddOnScreenDebugMessage(-1, 10, FColor::Green, FString::Printf(TEXT("GetRemoteRole !")));
    // UE_LOG(LogTemp, Warning, TEXT("GetRemoteRole"));

    ENetRole RemoteRole = InPawn->GetRemoteRole();
    FString Role;
    switch(RemoteRole)
    {
        case ENetRole::ROLE_Authority:
            Role = FString("Authority"); // Player replicated on server
            break;
        
        case ENetRole::ROLE_AutonomousProxy:
            Role = FString("Autonomous Proxy"); // Player
            break;

        case ENetRole::ROLE_SimulatedProxy:
            Role = FString("Simulated Proxy"); // Player replicated on other clients
            break;
        
        case ENetRole::ROLE_None:
            Role = FString("None"); // Player replicated on other clients
            break;
    }
    Role = FString::Printf(TEXT("Remote Role : %s"), *Role);
    return Role;
}


void UOverheadWidget::ShowPlayerNetRole(APawn* InPawn)
{
    if(DisplayRole)
    {
        // DisplayLocalRole->SetText(FText::FromString(GetLocalRole(InPawn)));
        // DisplayRemoteRole->SetText(FText::FromString(GetRemoteRole(InPawn)));
        DisplayRole->SetText(FText::FromString(GetRemoteRole(InPawn)));
    }
}
