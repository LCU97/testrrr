/*
 * 들고 있는 아이템의 사용 계약
 * 플레이어는 아이템의 구체적인 종류 대신 사용 동작과 사용 가능 조건을 조회한다.
 * 던지기 성공 후의 고유 반응은 OnItemThrown에서 아이템 자신이 구현한다.
 */

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "ItemUseTypes.h"
#include "UsableItem.generated.h"

class UItemUseBehavior;

UINTERFACE(BlueprintType)
class TESTRRR_API UUsableItem : public UInterface
{
	GENERATED_BODY()
};

class TESTRRR_API IUsableItem
{
	GENERATED_BODY()

public:
	// 조회 함수에서는 효과를 실행하지 않는다. nullptr이면 사용 기능이 없는 물건이다.
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Item Use")
	UItemUseBehavior* GetItemUseBehavior() const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Item Use")
	bool CanUseItem(const FItemUseContext& Context) const;

	// 일반 G 내려놓기에는 호출하지 않는다. 폭탄의 타이머 등은 이 알림에서 시작한다.
	UFUNCTION(BlueprintNativeEvent, Category="Item Use")
	void OnItemThrown(const FItemThrowInfo& Info);
};
