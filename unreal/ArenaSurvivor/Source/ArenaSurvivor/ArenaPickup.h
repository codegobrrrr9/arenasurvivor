#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ArenaTypes.h"
#include "ArenaPickup.generated.h"

class UStaticMeshComponent;
class UArenaAssets;

/** Bobbing, spinning drop that flies to the player once they are close. No collision; distance based. */
UCLASS()
class ARENASURVIVOR_API AArenaPickup : public AActor
{
	GENERATED_BODY()

public:
	AArenaPickup();

	void Init(EPickupType InType, int32 InValue, UArenaAssets* Assets);
	virtual void Tick(float DeltaSeconds) override;

	float MagnetRadius = 350.f;
	float MagnetSpeed = 1200.f;
	float Lifetime = 20.f;

private:
	UPROPERTY() TObjectPtr<UStaticMeshComponent> Mesh;

	EPickupType Type = EPickupType::Xp;
	int32 Value = 1;
	float SpawnTime = 0.f;
	float BobOffset = 0.f;
	bool bCollected = false;
};
