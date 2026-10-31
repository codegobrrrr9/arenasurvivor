#pragma once

#include "CoreMinimal.h"
#include "UI/ArenaWidgetBase.h"
#include "ArenaMainMenuWidget.generated.h"

class UButton;
class UTextBlock;

UCLASS()
class ARENASURVIVOR_API UArenaMainMenuWidget : public UArenaWidgetBase
{
	GENERATED_BODY()

public:
	UArenaMainMenuWidget(const FObjectInitializer& ObjectInitializer);

protected:
	virtual void BuildTree() override;
	virtual void OnShown() override;

private:
	UFUNCTION() void HandlePlay();
	UFUNCTION() void HandleQuit();

	UPROPERTY() TObjectPtr<UButton> PlayButton;
	UPROPERTY() TObjectPtr<UButton> QuitButton;
	UPROPERTY() TObjectPtr<UTextBlock> HighScoreText;
};
