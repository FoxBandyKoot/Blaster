// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "BlasterPlayerState.generated.h"

/**
 * 
 */
UCLASS()
class BLASTER_API ABlasterPlayerState : public APlayerState
{
	GENERATED_BODY()

public:

	/**
	 * Replication notifies
	 */
	virtual void GetLifetimeReplicatedProps(TArray< FLifetimeProperty > & OutLifetimeProps) const override;
	virtual void OnRep_Score() override; // Is already a UFUNCTION in her parent class

	UFUNCTION()
	virtual void OnRep_Deaths();
	
	void AddToScore(float ScoreAmount);
	void AddToDeaths(int32 DeathsAmount);

protected:


private:
	UPROPERTY();
	class ABlasterCharacter* Character;

	UPROPERTY();
	class ABlasterPlayerController* Controller;

	UPROPERTY(ReplicatedUsing = OnRep_Deaths)
	int32 Deaths;
};
