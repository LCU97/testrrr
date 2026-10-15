/*
 * 상호작용 액터의 공통 부모
 * 아이템과 스위치가 상속할 기본 활성화 조건과 안내 문구를 제공한다.
 * 자식 클래스는 조건과 실행 함수를 재정의하고, 키 입력은 플레이어 쪽에 맡긴다.
 */

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interactable.h"
#include "InteractableActor.generated.h"

// 상속 구조: InteractableActor → PickupActor / ButtonSwitchActor → 개별 콘텐츠.
UCLASS(Abstract, Blueprintable)
class TESTRRR_API AInteractableActor : public AActor, public IInteractable
{
	GENERATED_BODY()

public:
	AInteractableActor();
	virtual FInteractionInfo GetInteractionInfo_Implementation(const FInteractionContext& Context) const override;
	virtual bool CanInteract_Implementation(const FInteractionContext& Context) const override;
	virtual FInteractionResult Interact_Implementation(const FInteractionContext& Context) override;

	// 공통 사용 허용 스위치. 자식의 조건 판정에서도 부모 조건을 유지할 수 있다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Interaction")
	bool bInteractionEnabled = true;

	// 키 이름을 제외한 행동명만 지정한다. 예: 잡기, 누르기.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Interaction")
	FText InteractionPrompt;

	// 실행할 수 없을 때 UI에 보여줄 이유. 조건별 문구는 자식에서 확장한다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Interaction")
	FText DisabledReason;
};
