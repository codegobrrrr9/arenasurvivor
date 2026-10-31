#pragma once

#include "CoreMinimal.h"
#include "Engine/HitResult.h"

class AActor;

/** Kinematic movement helpers: swept offsets with a single slide, no simulated bodies. */
namespace ArenaMovement
{
	ARENASURVIVOR_API FHitResult MoveFlat(AActor* Actor, const FVector& Delta);
	ARENASURVIVOR_API FVector ClampToArena(const FVector& Position, float Radius);
}
