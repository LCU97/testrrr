/*
 * 배치용 던지기 아이템의 기본 설정
 * 입력 처리는 추가하지 않고 이 아이템이 사용할 전략 객체만 생성한다.
 */

#include "Interaction/ThrowablePickupActor.h"
#include "Interaction/ThrowItemUseBehavior.h"
#include "Components/StaticMeshComponent.h"

AThrowablePickupActor::AThrowablePickupActor()
{
	// 배치용 큐브를 손에 들기 쉬운 25cm 크기로 만든다. 별도 가짜 질량은 지정하지 않는다.
	// 실제 질량은 이 크기와 메쉬의 물리 설정으로 계산되며 아이템 작업자가 조정할 수 있다.
	Mesh->SetRelativeScale3D(FVector(0.25f));
	UseBehavior = CreateDefaultSubobject<UThrowItemUseBehavior>(TEXT("ThrowUseBehavior"));
}
