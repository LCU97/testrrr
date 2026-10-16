/*
 * 아이템별 사용 동작의 전략 객체
 * 아이템 인스턴스마다 소유하여 서로 다른 물건의 사용 상태가 섞이지 않게 한다.
 * C++ 또는 BP 자식은 사용 조건과 시작·유지·종료·취소를 구현한다.
 */

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "ItemUseTypes.h"
#include "ItemUseBehavior.generated.h"

UCLASS(Abstract, Blueprintable, EditInlineNew, DefaultToInstanced)
class TESTRRR_API UItemUseBehavior : public UObject
{
	GENERATED_BODY()

public:
	// 소유 아이템의 월드를 반환하여 BP 동작에서도 타이머 등 월드 기능을 사용할 수 있게 한다.
	virtual UWorld* GetWorld() const override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Item Use")
	FText UsePrompt;

	// 조회용 조건이다. 탄약·연료 등을 여기서 소모하지 않는다.
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Item Use")
	bool CanUse(const FItemUseContext& Context) const;

	// 단발 동작은 성공 후 bKeepUsing=false, 분사 같은 지속 동작은 true를 반환한다.
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Item Use")
	FItemUseResult BeginUse(const FItemUseContext& Context);

	UFUNCTION(BlueprintNativeEvent, Category="Item Use")
	void UpdateUse(const FItemUseContext& Context, float DeltaTime);

	// EndUse는 정상적인 입력 종료, CancelUse는 운반 변경·파괴 등으로 인한 중단이다.
	UFUNCTION(BlueprintNativeEvent, Category="Item Use")
	void EndUse(const FItemUseContext& Context);

	UFUNCTION(BlueprintNativeEvent, Category="Item Use")
	void CancelUse(const FItemUseContext& Context);
};
