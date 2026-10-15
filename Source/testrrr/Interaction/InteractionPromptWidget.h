/*
 * 기본 상호작용 안내 위젯
 * 별도 위젯 에셋 없이 안내를 표시하며, 자식 BP는 SetPrompt를 재정의해 UI를 교체할 수 있다.
 */

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "InteractionPromptWidget.generated.h"

class UTextBlock;

// 기본 C++ 안내 위젯. 자식 BP는 SetPrompt를 재정의하여 디자인을 교체할 수 있다.
UCLASS()
class TESTRRR_API UInteractionPromptWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// 빈 문구는 숨기고, bAvailable은 문구의 활성/비활성 표현에 사용한다.
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Interaction")
	void SetPrompt(const FText& Prompt, bool bAvailable);

protected:
	virtual void NativeOnInitialized() override;

private:
	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> PromptText;
};
