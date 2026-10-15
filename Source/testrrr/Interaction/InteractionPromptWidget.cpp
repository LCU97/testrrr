/*
 * 상호작용 안내 UI 구현
 * 화면 중앙보다 약간 위에 문구를 배치하고, 사용 가능 여부에 따라 색상을 바꾼다.
 * 빈 문구는 숨기며 위젯이 게임 입력을 가로채지 않도록 처리한다.
 */

#include "Interaction/InteractionPromptWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"
#include "Styling/CoreStyle.h"

void UInteractionPromptWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	// 자식 BP가 만든 위젯 트리는 덮어쓰지 않고, 없을 때만 기본 UI를 구성한다.
	if (!WidgetTree->RootWidget)
	{
		UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("Canvas"));
		WidgetTree->RootWidget = Canvas;
		PromptText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Prompt"));
		PromptText->SetJustification(ETextJustify::Center);
		PromptText->SetFont(FCoreStyle::GetDefaultFontStyle(TEXT("Regular"), 20));
		PromptText->SetShadowOffset(FVector2D(1.f, 1.f));
		PromptText->SetShadowColorAndOpacity(FLinearColor::Black);
		UCanvasPanelSlot* PromptSlot = Canvas->AddChildToCanvas(PromptText);
		// 가로 중앙, 세로 43%에 배치하여 화면 중앙보다 살짝 위에 표시한다.
		PromptSlot->SetAnchors(FAnchors(0.5f, 0.43f));
		PromptSlot->SetAlignment(FVector2D(0.5f, 0.5f));
		PromptSlot->SetAutoSize(true);
	}
	SetVisibility(ESlateVisibility::Collapsed);
}

void UInteractionPromptWidget::SetPrompt_Implementation(const FText& Prompt, bool bAvailable)
{
	if (PromptText)
	{
		PromptText->SetText(Prompt);
		PromptText->SetColorAndOpacity(FSlateColor(bAvailable ? FLinearColor::White : FLinearColor(0.6f, 0.6f, 0.6f)));
	}
	// 문구가 없으면 숨기며, 표시 중에도 마우스 입력을 가로채지 않는다.
	SetVisibility(Prompt.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
}
