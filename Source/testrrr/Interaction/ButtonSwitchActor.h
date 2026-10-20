/*
 * 스위치 액터의 공통 부모
 * 활성화 조건, 재진입 방지, 성공 후 쿨다운을 공통 처리한다.
 * 스위치 작업자는 Activate를 재정의하고 효과가 수락된 경우 true를 반환한다.
 */

#pragma once

#include "CoreMinimal.h"
#include "InteractableActor.h"
#include "ButtonSwitchActor.generated.h"

class USceneComponent;
class UBoxComponent;
class UPrimitiveComponent;
class UStaticMeshComponent;

// 생성·환경 변경 등 스위치별 기능은 자식의 Activate에서 구현한다.
UCLASS(Abstract, Blueprintable)
class TESTRRR_API AButtonSwitchActor : public AInteractableActor
{
	GENERATED_BODY()

public:
	AButtonSwitchActor();
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual bool CanInteract_Implementation(const FInteractionContext& Context) const override;
	virtual FInteractionResult Interact_Implementation(const FInteractionContext& Context) override;

	// 효과 요청이 수락된 경우에만 true를 반환한다. 부모가 이후 쿨다운을 적용한다.
	UFUNCTION(BlueprintNativeEvent, Category="Switch")
	bool Activate(const FInteractionContext& Context);

	// 액터와 생성 지점은 이 1배율 루트를 기준으로 배치한다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<USceneComponent> SwitchRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UStaticMeshComponent> Mesh;

	// 메시 충돌과 분리된 물리 입력 영역이다. 실제 버튼보다 조금 크게 두어 물체 접촉을 감지한다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UBoxComponent> PhysicalTrigger;

	// 켜면 물리 시뮬레이션 중인 InteractableActor가 트리거에 처음 닿을 때 Activate를 요청한다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Switch|Physical")
	bool bPhysicalOverlapEnabled = true;

	// 손에 든 물체·키네마틱 물체가 아닌, 실제 물리 물체만 반응시키는 기본 안전장치다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Switch|Physical")
	bool bRequireSimulatingPhysics = true;

	// 실패한 요청에는 적용하지 않는 성공 후 재사용 대기 시간이다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Switch", meta=(ClampMin="0", Units="s"))
	float Cooldown = 0.2f;

	// 버튼의 전면은 액터 로컬 +X 방향이다. 누를 때는 로컬 -X 방향으로 벽 쪽에 들어간다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Switch|Animation", meta=(ClampMin="0", Units="cm"))
	float PressDepth = 8.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Switch|Animation", meta=(ClampMin="0.01", Units="s"))
	float PressInDuration = 0.08f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Switch|Animation", meta=(ClampMin="0.01", Units="s"))
	float ReleaseDuration = 0.12f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Switch|Animation")
	bool bIsPressed = false;

private:
	UFUNCTION()
	void OnPhysicalTriggerBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnPhysicalTriggerEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	void BeginPressAnimation();
	void UpdatePressAnimation();

	// 액터가 여러 물리 부위를 가져도 완전히 트리거 밖으로 나가기 전에는 다시 누르지 않는다.
	TMap<TWeakObjectPtr<AActor>, int32> PhysicalOverlapCounts;

	// 다음 실행을 허용할 월드 시간과, 효과 호출 중 재진입을 막는 임시 상태다.
	double NextActivationTime = 0.0;
	bool bActivating = false;
	FVector ReleasedMeshLocation = FVector::ZeroVector;
	double PressStartTime = 0.0;
	double ReleaseStartTime = 0.0;
};
