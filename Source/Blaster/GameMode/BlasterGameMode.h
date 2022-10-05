// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "BlasterGameMode.generated.h"

namespace MatchState // It's an inheritance
{
	extern BLASTER_API const FName Cooldown; // Match duration has ben reached. Display winner and begin cooldown timer.
}

/**
 * Class only present on the server
 */
UCLASS()
class BLASTER_API ABlasterGameMode : public AGameMode
{
	GENERATED_BODY()
	
public:

	ABlasterGameMode();
	virtual void Tick(float DeltaTime) override;	
	virtual void PlayerEliminated(class ABlasterCharacter* EliminatedCharacter, class ABlasterPlayerController* VictimController, ABlasterPlayerController* AttackerController);
	virtual void RequestRespawn(ACharacter* EliminatedCharacter, AController* EliminatedController);

	FORCEINLINE float GetCountdownTime() const { return CountdownTime;}

	UPROPERTY(EditDefaultsOnly)
	float WarmupTime = 0.f;
	
	UPROPERTY(EditDefaultsOnly)
	float MatchTime = 0.f; // 8 minutes

	UPROPERTY(EditDefaultsOnly)
	float CooldownTime = 0.f;

	float LevelStartingTime = 0.f; // Used to start the start time only on the good map, and not from the Lobby

protected:

	virtual void BeginPlay() override;
	virtual void OnMatchStateSet() override;

private:

	float CountdownTime = 0.f;
};
