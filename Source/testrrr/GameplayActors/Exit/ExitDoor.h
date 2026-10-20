/* H3: 3초 동안 열리는 물리 탈출문과 성공 판정 트리거. */

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ExitDoor.generated.h"

class UBoxComponent;
class UStaticMeshComponent;
class UEscapeSuccessWidget;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FExitDoorStateChanged, bool, bIsOpen);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FEscapeSucceeded);

UCLASS(Blueprintable)
class TESTRRR_API AExitDoor : public AActor
{
	GENERATED_BODY()

public:
	AExitDoor();
	virtual void Tick(float DeltaSeconds) override;

	// 탈출 버튼이 호출하는 공용 진입점. 열린 상태에서도 닫힘 예약을 다시 시작한다.
	UFUNCTION(BlueprintCallable, Category="Exit")
	void ActivateExit();

	UFUNCTION(BlueprintCallable, Category="Exit")
	void CloseExit();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UStaticMeshComponent> DoorMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UBoxComponent> EscapeTrigger;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Exit", meta=(ClampMin="0.1", Units="s"))
	float OpenDuration = 2.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Exit", meta=(ClampMin="0.05", Units="s"))
	float TravelDuration = 0.55f;

	// 닫힌 위치에서 옆으로 밀리는 거리. 문 폭보다 커야 통로가 남는다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Exit")
	FVector OpenOffset = FVector(0.f, -450.f, 0.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Exit|UI")
	TSubclassOf<UEscapeSuccessWidget> SuccessWidgetClass;

	UPROPERTY(BlueprintAssignable, Category="Exit")
	FExitDoorStateChanged OnExitDoorStateChanged;

	UPROPERTY(BlueprintAssignable, Category="Exit")
	FEscapeSucceeded OnEscapeSucceeded;

	UFUNCTION(BlueprintPure, Category="Exit")
	bool IsExitOpen() const { return bIsOpen; }

protected:
	virtual void BeginPlay() override;

private:
	UFUNCTION()
	void OnEscapeTriggerBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
	void ShowSuccess();

	FTimerHandle CloseTimer;
	FVector ClosedLocation = FVector::ZeroVector;
	bool bIsOpen = false;
	bool bEscapeReported = false;
};
