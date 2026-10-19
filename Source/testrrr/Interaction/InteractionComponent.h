/*
 * 플레이어의 대상 탐색과 상호작용 요청
 * 0.2초 간격 Line Trace, 운반 중 E 차단, 공통 인터페이스 호출, 화면 안내를 담당한다.
 * 개별 아이템·스위치의 기능은 이 컴포넌트에 추가하지 않고 대상 액터에서 구현한다.
 */

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ButtonInteractionTypes.h"
#include "InteractionComponent.generated.h"

class UCarryComponent;
class UInteractionPromptWidget;
class UItemUseComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FInteractionPromptChanged, FText, Prompt, bool, bAvailable);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FInteractionFinished, FInteractionResult, Result);

// 대상 종류별 분기 없이 IInteractable 계약으로만 요청한다.
UCLASS(ClassGroup=(Interaction), meta=(BlueprintSpawnableComponent))
class TESTRRR_API UInteractionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category="Interaction")
	FText RotateInputLabel = FText::FromString(TEXT("R"));

	UInteractionComponent();

	// 안내용 탐색 주기. E를 누른 순간의 재검사는 이 타이머를 기다리지 않는다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Interaction", meta=(ClampMin="0.01", Units="s"))
	float TraceInterval = 0.2f;

	// 카메라에서 앞으로 검사할 최대 거리이며 단위는 cm다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Interaction", meta=(ClampMin="1", Units="cm"))
	float InteractionDistance = 300.f;

	// 상호작용 대상뿐 아니라 벽도 이 채널을 막아야 벽 뒤 대상을 선택하지 않는다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Interaction")
	TEnumAsByte<ECollisionChannel> TraceChannel = ECC_Visibility;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Interaction")
	bool bInteractionEnabled = true;

	// 자체 UI를 OnPromptChanged에 연결한다면 기본 위젯 생성을 끌 수 있다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Interaction|UI")
	bool bShowDefaultPrompt = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Interaction|UI")
	FText InteractInputLabel = FText::FromString(TEXT("E"));

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Interaction|UI")
	FText DropInputLabel = FText::FromString(TEXT("G"));

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Interaction|UI")
	FText UseInputLabel = FText::FromString(TEXT("좌클릭"));

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Interaction|UI")
	TSubclassOf<UInteractionPromptWidget> PromptWidgetClass;

	UPROPERTY(BlueprintAssignable, Category="Interaction|UI")
	FInteractionPromptChanged OnPromptChanged;

	UPROPERTY(BlueprintAssignable, Category="Interaction")
	FInteractionFinished OnInteractionFinished;

	UFUNCTION(BlueprintPure, Category="Interaction")
	bool CanInteractNow() const;

	// E 요청의 공통 진입점. 캐시된 대상 대신 현재 시선으로 검사하고 실행한다.
	UFUNCTION(BlueprintCallable, Category="Interaction")
	FInteractionResult TryInteract();

	UFUNCTION(BlueprintPure, Category="Interaction")
	AActor* GetFocusedActor() const { return FocusedActor.Get(); }

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	UPROPERTY(Transient)
	TObjectPtr<UCarryComponent> Carry;

	UPROPERTY(Transient)
	TObjectPtr<UItemUseComponent> ItemUse;

	// 안내용 캐시이므로 대상의 수명을 연장하지 않는다.
	UPROPERTY(Transient)
	TWeakObjectPtr<AActor> FocusedActor;

	UPROPERTY(Transient)
	TObjectPtr<UInteractionPromptWidget> PromptWidget;

	// 동일한 문구를 매번 외부 UI에 알리지 않도록 마지막 안내를 보관한다.
	FText LastPrompt;
	bool bLastAvailable = false;
	FTimerHandle TraceTimer;
	bool FindTarget(FInteractionContext& OutContext) const;
	void RefreshTarget();
	void SetPrompt(const FText& Prompt, bool bAvailable);

	UFUNCTION()
	void HandleCarryStateChanged();

	UFUNCTION()
	void HandleUseStateChanged();
};
