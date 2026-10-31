#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "ArenaTypes.h"
#include "ArenaGameMode.generated.h"

class UArenaAssets;
class AArenaPlayerCharacter;
class UArenaWaveSpawner;
class UArenaUpgradeSystem;
class UUserWidget;
class USoundBase;

DECLARE_MULTICAST_DELEGATE_OneParam(FOnArenaStateChanged, EArenaState);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnArenaWaveStarted, int32);

/**
 * Owns the game state machine, score, wave and XP bookkeeping, the wave spawner and
 * the upgrade system. Everything else talks to this; UI widgets listen to its events.
 */
UCLASS()
class ARENASURVIVOR_API AArenaGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AArenaGameMode();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	static AArenaGameMode* Get(const UObject* WorldContext);

	void StartRun();
	void ReturnToMenu();
	void TogglePause();
	void PauseGame();
	void Resume();

	void NextWave();
	void OnWaveCleared();
	void OnUpgradeChosen();
	void OnPlayerDied();
	void OnEnemyKilled(const FEnemyDef& Def, const FVector& Position);
	void OnPickupCollected(EPickupType Type, int32 Value);

	void PlaySfx(USoundBase* Sound, float Volume = 1.f) const;
	int32 XpToNext() const { return 10 + Level * 6; }

	FOnArenaStateChanged OnStateChanged;
	FSimpleMulticastDelegate OnStatsChanged;
	FOnArenaWaveStarted OnWaveStarted;

	EArenaState State = EArenaState::Menu;
	int32 Score = 0;
	int32 Wave = 0;
	int32 Level = 0;
	int32 Xp = 0;
	int32 HighScore = 0;
	float ArenaRadius = 2000.f;

	UPROPERTY() TObjectPtr<UArenaAssets> Assets;
	UPROPERTY() TObjectPtr<AArenaPlayerCharacter> Player;
	UPROPERTY() TObjectPtr<UArenaWaveSpawner> Spawner;
	UPROPERTY() TObjectPtr<UArenaUpgradeSystem> Upgrades;
	UPROPERTY() TArray<TObjectPtr<UUserWidget>> Widgets;

private:
	void SetState(EArenaState Next);
	void OpenUpgradeChoice();
	void EnsurePlayer();
	void EnsureArenaWorld();
	void CreateWidgets();
	void ClearActors();

	EArenaState StateBeforePause = EArenaState::Playing;
	bool bPendingLevelUpgrade = false;
	bool bPlayerBound = false;
};
