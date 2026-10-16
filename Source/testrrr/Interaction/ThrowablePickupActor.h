/*
 * 던지기 사용 동작을 미리 설정한 배치용 아이템
 * 일반 PickupActor에는 사용 동작이 없으며, 이 자식만 기본 던지기 전략을 소유한다.
 * 농구공·폭탄 등은 이 액터를 상속하거나 다른 PickupActor에 같은 전략을 설정할 수 있다.
 */

#pragma once

#include "CoreMinimal.h"
#include "PickupActor.h"
#include "ThrowablePickupActor.generated.h"

UCLASS(Blueprintable)
class TESTRRR_API AThrowablePickupActor : public APickupActor
{
	GENERATED_BODY()

public:
	AThrowablePickupActor();
};
