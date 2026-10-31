#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ArenaProjectile.generated.h"

class UStaticMeshComponent;
class UArenaAssets;

/**
 * Collider-free projectile: each tick it line-traces the distance it is about to
 * travel against enemies and props, so fast shots never tunnel.
 */
UCLASS()
class ARENASURVIVOR_API AArenaProjectile : public AActor
{
	GENERATED_BODY()

public:
	AArenaProjectile();

	void Launch(const FVector& InDirection, float InDamage, float InSpeed, float InArenaRadius, AActor* InShooter, UArenaAssets* Assets);

	virtual void Tick(float DeltaSeconds) override;

	float Lifetime = 2.5f;

private:
	UPROPERTY() TObjectPtr<UStaticMeshComponent> Mesh;
	UPROPERTY() TObjectPtr<AActor> Shooter;

	FVector Direction = FVector::ForwardVector;
	float Damage = 10.f;
	float Speed = 2400.f;
	float DieAt = 0.f;
	float ArenaRadius = 2000.f;
};
