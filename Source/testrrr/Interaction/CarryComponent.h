/*
 * 플레이어의 운반 상태와 물리 제어
 * 잡기 성공 후 상태를 기록하고, 내려놓기·파괴·거리 초과 시 상태와 충돌을 정리한다.
 * 이 컴포넌트의 IsHolding 결과가 E 상호작용 차단의 기준이다.
 */

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ButtonInteractionTypes.h"
#include "CarryComponent.generated.h"

class UCarryableComponent;
class UPhysicsHandleComponent;
class UPrimitiveComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FCarryStateChanged);

// 운반 여부는 이 컴포넌트에서만 판단한다. E 차단용 bool을 다른 곳에 중복 저장하지 않는다.
UCLASS(ClassGroup=(Interaction), meta=(BlueprintSpawnableComponent))
class TESTRRR_API UCarryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UCarryComponent();
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UFUNCTION(BlueprintPure, Category="Carry")
	bool IsHolding() const;

	UFUNCTION(BlueprintPure, Category="Carry")
	AActor* GetHeldActor() const;

	// 물리 핸들 잡기에 성공한 뒤 운반 상태와 알림을 갱신한다.
	UFUNCTION(BlueprintCallable, Category="Carry")
	bool TryGrab(UCarryableComponent* Target, const FInteractionContext& Context);

	// G 입력의 진입점. 빈손이면 안전하게 실패하고, 운반 중이면 물건의 조건을 확인한다.
	UFUNCTION(BlueprintCallable, Category="Carry")
	bool TryDrop();

	// 이후 던지기 기능이 사용할 조건 조회다. 현재는 던지기 입력이나 힘을 적용하지 않는다.
	UFUNCTION(BlueprintPure, Category="Carry")
	bool CanThrowHeldObject() const;

	// 잡기·해제 완료 알림. 탐색 타이머와 화면 안내가 이 이벤트를 구독한다.
	UPROPERTY(BlueprintAssignable, Category="Carry")
	FCarryStateChanged OnCarryStateChanged;

	// 카메라 앞 목표 거리. 물리 제약을 사용하므로 벽을 통과하도록 위치를 직접 설정하지 않는다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Carry", meta=(ClampMin="80", Units="cm"))
	float HoldDistance = 200.f;

	// 물건이 걸리거나 멀어졌을 때 강제 해제할 거리다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Carry", meta=(ClampMin="50", Units="cm"))
	float MaxCarryDistance = 400.f;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	UPROPERTY(Transient)
	TObjectPtr<UPhysicsHandleComponent> PhysicsHandle;

	// 파괴된 물건을 소유 참조로 붙잡지 않도록 약한 참조를 사용한다.
	UPROPERTY(Transient)
	TWeakObjectPtr<UCarryableComponent> HeldCarryable;

	UPROPERTY(Transient)
	TWeakObjectPtr<UPrimitiveComponent> HeldPrimitive;

	// 잡을 때의 카메라 상대 회전과, 해제 시 복원할 기존 Pawn 충돌 응답이다.
	FQuat RelativeHoldRotation = FQuat::Identity;
	ECollisionResponse PreviousPawnResponse = ECR_Block;
	FInteractionContext MakeContext() const;
	// 일반 내려놓기와 강제 해제가 공유하는 정리 경로. CanDrop 제한을 검사하지 않는다.
	void ReleaseHeldObject();

	UFUNCTION()
	void OnHeldActorDestroyed(AActor* DestroyedActor);
};
