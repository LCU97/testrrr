/*
 * H3 탈출 성공 안내 UI.
 * 별도 위젯 블루프린트를 만들지 않아도 동작하며, WBP_EscapeSuccess는 이 클래스를
 * 상속해 프로젝트의 최종 연출로 교체할 수 있다.
 */

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "EscapeSuccessWidget.generated.h"

UCLASS(Blueprintable)
class TESTRRR_API UEscapeSuccessWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;
};
