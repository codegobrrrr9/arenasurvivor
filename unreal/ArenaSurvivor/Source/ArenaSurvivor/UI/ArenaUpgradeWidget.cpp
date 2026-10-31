#include "UI/ArenaUpgradeWidget.h"
#include "ArenaAssets.h"
#include "ArenaGameMode.h"
#include "ArenaUpgradeSystem.h"
#include "ArenaWaveSpawner.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"

UArenaUpgradeWidget::UArenaUpgradeWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	VisibleIn = { EArenaState::Upgrading };
}

void UArenaUpgradeWidget::BuildTree()
{
	UCanvasPanel* Panel = MakePanel(PanelColor);
	Header = MakeText(Panel, TEXT("CHOOSE AN UPGRADE"), 64, FVector2D(0.5f, 0.5f), FVector2D(0.f, 230.f), FVector2D(1200.f, 90.f), ETextJustify::Center, true);

	Cards.Reset();
	CardTitles.Reset();
	CardDescriptions.Reset();
	const float Spacing = 420.f;
	for (int32 i = 0; i < 3; i++)
	{
		const float X = (i - 1) * Spacing;
		UButton* Card = MakeButton(Panel, TEXT(""), FVector2D(0.5f, 0.5f), FVector2D(X, -20.f), FVector2D(380.f, 300.f));

		UVerticalBox* Box = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
		Card->AddChild(Box);

		UTextBlock* Title = MakeLabel(TEXT("Upgrade"), 36, true);
		if (UVerticalBoxSlot* TitleSlot = Box->AddChildToVerticalBox(Title))
		{
			TitleSlot->SetHorizontalAlignment(HAlign_Fill);
			TitleSlot->SetPadding(FMargin(12.f, 24.f, 12.f, 8.f));
		}
		UTextBlock* Description = MakeLabel(TEXT(""), 24);
		if (UVerticalBoxSlot* DescSlot = Box->AddChildToVerticalBox(Description))
		{
			DescSlot->SetHorizontalAlignment(HAlign_Fill);
			DescSlot->SetPadding(FMargin(16.f, 8.f, 16.f, 8.f));
		}

		Cards.Add(Card);
		CardTitles.Add(Title);
		CardDescriptions.Add(Description);
	}
	Cards[0]->OnClicked.AddDynamic(this, &UArenaUpgradeWidget::HandleCard0);
	Cards[1]->OnClicked.AddDynamic(this, &UArenaUpgradeWidget::HandleCard1);
	Cards[2]->OnClicked.AddDynamic(this, &UArenaUpgradeWidget::HandleCard2);

	UTextBlock* Hint = MakeText(Panel, TEXT("Press 1, 2 or 3"), 24, FVector2D(0.5f, 0.5f), FVector2D(0.f, -230.f), FVector2D(600.f, 30.f));
	Hint->SetColorAndOpacity(FSlateColor(Muted));
}

void UArenaUpgradeWidget::OnShown()
{
	AArenaGameMode* Mode = Gm();
	if (Mode == nullptr || Mode->Upgrades == nullptr || Mode->Assets == nullptr) return;

	if (Mode->Upgrades->CurrentOffer.Num() == 0) Mode->Upgrades->Roll(3);
	const TArray<int32>& Offer = Mode->Upgrades->CurrentOffer;
	const TArray<FUpgradeDef>& All = Mode->Assets->Upgrades;

	for (int32 i = 0; i < Cards.Num(); i++)
	{
		const bool bHas = Offer.IsValidIndex(i) && All.IsValidIndex(Offer[i]);
		if (Cards[i] != nullptr) Cards[i]->SetVisibility(bHas ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
		if (!bHas) continue;
		const FUpgradeDef& Def = All[Offer[i]];
		const int32 Stacks = Mode->Upgrades->StacksOf(Offer[i]);
		if (CardTitles[i] != nullptr) CardTitles[i]->SetText(FText::FromString(Def.Title));
		if (CardDescriptions[i] != nullptr)
		{
			CardDescriptions[i]->SetText(FText::FromString(FString::Printf(TEXT("%s\n\n%d/%d taken"), *Def.Description, Stacks, Def.MaxStacks)));
		}
	}
	if (Header != nullptr)
	{
		const bool bMidWave = Mode->Spawner != nullptr && Mode->Spawner->bWaveInProgress;
		Header->SetText(FText::FromString(bMidWave ? FString(TEXT("LEVEL UP!")) : FString::Printf(TEXT("WAVE %d CLEARED"), Mode->Wave)));
	}
	if (Cards.Num() > 0 && Cards[0] != nullptr) Cards[0]->SetKeyboardFocus();
}

void UArenaUpgradeWidget::HandleCard0() { Pick(0); }
void UArenaUpgradeWidget::HandleCard1() { Pick(1); }
void UArenaUpgradeWidget::HandleCard2() { Pick(2); }

void UArenaUpgradeWidget::Pick(int32 Index)
{
	AArenaGameMode* Mode = Gm();
	if (Mode == nullptr || Mode->Upgrades == nullptr) return;
	Mode->Upgrades->Choose(Index);
}
