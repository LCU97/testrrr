/*
 * 플레이어의 아이템 사용 입력과 사용 세션 관리
 * 클릭 시작 시 선택한 아이템과 동작을 고정하고, 지속 사용 중에만 매 프레임 갱신한다.
 * 내려놓기·파괴·아이템 교체 시 취소하여 새 아이템으로 기존 입력이 이어지지 않게 한다.
 */

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ItemUseTypes.h"
#include "ItemUseComponent.generated.h"

class UCarryComponent;
class UItemUseBehavior;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FItemUseStateChanged);

UCLASS(ClassGroup=(Interaction), meta=(BlueprintSpawnableComponent))
class TESTRRR_API UItemUseComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UItemUseComponent();
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// Enhanced Input Started / Completed / Canceled에 각각 연결한다.
	UFUNCTION(BlueprintCallable, Category="Item Use")
	FItemUseResult BeginUseInput();

	UFUNCTION(BlueprintCallable, Category="Item Use")
	void EndUseInput();

	UFUNCTION(BlueprintCallable, Category="Item Use")
	void CancelUseInput();

	// 운반 상태 변경에 의한 취소. 물리 키를 떼었다고 간주하지는 않는다.
	UFUNCTION(BlueprintCallable, Category="Item Use")
	void CancelUse();

	UFUNCTION(BlueprintPure, Category="Item Use")
	bool IsUsing() const;

	UFUNCTION(BlueprintPure, Category="Item Use")
	FText GetUsePrompt() const;

	// 외부 아이템 상태만 변경된 경우에도 사용 가능 안내를 즉시 다시 조회하게 한다.
	UFUNCTION(BlueprintCallable, Category="Item Use")
	void NotifyUseAvailabilityChanged();

	UPROPERTY(BlueprintAssignable, Category="Item Use")
	FItemUseStateChanged OnUseStateChanged;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	UPROPERTY(Transient)
	TObjectPtr<UCarryComponent> Carry;

	// 콜백 도중에도 같은 동작을 정리할 수 있도록 사용 중인 전략 객체는 보관한다.
	UPROPERTY(Transient)
	TObjectPtr<UItemUseBehavior> ActiveBehavior;

	UPROPERTY(Transient)
	TWeakObjectPtr<AActor> ActiveItem;

	UPROPERTY(Transient)
	FItemUseContext ActiveContext;

	bool bInputHeld = false;
	bool bBeginningUse = false;
	bool bCancelRequested = false;
	bool IsLocalUser() const;
	FItemUseContext MakeContext(AActor* Item) const;
	void FinishSession(bool bCanceled);
	void ClearSession();

	UFUNCTION()
	void HandleCarryStateChanged();
};
