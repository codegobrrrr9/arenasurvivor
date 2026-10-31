#pragma once

#include "CoreMinimal.h"
#include "UI/ArenaWidgetBase.h"
#include "ArenaUpgradeWidget.generated.h"

class UButton;
class UTextBlock;

UCLASS()
class ARENASURVIVOR_API UArenaUpgradeWidget : public UArenaWidgetBase
{
	GENERATED_BODY()

public:
	UArenaUpgradeWidget(const FObjectInitializer& ObjectInitializer);

protected:
	virtual void BuildTree() override;
	virtual void OnShown() override;

private:
	UFUNCTION() void HandleCard0();
	UFUNCTION() void HandleCard1();
	UFUNCTION() void HandleCard2();
	void Pick(int32 Index);

	UPROPERTY() TObjectPtr<UTextBlock> Header;
	UPROPERTY() TArray<TObjectPtr<UButton>> Cards;
	UPROPERTY() TArray<TObjectPtr<UTextBlock>> CardTitles;
	UPROPERTY() TArray<TObjectPtr<UTextBlock>> CardDescriptions;
};
