#pragma once

#include "CoreMinimal.h"
#include "UI/ArenaWidgetBase.h"
#include "ArenaPauseWidget.generated.h"

class UButton;

UCLASS()
class ARENASURVIVOR_API UArenaPauseWidget : public UArenaWidgetBase
{
	GENERATED_BODY()

public:
	UArenaPauseWidget(const FObjectInitializer& ObjectInitializer);

protected:
	virtual void BuildTree() override;
	virtual void OnShown() override;

private:
	UFUNCTION() void HandleResume();
	UFUNCTION() void HandleRestart();
	UFUNCTION() void HandleMenu();

	UPROPERTY() TObjectPtr<UButton> ResumeButton;
	UPROPERTY() TObjectPtr<UButton> RestartButton;
	UPROPERTY() TObjectPtr<UButton> MenuButton;
};
