/*
 * 잡을 수 있는 아이템의 부모 액터
 * 기본 물리 메쉬와 CarryableComponent를 제공하고, E 상호작용을 잡기 요청으로 연결한다.
 * 아이템 작업자는 이 클래스를 상속해 메쉬·운반 설정·좌클릭 사용 전략·던져진 뒤 반응을 구현한다.
 */

#pragma once

#include "CoreMinimal.h"
#include "InteractableActor.h"
#include "UsableItem.h"
#include "PickupActor.generated.h"

class UStaticMeshComponent;
class UCarryableComponent;
class UItemUseBehavior;

// 개별 아이템의 부모 클래스. 기본 메쉬와 운반 설정을 상속해서 사용한다.
UCLASS(Blueprintable)
class TESTRRR_API APickupActor : public AInteractableActor, public IUsableItem
{
	GENERATED_BODY()

public:
	APickupActor();
	virtual bool CanInteract_Implementation(const FInteractionContext& Context) const override;
	virtual FInteractionResult Interact_Implementation(const FInteractionContext& Context) override;
	virtual UItemUseBehavior* GetItemUseBehavior_Implementation() const override;
	virtual bool CanUseItem_Implementation(const FItemUseContext& Context) const override;
	virtual void OnItemThrown_Implementation(const FItemThrowInfo& Info) override;

	// 아이템마다 별도 동작 인스턴스를 소유한다. 비워두면 좌클릭 사용 기능이 없다.
	// BP 기본값에서 ThrowItemUseBehavior 또는 작업자가 만든 사용 동작을 선택한다.
	UPROPERTY(EditAnywhere, Instanced, BlueprintReadOnly, Category="Item Use")
	TObjectPtr<UItemUseBehavior> UseBehavior;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Item Use")
	bool bItemUseEnabled = true;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UStaticMeshComponent> Mesh;

	// 물건별 조건만 보관한다. 현재 운반 상태는 플레이어 CarryComponent의 책임이다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UCarryableComponent> Carryable;
};
