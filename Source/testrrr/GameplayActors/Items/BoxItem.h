#pragma once
#include "CoreMinimal.h"
#include "GameplayActors/Items/PlacementItem.h"
#include "BoxItem.generated.h"

UCLASS(meta=(DisplayName="Box Item"))
class TESTRRR_API ABoxItem : public APlacementItem
{
	GENERATED_BODY()
public:
	ABoxItem();
};
