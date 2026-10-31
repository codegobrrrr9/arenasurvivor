#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "ArenaTypes.h"
#include "ArenaEnemy.generated.h"

class UCapsuleComponent;
class UStaticMeshComponent;
class UArenaHealthComponent;
class UArenaAssets;
class AArenaPlayerCharacter;

/**
 * One enemy class, three behaviours. Kinematic capsule moved with sweeps; contact
 * damage is distance based with a cooldown; enemies ignore each other in physics and
 * are separated by the wave spawner.
 */
UCLASS()
class ARENASURVIVOR_API AArenaEnemy : public APawn
{
	GENERATED_BODY()

public:
	AArenaEnemy();

	void Init(const FEnemyDef* InDef, float HpMultiplier, float Scale, UArenaAssets* Assets);
	virtual void Tick(float DeltaSeconds) override;

	float Radius() const;
	float Mass() const { return Def ? Def->Mass : 1.f; }
	bool IsDead() const { return bDead; }

	const FEnemyDef* Def = nullptr;

	UPROPERTY() TObjectPtr<UCapsuleComponent> Capsule;
	UPROPERTY() TObjectPtr<UStaticMeshComponent> Mesh;
	UPROPERTY() TObjectPtr<UArenaHealthComponent> Health;

private:
	FVector DasherVelocity(const FVector& Dir, float Dist, float DeltaSeconds);
	void TryContactDamage(AArenaPlayerCharacter* Player, float Now);
	void HandleDamaged(float Amount);
	void HandleDied();

	FVector Knockback = FVector::ZeroVector;
	FVector DashDir = FVector::ForwardVector;
	float NextContactTime = 0.f;
	float DashTimer = 0.f;
	float ArenaRadius = 2000.f;
	bool bDashing = false;
	bool bDead = false;
	bool bBound = false;
	int32 OrbitSign = 1;
};
