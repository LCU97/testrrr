/*
 * 스위치 액터의 공통 부모
 * 활성화 조건, 재진입 방지, 성공 후 쿨다운을 공통 처리한다.
 * 스위치 작업자는 Activate를 재정의하고 효과가 수락된 경우 true를 반환한다.
 */

#pragma once

#include "CoreMinimal.h"
#include "InteractableActor.h"
#include "ButtonSwitchActor.generated.h"

class UStaticMeshComponent;

// 생성·환경 변경 등 스위치별 기능은 자식의 Activate에서 구현한다.
UCLASS(Abstract, Blueprintable)
class TESTRRR_API AButtonSwitchActor : public AInteractableActor
{
	GENERATED_BODY()

public:
	AButtonSwitchActor();
	virtual bool CanInteract_Implementation(const FInteractionContext& Context) const override;
	virtual FInteractionResult Interact_Implementation(const FInteractionContext& Context) override;

	// 효과 요청이 수락된 경우에만 true를 반환한다. 부모가 이후 쿨다운을 적용한다.
	UFUNCTION(BlueprintNativeEvent, Category="Switch")
	bool Activate(const FInteractionContext& Context);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UStaticMeshComponent> Mesh;

	// 실패한 요청에는 적용하지 않는 성공 후 재사용 대기 시간이다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Switch", meta=(ClampMin="0", Units="s"))
	float Cooldown = 0.2f;

private:
	// 다음 실행을 허용할 월드 시간과, 효과 호출 중 재진입을 막는 임시 상태다.
	double NextActivationTime = 0.0;
	bool bActivating = false;
};
