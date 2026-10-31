#include "ArenaUpgradeSystem.h"
#include "ArenaAssets.h"
#include "ArenaGameMode.h"
#include "ArenaHealthComponent.h"
#include "ArenaPlayerCharacter.h"

void UArenaUpgradeSystem::Init(AArenaGameMode* InOwner)
{
	Owner = InOwner;
}

void UArenaUpgradeSystem::ResetRun()
{
	Stacks.Reset();
	CurrentOffer.Reset();
}

int32 UArenaUpgradeSystem::StacksOf(int32 DefIndex) const
{
	const int32* Found = Stacks.Find(DefIndex);
	return Found ? *Found : 0;
}

const TArray<int32>& UArenaUpgradeSystem::Roll(int32 Count)
{
	CurrentOffer.Reset();
	if (Owner == nullptr || Owner->Assets == nullptr) return CurrentOffer;

	TArray<int32> Pool;
	const TArray<FUpgradeDef>& All = Owner->Assets->Upgrades;
	for (int32 i = 0; i < All.Num(); i++)
	{
		if (StacksOf(i) < All[i].MaxStacks) Pool.Add(i);
	}
	for (int32 i = Pool.Num() - 1; i > 0; i--)
	{
		Pool.Swap(i, FMath::RandRange(0, i));
	}
	if (Pool.Num() > Count) Pool.SetNum(Count);
	CurrentOffer = Pool;
	return CurrentOffer;
}

void UArenaUpgradeSystem::Choose(int32 OfferIndex)
{
	if (Owner == nullptr || Owner->Assets == nullptr || Owner->Player == nullptr) return;
	if (!CurrentOffer.IsValidIndex(OfferIndex)) return;
	const int32 DefIndex = CurrentOffer[OfferIndex];
	if (!Owner->Assets->Upgrades.IsValidIndex(DefIndex)) return;
	const FUpgradeDef& Def = Owner->Assets->Upgrades[DefIndex];

	AArenaPlayerCharacter* Player = Owner->Player;
	const float OldMax = Player->Stats.MaxHp;
	Player->Stats.ApplyUpgrade(Def);
	Stacks.FindOrAdd(DefIndex) = StacksOf(DefIndex) + 1;

	if (Def.Stat == EArenaStat::MaxHp)
	{
		Player->Health->Configure(Player->Stats.MaxHp, false);
		Player->Health->Heal(Player->Stats.MaxHp - OldMax);
	}
	if (Def.bHealOnPick)
	{
		Player->Health->Heal(Player->Stats.MaxHp * 0.25f);
	}

	Owner->PlaySfx(Owner->Assets->UiClick, 0.6f);
	Owner->OnUpgradeChosen();
}
