#include "UI/ArenaPauseWidget.h"
#include "ArenaGameMode.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"

UArenaPauseWidget::UArenaPauseWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	VisibleIn = { EArenaState::Paused };
}

void UArenaPauseWidget::BuildTree()
{
	UCanvasPanel* Panel = MakePanel(PanelColor);
	MakeText(Panel, TEXT("PAUSED"), 72, FVector2D(0.5f, 0.5f), FVector2D(0.f, 150.f), FVector2D(800.f, 90.f), ETextJustify::Center, true);
	ResumeButton = MakeButton(Panel, TEXT("RESUME"), FVector2D(0.5f, 0.5f), FVector2D(0.f, 20.f), FVector2D(320.f, 70.f));
	RestartButton = MakeButton(Panel, TEXT("RESTART"), FVector2D(0.5f, 0.5f), FVector2D(0.f, -70.f), FVector2D(320.f, 70.f));
	MenuButton = MakeButton(Panel, TEXT("MAIN MENU"), FVector2D(0.5f, 0.5f), FVector2D(0.f, -160.f), FVector2D(320.f, 70.f));
	ResumeButton->OnClicked.AddDynamic(this, &UArenaPauseWidget::HandleResume);
	RestartButton->OnClicked.AddDynamic(this, &UArenaPauseWidget::HandleRestart);
	MenuButton->OnClicked.AddDynamic(this, &UArenaPauseWidget::HandleMenu);
}

void UArenaPauseWidget::OnShown()
{
	if (ResumeButton != nullptr) ResumeButton->SetKeyboardFocus();
}

void UArenaPauseWidget::HandleResume()
{
	Click();
	if (AArenaGameMode* Mode = Gm()) Mode->Resume();
}

void UArenaPauseWidget::HandleRestart()
{
	Click();
	if (AArenaGameMode* Mode = Gm()) Mode->StartRun();
}

void UArenaPauseWidget::HandleMenu()
{
	Click();
	if (AArenaGameMode* Mode = Gm()) Mode->ReturnToMenu();
}
