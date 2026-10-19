#pragma once

#include "CoreMinimal.h"
#include "Interaction/PickupActor.h"
#include "BallItem.generated.h"

class UPhysicalMaterial;
class UMaterialInterface;

// 구형 메시·색상·물리 설정을 공유한다. 개별 공의 조작 정책은 자식에서 정한다.
UCLASS(Abstract)
class TESTRRR_API ABallItem : public APickupActor
{
	GENERATED_BODY()
public:
	ABallItem();
	virtual void OnConstruction(const FTransform& Transform) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ball", meta=(ClampMin="0.01", Units="kg"))
	float MassKg = 0.62f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ball")
	FLinearColor BallColor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ball", meta=(ClampMin="0", ClampMax="1"))
	float Roughness = 0.65f;

protected:
	virtual void BeginPlay() override;
	void ApplyBallSettings();

	UPROPERTY(VisibleAnywhere, Instanced, Category="Ball")
	TObjectPtr<UPhysicalMaterial> BallPhysicsMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ball")
	TObjectPtr<UMaterialInterface> BallMaterial;
};
