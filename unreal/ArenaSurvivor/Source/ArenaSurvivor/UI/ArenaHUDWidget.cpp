#include "UI/ArenaHUDWidget.h"
#include "ArenaGameMode.h"
#include "ArenaHealthComponent.h"
#include "ArenaPlayerCharacter.h"
#include "ArenaWaveSpawner.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"

UArenaHUDWidget::UArenaHUDWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	VisibleIn = { EArenaState::Playing, EArenaState::Paused, EArenaState::Upgrading };
	ShownVisibility = ESlateVisibility::HitTestInvisible;
}

void UArenaHUDWidget::BuildTree()
{
	HpBar = MakeBar(Root, FVector2D(0.f, 1.f), FVector2D(30.f, -30.f), FVector2D(420.f, 34.f), FLinearColor(0.85f, 0.2f, 0.25f));
	HpText = MakeText(Root, TEXT("100 / 100"), 22, FVector2D(0.f, 1.f), FVector2D(30.f, -30.f), FVector2D(420.f, 34.f), ETextJustify::Center, true);
	XpBar = MakeBar(Root, FVector2D(0.f, 1.f), FVector2D(30.f, -72.f), FVector2D(420.f, 18.f), FLinearColor(0.3f, 0.85f, 0.95f));
	LevelText = MakeText(Root, TEXT("Lv 1"), 20, FVector2D(0.f, 1.f), FVector2D(460.f, -72.f), FVector2D(120.f, 24.f), ETextJustify::Left);
	WaveText = MakeText(Root, TEXT("Wave 1"), 40, FVector2D(0.5f, 1.f), FVector2D(0.f, -28.f), FVector2D(400.f, 50.f), ETextJustify::Center, true);
	EnemiesText = MakeText(Root, TEXT(""), 22, FVector2D(0.5f, 1.f), FVector2D(0.f, -74.f), FVector2D(400.f, 30.f));
	EnemiesText->SetColorAndOpacity(FSlateColor(Muted));
	ScoreText = MakeText(Root, TEXT("Score 0"), 34, FVector2D(1.f, 1.f), FVector2D(-30.f, -30.f), FVector2D(420.f, 44.f), ETextJustify::Right, true);
}

void UArenaHUDWidget::NativeConstruct()
{
	Super::NativeConstruct();
	if (AArenaGameMode* Mode = Gm())
	{
		StatsHandle = Mode->OnStatsChanged.AddUObject(this, &UArenaHUDWidget::Refresh);
	}
	HookPlayer();
	Refresh();
}

void UArenaHUDWidget::NativeDestruct()
{
	if (AArenaGameMode* Mode = Gm())
	{
		Mode->OnStatsChanged.Remove(StatsHandle);
		if (Mode->Player != nullptr && Mode->Player->Health != nullptr)
		{
			Mode->Player->Health->OnChanged.Remove(HealthHandle);
		}
	}
	Super::NativeDestruct();
}

void UArenaHUDWidget::HookPlayer()
{
	AArenaGameMode* Mode = Gm();
	if (bPlayerHooked || Mode == nullptr || Mode->Player == nullptr || Mode->Player->Health == nullptr) return;
	bPlayerHooked = true;
	HealthHandle = Mode->Player->Health->OnChanged.AddUObject(this, &UArenaHUDWidget::OnHealthChanged);
	OnHealthChanged(Mode->Player->Health->Current, Mode->Player->Health->Max);
}

void UArenaHUDWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	AArenaGameMode* Mode = Gm();
	if (Mode == nullptr) return;
	if (!bPlayerHooked) HookPlayer();
	if (Mode->Spawner != nullptr && EnemiesText != nullptr)
	{
		const int32 Left = Mode->Spawner->Alive() + Mode->Spawner->Remaining();
		EnemiesText->SetText(Left > 0 ? FText::FromString(FString::Printf(TEXT("%d enemies left"), Left)) : FText::GetEmpty());
	}
}

void UArenaHUDWidget::OnHealthChanged(float Current, float Max)
{
	if (HpBar != nullptr) HpBar->SetPercent(Max > 0.f ? Current / Max : 0.f);
	if (HpText != nullptr) HpText->SetText(FText::FromString(FString::Printf(TEXT("%d / %d"), FMath::CeilToInt(Current), FMath::CeilToInt(Max))));
}

void UArenaHUDWidget::Refresh()
{
	AArenaGameMode* Mode = Gm();
	if (Mode == nullptr) return;
	if (WaveText != nullptr) WaveText->SetText(FText::FromString(FString::Printf(TEXT("Wave %d"), FMath::Max(1, Mode->Wave))));
	if (ScoreText != nullptr) ScoreText->SetText(FText::FromString(FString::Printf(TEXT("Score %d"), Mode->Score)));
	if (LevelText != nullptr) LevelText->SetText(FText::FromString(FString::Printf(TEXT("Lv %d"), Mode->Level + 1)));
	if (XpBar != nullptr) XpBar->SetPercent(Mode->XpToNext() > 0 ? (float)Mode->Xp / (float)Mode->XpToNext() : 0.f);
	if (Mode->Player != nullptr && Mode->Player->Health != nullptr)
	{
		OnHealthChanged(Mode->Player->Health->Current, Mode->Player->Health->Max);
	}
}
