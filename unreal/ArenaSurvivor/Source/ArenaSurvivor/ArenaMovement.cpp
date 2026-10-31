#include "ArenaMovement.h"
#include "GameFramework/Actor.h"

FHitResult ArenaMovement::MoveFlat(AActor* Actor, const FVector& InDelta)
{
	FHitResult Hit;
	if (Actor == nullptr) return Hit;
	FVector Delta = InDelta;
	Delta.Z = 0.f;
	if (Delta.IsNearlyZero()) return Hit;

	Actor->AddActorWorldOffset(Delta, true, &Hit);
	if (Hit.IsValidBlockingHit())
	{
		// Slide the remaining distance along the surface once.
		FVector Remaining = Delta * (1.f - Hit.Time);
		FVector Slide = FVector::VectorPlaneProject(Remaining, Hit.Normal);
		Slide.Z = 0.f;
		if (!Slide.IsNearlyZero())
		{
			FHitResult SlideHit;
			Actor->AddActorWorldOffset(Slide, true, &SlideHit);
		}
	}
	return Hit;
}

FVector ArenaMovement::ClampToArena(const FVector& Position, float Radius)
{
	FVector Flat(Position.X, Position.Y, 0.f);
	if (Flat.Size() > Radius)
	{
		Flat = Flat.GetSafeNormal() * Radius;
	}
	return FVector(Flat.X, Flat.Y, Position.Z);
}
