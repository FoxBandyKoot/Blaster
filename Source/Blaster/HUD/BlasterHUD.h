// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "BlasterHUD.generated.h"

USTRUCT(BlueprintType)
struct FHUDPackage
{
	GENERATED_BODY()

public:
	class UTexture2D* CrosshairsCenter;
	class UTexture2D* CrosshairsLeft;
	class UTexture2D* CrosshairsRight;
	class UTexture2D* CrosshairsTop;
	class UTexture2D* CrosshairsBottom;
	float CrosshairSpread;
	FLinearColor CrosshairsColor;
};

/**
 * 
 */
UCLASS()
class BLASTER_API ABlasterHUD : public AHUD
{
	GENERATED_BODY()

public:
	virtual void DrawHUD() override; // Called every frame
	void AddCharacterOverlay();
	void AddAnnouncementOverlay();

	FORCEINLINE void SetHUDPackage(const FHUDPackage& Package) {HUDPackage = Package;};

	UPROPERTY(EditAnywhere, Category = Custom)
	TSubclassOf<class UUserWidget> CharacterOverlayClass;

	UPROPERTY(EditAnywhere, Category = Custom)
	TSubclassOf<class UUserWidget> AnnouncementOverlayClass;

	UPROPERTY()
	class UCharacterOverlay* CharacterOverlay;

	UPROPERTY()
	class UAnnouncementOverlay* AnnouncementOverlay;

protected:

	virtual void BeginPlay() override;

private:

	void DrawCrosshair(UTexture2D* Texture, FVector2D ViewportCenter, FVector2D _Spread, FLinearColor CrosshairColor);
	
	FVector2D Spread;
	FHUDPackage HUDPackage;

	UPROPERTY(EditAnywhere)
	float CrosshairSpreadMax = 24;
};
