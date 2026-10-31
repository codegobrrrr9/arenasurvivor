#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "ArenaTypes.h"
#include "ArenaUpgradeSystem.generated.h"

class AArenaGameMode;

UCLASS()
class ARENASURVIVOR_API UArenaUpgradeSystem : public UObject
{
	GENERATED_BODY()

public:
	void Init(AArenaGameMode* InOwner);
	void ResetRun();
	int32 StacksOf(int32 DefIndex) const;

	/** Picks up to Count distinct upgrades that still have stacks left. Returns indices into Assets->Upgrades. */
	const TArray<int32>& Roll(int32 Count);
	void Choose(int32 OfferIndex);

	TArray<int32> CurrentOffer;

private:
	UPROPERTY() TObjectPtr<AArenaGameMode> Owner;
	TMap<int32, int32> Stacks;
};
