/*
 * 물건별 운반 설정과 확장 지점
 * 운반할 물리 메쉬, 잡기·내려놓기·던지기 조건 및 운반 알림을 제공한다.
 * 플레이어가 현재 무엇을 들고 있는지는 CarryComponent가 관리한다.
 */

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ButtonInteractionTypes.h"
#include "ItemUseTypes.h"
#include "CarryableComponent.generated.h"

class UPrimitiveComponent;

// 물건별 운반 정책을 조합하는 컴포넌트. 플레이어 입력을 직접 받지 않는다.
UCLASS(ClassGroup=(Interaction), meta=(BlueprintSpawnableComponent))
class TESTRRR_API UCarryableComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UCarryableComponent();

	// 실제 운반할 물리 부위를 지정한다. 미지정 시 소유 액터의 루트 Primitive를 사용한다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Carry")
	TObjectPtr<UPrimitiveComponent> CarryPrimitive;

	// 잡기 허용 여부. 일시적인 추가 조건은 CanGrab에서 판단한다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Carry")
	bool bCanBeCarried = true;

	// 던지기만 제한하며, false여도 일반 내려놓기는 가능하다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Carry")
	bool bCanThrow = true;

	// Shift + 마우스 회전을 아이템별로 제한할 수 있다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Carry")
	bool bCanRotateWhileCarried = true;

	UFUNCTION(BlueprintPure, Category="Carry")
	UPrimitiveComponent* GetCarryPrimitive() const;

	// 잡기 조건 확장 지점. 기본 물리·충돌 조건을 유지하려면 부모 구현도 호출한다.
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Carry")
	bool CanGrab(const FInteractionContext& Context) const;

	// 일반 내려놓기 조건. 파괴·거리 초과 등의 강제 해제는 이 조건을 거치지 않는다.
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Carry")
	bool CanDrop(const FInteractionContext& Context) const;

	// 던지기 조건 확장 지점. 실제 던지는 힘이나 입력은 여기서 처리하지 않는다.
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Carry")
	bool CanThrow(const FInteractionContext& Context) const;

	// 프레임워크가 운반 상태를 기록한 뒤 호출하는 물건별 알림이다.
	UFUNCTION(BlueprintNativeEvent, Category="Carry")
	void OnGrabbed(const FInteractionContext& Context);

	// 프레임워크가 물리 제약과 운반 상태를 해제한 뒤 호출한다.
	UFUNCTION(BlueprintNativeEvent, Category="Carry")
	void OnReleased(const FInteractionContext& Context);

	// 기존 OnReleased와 별개로 해제 사유가 필요한 아이템이 구독할 수 있는 확장점이다.
	UFUNCTION(BlueprintNativeEvent, Category="Carry")
	void OnReleasedWithReason(const FInteractionContext& Context, ECarryReleaseReason Reason);

	UPROPERTY(Transient, BlueprintReadOnly, Category="Carry")
	ECarryReleaseReason LastReleaseReason = ECarryReleaseReason::Forced;

	// 중복 운반 방지용 약한 참조. CarryComponent가 관리하므로 아이템 코드에서 직접 바꾸지 않는다.
	UPROPERTY(Transient, BlueprintReadOnly, Category="Carry")
	TWeakObjectPtr<AActor> CurrentCarrier;
};
