/*
 * 플레이어의 운반 상태와 물리 제어
 * 잡기 성공 후 상태를 기록하고, 내려놓기·파괴·거리 초과 시 상태와 충돌을 정리한다.
 * 이 컴포넌트의 IsHolding 결과가 E 상호작용 차단의 기준이다.
 */

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ButtonInteractionTypes.h"
#include "ItemUseTypes.h"
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

	UFUNCTION(BlueprintPure, Category="Carry")
	bool CanRotateHeldObject() const;

	UFUNCTION(BlueprintCallable, Category="Carry")
	bool BeginRotateHeldObject();

	UFUNCTION(BlueprintCallable, Category="Carry")
	void EndRotateHeldObject();

	UFUNCTION(BlueprintPure, Category="Carry")
	bool IsRotatingHeldObject() const;

	UFUNCTION(BlueprintCallable, Category="Carry")
	bool RotateHeldObject(FVector2D MouseDelta);

	// 마우스 이동량 기준 각도이므로 DeltaTime을 다시 곱하지 않는다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Carry", meta=(ClampMin="0.01", ClampMax="10"))
	float HeldRotationSensitivity = 0.35f;

	// 던지기 전략에서 사용하는 조건 조회. 이 함수 자체는 힘을 적용하지 않는다.
	UFUNCTION(BlueprintPure, Category="Carry")
	bool CanThrowHeldObject() const;

	// 던지기 전략이 호출하는 물리 적용 진입점. 플레이어 입력에서 직접 호출하지 않는다.
	UFUNCTION(BlueprintCallable, Category="Carry")
	bool TryThrow(const FItemThrowSettings& Settings, FVector Direction);

	// 빙의 해제·종료처럼 일반 CanDrop 조건과 무관하게 정리해야 하는 경우 사용한다.
	UFUNCTION(BlueprintCallable, Category="Carry")
	void ForceRelease();

	// 잡기·해제 완료 알림. 탐색 타이머와 화면 안내가 이 이벤트를 구독한다.
	UPROPERTY(BlueprintAssignable, Category="Carry")
	FCarryStateChanged OnCarryStateChanged;

	// 카메라 앞 목표 거리. 물리 제약을 사용하므로 벽을 통과하도록 위치를 직접 설정하지 않는다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Carry", meta=(ClampMin="80", Units="cm"))
	float HoldDistance = 200.f;

	// 물건이 걸리거나 멀어졌을 때 강제 해제할 거리다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Carry", meta=(ClampMin="50", Units="cm"))
	float MaxCarryDistance = 400.f;

	// 플레이어의 기본 던지는 힘. 실제 맵의 약 7.86kg 예제에 12N·s만 적용하면
	// 1.53m/s로 떨어지는 느낌이 나므로 기본 4배(약 6.1m/s)로 보정한다. 질량은 그대로 사용한다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Carry|Throw", meta=(ClampMin="0.01"))
	float ThrowStrengthMultiplier = 4.f;

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

	// 수동으로 돌린 자세는 Shift 해제/시선 이동 후에도 월드 기준으로 유지한다.
	FQuat HeldWorldRotation = FQuat::Identity;
	bool bRotatingHeldObject = false;
	ECollisionResponse PreviousPawnResponse = ECR_Block;
	// 물건 알림 도중 새로운 잡기·던지기가 재진입하여 같은 물리 부위를 조작하지 못하게 한다.
	bool bCarryTransition = false;
	FInteractionContext MakeContext() const;
	// 발사 자체를 지형 접촉으로 막지 않고, 자기 몸 충돌의 복원 시점만 별도로 관리한다.
	struct FPendingPawnCollisionRestore
	{
		TWeakObjectPtr<UPrimitiveComponent> Body;
		ECollisionResponse OriginalResponse = ECR_Block;
		double RestoreAfterTime = 0.0;
	};
	TArray<FPendingPawnCollisionRestore> PendingPawnCollisionRestores;
	FTimerHandle PawnCollisionRestoreTimer;
	void QueuePawnCollisionRestore(UPrimitiveComponent* Body, ECollisionResponse OriginalResponse);
	void UpdatePendingPawnCollisions();
	ECollisionResponse TakePendingPawnResponse(UPrimitiveComponent* Body);
	bool IsOverlappingThrower(UPrimitiveComponent* Body) const;
	// 일반 내려놓기와 강제 해제가 공유하는 정리 경로. CanDrop 제한을 검사하지 않는다.
	void ReleaseHeldObject(ECarryReleaseReason Reason = ECarryReleaseReason::Forced);

	UFUNCTION()
	void OnHeldActorDestroyed(AActor* DestroyedActor);
};
