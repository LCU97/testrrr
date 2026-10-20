#pragma once

#include "CoreMinimal.h"
#include "GameplayActors/Items/PlacementItem.h"
#include "DominoItem.generated.h"

// 얇은 X축 방향으로 쓰러지는 테스트용 도미노. 기존 줍기/배치 기능을 상속한다.
UCLASS(Blueprintable, meta=(DisplayName="Domino Item"))
class TESTRRR_API ADominoItem : public APlacementItem
{
	GENERATED_BODY()
public:
	ADominoItem();
};
