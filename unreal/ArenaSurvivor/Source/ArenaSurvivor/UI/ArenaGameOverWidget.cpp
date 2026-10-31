#include "UI/ArenaGameOverWidget.h"
#include "ArenaGameMode.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/TextBlock.h"

UArenaGameOverWidget::UArenaGameOverWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	VisibleIn = { EArenaState::GameOver };
}

void UArenaGameOverWidget::BuildTree()
{
	UCanvasPanel* Panel = MakePanel(FLinearColor(0.12f, 0.02f, 0.03f, 0.88f));
	MakeText(Panel, TEXT("YOU DIED"), 84, FVector2D(0.5f, 0.5f), FVector2D(0.f, 170.f), FVector2D(800.f, 100.f), ETextJustify::Center, true);
	SummaryText = MakeText(Panel, TEXT(""), 32, FVector2D(0.5f, 0.5f), FVector2D(0.f, 70.f), FVector2D(900.f, 80.f));
	RestartButton = MakeButton(Panel, TEXT("TRY AGAIN"), FVector2D(0.5f, 0.5f), FVector2D(0.f, -40.f), FVector2D(320.f, 70.f));
	MenuButton = MakeButton(Panel, TEXT("MAIN MENU"), FVector2D(0.5f, 0.5f), FVector2D(0.f, -130.f), FVector2D(320.f, 70.f));
	RestartButton->OnClicked.AddDynamic(this, &UArenaGameOverWidget::HandleRestart);
	MenuButton->OnClicked.AddDynamic(this, &UArenaGameOverWidget::HandleMenu);
}

void UArenaGameOverWidget::OnShown()
{
	AArenaGameMode* Mode = Gm();
	if (Mode == nullptr || SummaryText == nullptr) return;
	const FString Best = (Mode->Score >= Mode->HighScore && Mode->Score > 0)
		? FString(TEXT("  NEW HIGH SCORE!"))
		: FString::Printf(TEXT("  (best %d)"), Mode->HighScore);
	SummaryText->SetText(FText::FromString(FString::Printf(TEXT("Reached wave %d with a score of %d%s"), Mode->Wave, Mode->Score, *Best)));
	if (RestartButton != nullptr) RestartButton->SetKeyboardFocus();
}

void UArenaGameOverWidget::HandleRestart()
{
	Click();
	if (AArenaGameMode* Mode = Gm()) Mode->StartRun();
}

void UArenaGameOverWidget::HandleMenu()
{
	Click();
	if (AArenaGameMode* Mode = Gm()) Mode->ReturnToMenu();
}
