/*
 * 잡을 수 있는 아이템의 부모 액터
 * 기본 물리 메쉬와 CarryableComponent를 제공하고, E 상호작용을 잡기 요청으로 연결한다.
 * 아이템 작업자는 이 클래스를 상속해 메쉬와 운반 설정을 변경한다.
 */

#pragma once

#include "CoreMinimal.h"
#include "InteractableActor.h"
#include "PickupActor.generated.h"

class UStaticMeshComponent;
class UCarryableComponent;

// 개별 아이템의 부모 클래스. 기본 메쉬와 운반 설정을 상속해서 사용한다.
UCLASS(Blueprintable)
class TESTRRR_API APickupActor : public AInteractableActor
{
	GENERATED_BODY()

public:
	APickupActor();
	virtual bool CanInteract_Implementation(const FInteractionContext& Context) const override;
	virtual FInteractionResult Interact_Implementation(const FInteractionContext& Context) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UStaticMeshComponent> Mesh;

	// 물건별 조건만 보관한다. 현재 운반 상태는 플레이어 CarryComponent의 책임이다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UCarryableComponent> Carryable;
};
