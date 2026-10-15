/*
 * 토글 스위치 확장 예제
 * ButtonSwitchActor를 상속하여 켜짐 상태를 전환하고 OnToggled 이벤트를 전달한다.
 * 문·조명 등의 작업자는 이 이벤트에 자신의 기능을 연결할 수 있다.
 */

#pragma once

#include "CoreMinimal.h"
#include "ButtonSwitchActor.h"
#include "ToggleSwitchActor.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSwitchToggled, bool, bEnabled);

// 배치 가능한 예제. 문·조명 작업자는 OnToggled 이벤트에 기능을 연결한다.
UCLASS(Blueprintable)
class TESTRRR_API AToggleSwitchActor : public AButtonSwitchActor
{
	GENERATED_BODY()

public:
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Switch")
	bool bIsOn = false;

	// 수신 대상을 특정 클래스에 묶지 않고 상태 변경을 전달한다.
	UPROPERTY(BlueprintAssignable, Category="Switch")
	FSwitchToggled OnToggled;

	virtual bool Activate_Implementation(const FInteractionContext& Context) override;
};
