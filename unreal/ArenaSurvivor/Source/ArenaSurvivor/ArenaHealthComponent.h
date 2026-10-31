#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ArenaHealthComponent.generated.h"

DECLARE_MULTICAST_DELEGATE_TwoParams(FOnArenaHealthChanged, float /*Current*/, float /*Max*/);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnArenaDamaged, float /*Amount*/);

UCLASS()
class ARENASURVIVOR_API UArenaHealthComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UArenaHealthComponent();

	void Configure(float NewMax, bool bRefill = true);
	void ApplyArenaDamage(float Amount, const FVector& HitDirection);
	void Heal(float Amount);

	bool IsDead() const { return Current <= 0.f; }
	float Fraction() const { return Max > 0.f ? FMath::Clamp(Current / Max, 0.f, 1.f) : 0.f; }

	float Max = 100.f;
	float Current = 100.f;
	float InvulnerabilityAfterHit = 0.f;

	FOnArenaHealthChanged OnChanged;
	FOnArenaDamaged OnDamaged;
	FSimpleMulticastDelegate OnDied;

private:
	float InvulnerableUntil = 0.f;
};
