// Fill out your copyright notice in the Description page of Project Settings.


#include "BlasterHUD.h"
#include "GameFramework/PlayerController.h"
#include "CharacterOverlay.h"
#include "AnnouncementOverlay.h"

void ABlasterHUD::BeginPlay()
{
    Super::BeginPlay();
}

void ABlasterHUD::AddCharacterOverlay()
{
    APlayerController* PlayerController = GetOwningPlayerController();
    if(!PlayerController) {UE_LOG(LogTemp, Error, TEXT("ABlasterHUD.AddCharacterOverlay: PlayerController is null")); return;}
    if(!CharacterOverlayClass) {UE_LOG(LogTemp, Error, TEXT("ABlasterHUD.AddCharacterOverlay: CharacterOverlayClass is null")); return;}

    CharacterOverlay = CreateWidget<UCharacterOverlay>(PlayerController, CharacterOverlayClass);
    CharacterOverlay->AddToViewport();
}


void ABlasterHUD::AddAnnouncementOverlay()
{
    APlayerController* PlayerController = GetOwningPlayerController();
    if(!PlayerController) {UE_LOG(LogTemp, Error, TEXT("ABlasterHUD.AddAnnouncementOverlay: PlayerController is null")); return;}
    if(!AnnouncementOverlayClass) {UE_LOG(LogTemp, Error, TEXT("ABlasterHUD.AddAnnouncementOverlay: UAnnouncementOverlayClass is null")); return;}
    
    AnnouncementOverlay = CreateWidget<UAnnouncementOverlay>(PlayerController, AnnouncementOverlayClass);
    AnnouncementOverlay->AddToViewport();
}


void ABlasterHUD::DrawHUD()
{
    Super::DrawHUD();
    FVector2D ViewportSize;
    if(!GEngine) {UE_LOG(LogTemp, Error, TEXT("ABlasterHUD.DrawHUD: GEngine is null")); return;}

    GEngine->GameViewport->GetViewportSize(ViewportSize);
    FVector2D ViewportCenter(ViewportSize.X / 2, ViewportSize.Y / 2);

    float SpreadScaled = CrosshairSpreadMax * HUDPackage.CrosshairSpread;

    if(!HUDPackage.CrosshairsCenter) return;
    Spread.X = 0; 
    Spread.Y = 0;
    DrawCrosshair(HUDPackage.CrosshairsCenter, ViewportCenter, Spread, HUDPackage.CrosshairsColor);

    if(!HUDPackage.CrosshairsLeft) return;
    Spread.X = -SpreadScaled - 5; Spread.Y = 0;
    DrawCrosshair(HUDPackage.CrosshairsLeft, ViewportCenter, Spread, HUDPackage.CrosshairsColor);

    if(!HUDPackage.CrosshairsRight) return;
    Spread.X = SpreadScaled + 5; Spread.Y = 0;
    DrawCrosshair(HUDPackage.CrosshairsRight, ViewportCenter, Spread, HUDPackage.CrosshairsColor);

    if(!HUDPackage.CrosshairsTop) return;
    Spread.X = 0; Spread.Y = -SpreadScaled - 5;
    DrawCrosshair(HUDPackage.CrosshairsTop, ViewportCenter, Spread, HUDPackage.CrosshairsColor);

    if(!HUDPackage.CrosshairsBottom) return;
    Spread.X = 0; Spread.Y = SpreadScaled + 5;
    DrawCrosshair(HUDPackage.CrosshairsBottom, ViewportCenter, Spread, HUDPackage.CrosshairsColor);

}

void ABlasterHUD::DrawCrosshair(UTexture2D* Texture, FVector2D ViewportCenter, FVector2D _Spread, FLinearColor CrosshairColor)
{
    const float TextureWidth = Texture->GetSizeX();
    const float TextureHight = Texture->GetSizeY();
    const FVector2D TextureDrawPoint(ViewportCenter.X - (TextureWidth / 2) + _Spread.X, ViewportCenter.Y - (TextureHight / 2) + _Spread.Y);

    DrawTexture(Texture, TextureDrawPoint.X, TextureDrawPoint.Y, TextureWidth, TextureHight, 0, 0, 1, 1, CrosshairColor);
}
