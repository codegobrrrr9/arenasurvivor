#include "UI/ArenaMainMenuWidget.h"
#include "ArenaGameMode.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/TextBlock.h"
#include "Kismet/KismetSystemLibrary.h"

UArenaMainMenuWidget::UArenaMainMenuWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	VisibleIn = { EArenaState::Menu };
}

void UArenaMainMenuWidget::BuildTree()
{
	UCanvasPanel* Panel = MakePanel(FLinearColor(0.03f, 0.04f, 0.07f, 0.75f));

	MakeText(Panel, TEXT("ARENA SURVIVOR"), 96, FVector2D(0.5f, 0.5f), FVector2D(0.f, 190.f), FVector2D(1200.f, 120.f), ETextJustify::Center, true);
	UTextBlock* Subtitle = MakeText(Panel, TEXT("Survive the waves. Pick upgrades. Beat your high score."), 30, FVector2D(0.5f, 0.5f), FVector2D(0.f, 110.f), FVector2D(1200.f, 40.f));
	Subtitle->SetColorAndOpacity(FSlateColor(Muted));

	PlayButton = MakeButton(Panel, TEXT("PLAY"), FVector2D(0.5f, 0.5f), FVector2D(0.f, 0.f), FVector2D(320.f, 80.f), 38);
	PlayButton->OnClicked.AddDynamic(this, &UArenaMainMenuWidget::HandlePlay);
	QuitButton = MakeButton(Panel, TEXT("QUIT"), FVector2D(0.5f, 0.5f), FVector2D(0.f, -100.f), FVector2D(320.f, 64.f), 28);
	QuitButton->OnClicked.AddDynamic(this, &UArenaMainMenuWidget::HandleQuit);

	HighScoreText = MakeText(Panel, TEXT("High score: 0"), 28, FVector2D(0.5f, 0.5f), FVector2D(0.f, -190.f), FVector2D(600.f, 40.f));
	UTextBlock* Controls = MakeText(Panel,
		TEXT("WASD / Left stick: move     Mouse / Right stick: aim     Click / RT / Space: fire     Esc / Start: pause     Enter: play"),
		22, FVector2D(0.5f, 0.f), FVector2D(0.f, 40.f), FVector2D(1700.f, 30.f));
	Controls->SetColorAndOpacity(FSlateColor(Muted));
}

void UArenaMainMenuWidget::OnShown()
{
	if (AArenaGameMode* Mode = Gm())
	{
		if (HighScoreText != nullptr) HighScoreText->SetText(FText::FromString(FString::Printf(TEXT("High score: %d"), Mode->HighScore)));
	}
	if (PlayButton != nullptr) PlayButton->SetKeyboardFocus();
}

void UArenaMainMenuWidget::HandlePlay()
{
	Click();
	if (AArenaGameMode* Mode = Gm()) Mode->StartRun();
}

void UArenaMainMenuWidget::HandleQuit()
{
	UKismetSystemLibrary::QuitGame(this, GetOwningPlayer(), EQuitPreference::Quit, false);
}
