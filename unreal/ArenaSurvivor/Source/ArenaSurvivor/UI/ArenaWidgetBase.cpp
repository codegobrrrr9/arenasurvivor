#include "UI/ArenaWidgetBase.h"
#include "ArenaAssets.h"
#include "ArenaGameMode.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"

const FLinearColor UArenaWidgetBase::PanelColor(0.05f, 0.06f, 0.09f, 0.88f);
const FLinearColor UArenaWidgetBase::Accent(0.2f, 0.47f, 0.96f, 1.f);
const FLinearColor UArenaWidgetBase::TextColor(0.95f, 0.95f, 0.97f, 1.f);
const FLinearColor UArenaWidgetBase::Muted(0.7f, 0.72f, 0.78f, 1.f);

UArenaWidgetBase::UArenaWidgetBase(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

TSharedRef<SWidget> UArenaWidgetBase::RebuildWidget()
{
	if (WidgetTree != nullptr && Root == nullptr)
	{
		Root = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("Root"));
		WidgetTree->RootWidget = Root;
		BuildTree();
	}
	return Super::RebuildWidget();
}

void UArenaWidgetBase::NativeConstruct()
{
	Super::NativeConstruct();
	if (AArenaGameMode* Mode = Gm())
	{
		StateHandle = Mode->OnStateChanged.AddUObject(this, &UArenaWidgetBase::OnStateChanged);
		OnStateChanged(Mode->State);
	}
}

void UArenaWidgetBase::NativeDestruct()
{
	if (AArenaGameMode* Mode = Gm())
	{
		Mode->OnStateChanged.Remove(StateHandle);
	}
	Super::NativeDestruct();
}

void UArenaWidgetBase::OnStateChanged(EArenaState NewState)
{
	const bool bShow = VisibleIn.Contains(NewState);
	SetVisibility(bShow ? ShownVisibility : ESlateVisibility::Collapsed);
	if (bShow) OnShown();
}

AArenaGameMode* UArenaWidgetBase::Gm() const
{
	return AArenaGameMode::Get(this);
}

void UArenaWidgetBase::Click()
{
	if (AArenaGameMode* Mode = Gm())
	{
		Mode->PlaySfx(Mode->Assets ? Mode->Assets->UiClick : nullptr, 0.6f);
	}
}

void UArenaWidgetBase::Place(UWidget* Widget, UCanvasPanel* Canvas, FVector2D UnityAnchor, FVector2D UnityPosition, FVector2D Size)
{
	if (Widget == nullptr || Canvas == nullptr) return;
	UCanvasPanelSlot* CanvasSlot = Canvas->AddChildToCanvas(Widget);
	if (CanvasSlot == nullptr) return;
	const FVector2D Anchor(UnityAnchor.X, 1.f - UnityAnchor.Y);
	CanvasSlot->SetAnchors(FAnchors(Anchor.X, Anchor.Y));
	CanvasSlot->SetAlignment(Anchor);
	CanvasSlot->SetAutoSize(false);
	CanvasSlot->SetPosition(FVector2D(UnityPosition.X, -UnityPosition.Y));
	CanvasSlot->SetSize(Size);
}

void UArenaWidgetBase::Stretch(UWidget* Widget, UCanvasPanel* Canvas)
{
	if (Widget == nullptr || Canvas == nullptr) return;
	UCanvasPanelSlot* CanvasSlot = Canvas->AddChildToCanvas(Widget);
	if (CanvasSlot == nullptr) return;
	CanvasSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
	CanvasSlot->SetOffsets(FMargin(0.f));
}

UTextBlock* UArenaWidgetBase::MakeLabel(const FString& Text, int32 Size, bool bBold, ETextJustify::Type Justify)
{
	UTextBlock* Block = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	Block->SetText(FText::FromString(Text));
	FSlateFontInfo Font = Block->GetFont();
	Font.Size = Size;
	if (bBold) Font.TypefaceFontName = FName(TEXT("Bold"));
	Block->SetFont(Font);
	Block->SetColorAndOpacity(FSlateColor(TextColor));
	Block->SetJustification(Justify);
	Block->SetShadowOffset(FVector2D(1.5f, 1.5f));
	Block->SetShadowColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, 0.6f));
	Block->SetAutoWrapText(true);
	Block->SetVisibility(ESlateVisibility::HitTestInvisible);
	return Block;
}

UTextBlock* UArenaWidgetBase::MakeText(UCanvasPanel* Canvas, const FString& Text, int32 Size, FVector2D UnityAnchor, FVector2D UnityPosition, FVector2D Box,
	ETextJustify::Type Justify, bool bBold)
{
	UTextBlock* Block = MakeLabel(Text, Size, bBold, Justify);
	Place(Block, Canvas, UnityAnchor, UnityPosition, Box);
	return Block;
}

UButton* UArenaWidgetBase::MakeButton(UCanvasPanel* Canvas, const FString& Label, FVector2D UnityAnchor, FVector2D UnityPosition, FVector2D Size, int32 FontSize)
{
	UButton* Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
	Button->SetBackgroundColor(Accent);
	if (!Label.IsEmpty())
	{
		UTextBlock* Text = MakeLabel(Label, FontSize, true);
		Button->AddChild(Text);
	}
	Place(Button, Canvas, UnityAnchor, UnityPosition, Size);
	return Button;
}

UProgressBar* UArenaWidgetBase::MakeBar(UCanvasPanel* Canvas, FVector2D UnityAnchor, FVector2D UnityPosition, FVector2D Size, FLinearColor Fill)
{
	UProgressBar* Bar = WidgetTree->ConstructWidget<UProgressBar>(UProgressBar::StaticClass());
	Bar->SetFillColorAndOpacity(Fill);
	Bar->SetPercent(1.f);
	Bar->SetVisibility(ESlateVisibility::HitTestInvisible);
	Place(Bar, Canvas, UnityAnchor, UnityPosition, Size);
	return Bar;
}

UCanvasPanel* UArenaWidgetBase::MakePanel(FLinearColor Color)
{
	UBorder* Border = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
	Border->SetBrushColor(Color);
	Border->SetPadding(FMargin(0.f));
	UCanvasPanel* Inner = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass());
	Border->SetContent(Inner);
	Stretch(Border, Root);
	return Inner;
}
