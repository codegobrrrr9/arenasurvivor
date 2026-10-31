#pragma once

#include "CoreMinimal.h"
#include "UI/ArenaWidgetBase.h"
#include "ArenaHUDWidget.generated.h"

class UTextBlock;
class UProgressBar;

UCLASS()
class ARENASURVIVOR_API UArenaHUDWidget : public UArenaWidgetBase
{
	GENERATED_BODY()

public:
	UArenaHUDWidget(const FObjectInitializer& ObjectInitializer);

	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

protected:
	virtual void BuildTree() override;

private:
	void Refresh();
	void HookPlayer();
	void OnHealthChanged(float Current, float Max);

	UPROPERTY() TObjectPtr<UProgressBar> HpBar;
	UPROPERTY() TObjectPtr<UProgressBar> XpBar;
	UPROPERTY() TObjectPtr<UTextBlock> HpText;
	UPROPERTY() TObjectPtr<UTextBlock> WaveText;
	UPROPERTY() TObjectPtr<UTextBlock> ScoreText;
	UPROPERTY() TObjectPtr<UTextBlock> EnemiesText;
	UPROPERTY() TObjectPtr<UTextBlock> LevelText;

	FDelegateHandle StatsHandle;
	FDelegateHandle HealthHandle;
	bool bPlayerHooked = false;
};
