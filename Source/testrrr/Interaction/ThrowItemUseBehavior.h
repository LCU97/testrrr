/*
 * 아이템 사용 방식 중 하나인 던지기
 * 이 동작이 설정된 아이템만 좌클릭으로 던지기를 요청한다.
 * 물리 제약 해제와 질량 반영은 CarryComponent에 맡기고 고유 후속 효과는 아이템에 맡긴다.
 */

#pragma once

#include "CoreMinimal.h"
#include "ItemUseBehavior.h"
#include "ThrowItemUseBehavior.generated.h"

UCLASS(Blueprintable, EditInlineNew)
class TESTRRR_API UThrowItemUseBehavior : public UItemUseBehavior
{
	GENERATED_BODY()

public:
	UThrowItemUseBehavior();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Throw")
	FItemThrowSettings ThrowSettings;

	virtual bool CanUse_Implementation(const FItemUseContext& Context) const override;
	virtual FItemUseResult BeginUse_Implementation(const FItemUseContext& Context) override;
};
