/* H3: E 입력과 유효한 물리 충돌로 같은 탈출문을 여는 벽면 하단 버튼. */

#pragma once

#include "CoreMinimal.h"
#include "Interaction/ButtonSwitchActor.h"
#include "ExitSwitch.generated.h"

class AExitDoor;
class UPrimitiveComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FExitActivated);

UCLASS(Blueprintable)
class TESTRRR_API AExitSwitch : public AButtonSwitchActor
{
	GENERATED_BODY()

public:
	AExitSwitch();
	virtual bool Activate_Implementation(const FInteractionContext& Context) override;

	// BP_ExitDoor 또는 이 C++ 클래스를 지정한다. 비어 있으면 E/물리 충돌 모두 실패한다.
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category="Exit")
	TObjectPtr<AExitDoor> ExitDoor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Exit|Physical", meta=(ClampMin="0", Units="cm/s"))
	float MinimumImpactSpeed = 80.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Exit|Physical", meta=(ClampMin="-1", ClampMax="1"))
	float MinimumFrontApproachDot = 0.25f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Exit|Physical", meta=(ClampMin="0", Units="s"))
	float PhysicalActivationCooldown = 0.75f;

	// 볼링공·농구공·도미노는 이 태그를 가지면 같은 판정으로 처리된다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Exit|Physical")
	FName TriggerableTag = TEXT("ExitTriggerable");

	UPROPERTY(BlueprintAssignable, Category="Exit")
	FExitActivated OnExitActivated;

	UFUNCTION(BlueprintCallable, Category="Exit")
	bool ActivateExit();

private:
	UFUNCTION()
	void OnTargetHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
		FVector NormalImpulse, const FHitResult& Hit);

	double LastPhysicalActivationTime = -DBL_MAX;
};
