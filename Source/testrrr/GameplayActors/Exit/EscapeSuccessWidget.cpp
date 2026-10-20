#include "GameplayActors/Exit/EscapeSuccessWidget.h"

#include "Components/Border.h"
#include "Components/TextBlock.h"
#include "Blueprint/WidgetTree.h"
#include "Styling/CoreStyle.h"

void UEscapeSuccessWidget::NativeConstruct()
{
	Super::NativeConstruct();
	if (WidgetTree->RootWidget)
	{
		return;
	}

	UBorder* Background = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("SuccessBackground"));
	Background->SetBrushColor(FLinearColor(0.02f, 0.22f, 0.07f, 0.9f));
	Background->SetPadding(FMargin(32.f, 18.f));
	WidgetTree->RootWidget = Background;

	UTextBlock* Message = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("SuccessMessage"));
	Message->SetText(NSLOCTEXT("Escape", "Success", "탈출 성공!"));
	Message->SetJustification(ETextJustify::Center);
	Message->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", 32));
	Message->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	Background->SetContent(Message);
}
