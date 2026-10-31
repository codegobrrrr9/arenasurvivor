#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "ArenaTypes.h"
#include "ArenaWaveSpawner.generated.h"

class AArenaGameMode;
class AArenaEnemy;

/**
 * Formula-driven waves, ticked by the game mode. More enemies, more HP and faster
 * spawning as waves go up; dashers from wave 3, tanks from wave 5, a boss tank every 5th.
 */
UCLASS()
class ARENASURVIVOR_API UArenaWaveSpawner : public UObject
{
	GENERATED_BODY()

public:
	void Init(AArenaGameMode* InOwner);
	void StartWave(int32 InWave);
	void StopWaves();
	void Tick(float DeltaSeconds);
	void OnEnemyRemoved(AArenaEnemy* Enemy);

	int32 Alive() const { return Enemies.Num(); }
	int32 Remaining() const { return Queue.Num(); }

	bool bWaveInProgress = false;
	float SpawnRadiusPadding = 150.f;

	UPROPERTY() TArray<TObjectPtr<AArenaEnemy>> Enemies;

private:
	struct FQueued
	{
		const FEnemyDef* Def = nullptr;
		float Scale = 1.f;
		float HpMul = 1.f;
	};

	void Spawn(const FQueued& Entry);
	void CheckCleared();
	void SeparateEnemies(float DeltaSeconds);

	UPROPERTY() TObjectPtr<AArenaGameMode> Owner;

	TArray<FQueued> Queue;
	float SpawnInterval = 1.f;
	float NextSpawn = 0.f;
	int32 Wave = 0;
};
