#include "ArenaHealthComponent.h"
#include "Engine/World.h"

UArenaHealthComponent::UArenaHealthComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UArenaHealthComponent::Configure(float NewMax, bool bRefill)
{
	Max = NewMax;
	Current = bRefill ? Max : FMath::Min(Current, Max);
	OnChanged.Broadcast(Current, Max);
}

void UArenaHealthComponent::ApplyArenaDamage(float Amount, const FVector& HitDirection)
{
	if (IsDead() || Amount <= 0.f) return;
	const float Now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;
	if (Now < InvulnerableUntil) return;
	InvulnerableUntil = Now + InvulnerabilityAfterHit;

	Current = FMath::Max(0.f, Current - Amount);
	OnDamaged.Broadcast(Amount);
	OnChanged.Broadcast(Current, Max);
	if (Current <= 0.f)
	{
		OnDied.Broadcast();
	}
}

void UArenaHealthComponent::Heal(float Amount)
{
	if (Amount <= 0.f) return;
	Current = FMath::Min(Max, Current + Amount);
	OnChanged.Broadcast(Current, Max);
}
