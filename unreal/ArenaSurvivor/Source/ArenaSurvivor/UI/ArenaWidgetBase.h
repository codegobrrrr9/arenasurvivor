#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ArenaTypes.h"
#include "ArenaWidgetBase.generated.h"

class UCanvasPanel;
class UTextBlock;
class UButton;
class UProgressBar;
class AArenaGameMode;
class UWidget;

/**
 * Base for state-driven panels built entirely in C++ (no Blueprint assets). The
 * widget tree is constructed in RebuildWidget before Slate builds it; panels show
 * themselves only in the game states they list.
 */
UCLASS()
class ARENASURVIVOR_API UArenaWidgetBase : public UUserWidget
{
	GENERATED_BODY()

public:
	UArenaWidgetBase(const FObjectInitializer& ObjectInitializer);

	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	static const FLinearColor PanelColor;
	static const FLinearColor Accent;
	static const FLinearColor TextColor;
	static const FLinearColor Muted;

protected:
	/** Creates the child hierarchy under Root. */
	virtual void BuildTree() {}
	virtual void OnShown() {}
	virtual void OnStateChanged(EArenaState NewState);

	AArenaGameMode* Gm() const;
	void Click();

	/** Positions a widget inside Canvas using Unity-style anchors (y up) converted to UMG (y down). */
	void Place(UWidget* Widget, UCanvasPanel* Canvas, FVector2D UnityAnchor, FVector2D UnityPosition, FVector2D Size);
	void Stretch(UWidget* Widget, UCanvasPanel* Canvas);

	UTextBlock* MakeLabel(const FString& Text, int32 Size, bool bBold = false, ETextJustify::Type Justify = ETextJustify::Center);
	UTextBlock* MakeText(UCanvasPanel* Canvas, const FString& Text, int32 Size, FVector2D UnityAnchor, FVector2D UnityPosition, FVector2D Box,
		ETextJustify::Type Justify = ETextJustify::Center, bool bBold = false);
	UButton* MakeButton(UCanvasPanel* Canvas, const FString& Label, FVector2D UnityAnchor, FVector2D UnityPosition, FVector2D Size, int32 FontSize = 30);
	UProgressBar* MakeBar(UCanvasPanel* Canvas, FVector2D UnityAnchor, FVector2D UnityPosition, FVector2D Size, FLinearColor Fill);
	/** Full-screen tinted backdrop; returns an inner canvas to place children in. */
	UCanvasPanel* MakePanel(FLinearColor Color);

	TArray<EArenaState> VisibleIn;
	ESlateVisibility ShownVisibility = ESlateVisibility::Visible;

	UPROPERTY() TObjectPtr<UCanvasPanel> Root;

private:
	FDelegateHandle StateHandle;
};
