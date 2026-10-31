#pragma once

#include "CoreMinimal.h"
#include "UI/ArenaWidgetBase.h"
#include "ArenaGameOverWidget.generated.h"

class UButton;
class UTextBlock;

UCLASS()
class ARENASURVIVOR_API UArenaGameOverWidget : public UArenaWidgetBase
{
	GENERATED_BODY()

public:
	UArenaGameOverWidget(const FObjectInitializer& ObjectInitializer);

protected:
	virtual void BuildTree() override;
	virtual void OnShown() override;

private:
	UFUNCTION() void HandleRestart();
	UFUNCTION() void HandleMenu();

	UPROPERTY() TObjectPtr<UTextBlock> SummaryText;
	UPROPERTY() TObjectPtr<UButton> RestartButton;
	UPROPERTY() TObjectPtr<UButton> MenuButton;
};
