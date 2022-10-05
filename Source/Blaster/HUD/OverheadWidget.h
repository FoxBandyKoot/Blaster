// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "OverheadWidget.generated.h"

/**
 * 
 */
UCLASS()
class BLASTER_API UOverheadWidget : public UUserWidget
{
	GENERATED_BODY()

public:

	// UPROPERTY(meta = (BindWidget));
	// class UTextBlock* DisplayLocalRole;
	// UPROPERTY(meta = (BindWidget));
	// class UTextBlock* DisplayRemoteRole;
	
	UPROPERTY(meta = (BindWidget));
	class UTextBlock* DisplayRole;

	// void SetDisplayText(FString TextToDisplay);
	
	UFUNCTION(BlueprintCallable)
	void ShowPlayerNetRole(APawn* InPawn);
	
protected:
	virtual void OnLevelRemovedFromWorld(ULevel* InLevel, UWorld* InWorld) override;

private:
	FString GetLocalRole(APawn* InPawn);
	FString GetRemoteRole(APawn* InPawn);
};
