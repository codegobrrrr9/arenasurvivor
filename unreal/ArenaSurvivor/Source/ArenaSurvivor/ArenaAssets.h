#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "ArenaTypes.h"
#include "ArenaAssets.generated.h"

class UStaticMesh;
class UStaticMeshComponent;
class UMaterialInterface;
class USoundBase;

/**
 * Registry of every asset the runtime needs, loaded by string path at BeginPlay
 * (the editor Python bootstrap creates them under /Game). Also holds the
 * code-defined enemy and upgrade tables.
 */
UCLASS()
class ARENASURVIVOR_API UArenaAssets : public UObject
{
	GENERATED_BODY()

public:
	void LoadAll();

	const FEnemyDef* FindEnemy(FName Id) const;
	UStaticMesh* EnemyMesh(FName Id) const;

	/** Assigns mesh + palette material + orientation fix to a component. ZOffset moves the mesh origin (feet) below the capsule centre. */
	void ApplyMesh(UStaticMeshComponent* Component, UStaticMesh* Mesh, float ZOffset) const;

	/** Fire-and-forget 2D sound with slight pitch jitter. */
	void Play(const UObject* WorldContext, USoundBase* Sound, float Volume = 1.f) const;

	UPROPERTY() TObjectPtr<UStaticMesh> PlayerMesh;
	UPROPERTY() TObjectPtr<UStaticMesh> BlobMesh;
	UPROPERTY() TObjectPtr<UStaticMesh> SpikyMesh;
	UPROPERTY() TObjectPtr<UStaticMesh> TankMesh;
	UPROPERTY() TObjectPtr<UStaticMesh> ProjectileMesh;
	UPROPERTY() TObjectPtr<UStaticMesh> XpMesh;
	UPROPERTY() TObjectPtr<UStaticMesh> HealthMesh;
	UPROPERTY() TObjectPtr<UStaticMesh> FloorMesh;
	UPROPERTY() TObjectPtr<UStaticMesh> PillarMesh;
	UPROPERTY() TObjectPtr<UStaticMesh> CrateMesh;

	UPROPERTY() TObjectPtr<UMaterialInterface> PaletteMaterial;

	UPROPERTY() TObjectPtr<USoundBase> Shoot;
	UPROPERTY() TObjectPtr<USoundBase> Hit;
	UPROPERTY() TObjectPtr<USoundBase> EnemyDie;
	UPROPERTY() TObjectPtr<USoundBase> Pickup;
	UPROPERTY() TObjectPtr<USoundBase> Upgrade;
	UPROPERTY() TObjectPtr<USoundBase> PlayerHurt;
	UPROPERTY() TObjectPtr<USoundBase> GameOver;
	UPROPERTY() TObjectPtr<USoundBase> UiClick;
	UPROPERTY() TObjectPtr<USoundBase> WaveStart;

	TArray<FEnemyDef> Enemies;
	TArray<FUpgradeDef> Upgrades;

	/** Yaw applied to every mesh so that the model's front faces +X. Detected from the player mesh bounds. */
	float MeshYaw = 0.f;
	float MasterVolume = 0.8f;
	float ArenaRadius = 2000.f;

private:
	void BuildTables();
	float DetectMeshYaw() const;

	template <typename T>
	T* LoadByPath(const TCHAR* Path) const;
};
