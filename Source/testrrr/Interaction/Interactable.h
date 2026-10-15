/*
 * 상호작용 인터페이스
 * 플레이어는 물건이나 스위치의 구체적인 종류 대신 이 공통 계약으로 대상을 호출한다.
 * 안내 조회·가능 여부 확인은 상태를 바꾸지 않고, 실제 동작은 Interact에서만 실행한다.
 */

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "ButtonInteractionTypes.h"
#include "Interactable.generated.h"

UINTERFACE(BlueprintType)
class TESTRRR_API UInteractable : public UInterface
{
	GENERATED_BODY()
};

// C++과 BP 구현 모두 Execute_ 함수를 통해 호출해야 BP 재정의까지 반영된다.
class TESTRRR_API IInteractable
{
	GENERATED_BODY()

public:
	// 안내 조회에서는 생성·소모·토글 같은 상태 변경을 하지 않는다.
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Interaction")
	FInteractionInfo GetInteractionInfo(const FInteractionContext& Context) const;

	// 잠김·쿨다운 등 현재 문맥에 따른 실행 조건만 판단한다.
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Interaction")
	bool CanInteract(const FInteractionContext& Context) const;

	// 실행 시점에도 조건을 확인하고 실제 처리 결과를 반환한다.
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Interaction")
	FInteractionResult Interact(const FInteractionContext& Context);
};
