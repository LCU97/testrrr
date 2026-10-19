#pragma once

#include "CoreMinimal.h"
#include "Interaction/PickupActor.h"
#include "PlacementItem.generated.h"

// 실제 배치용 아이템의 크기와 물리 기본값. 입력·운반은 기존 프레임워크를 사용한다.
UCLASS(Abstract)
class TESTRRR_API APlacementItem : public APickupActor
{
	GENERATED_BODY()
public:
	APlacementItem();
	virtual void OnConstruction(const FTransform& Transform) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Item", meta=(ClampMin="0.1", Units="kg"))
	float ItemMass = 5.f;

protected:
	// C++ 생성자에서만 기본 크기를 지정한다. 이후 에디터/BP의 Scale은 덮어쓰지 않는다.
	void SetDefaultItemDimensions(const FVector& Dimensions);
};
