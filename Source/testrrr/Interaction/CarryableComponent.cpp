/*
 * 물건별 운반 가능 조건 구현
 * 기본적으로 물리 시뮬레이션과 충돌 설정을 확인하고 중복 운반을 막는다.
 * 자식 컴포넌트에서 조건을 확장할 때 기본 제약이 필요하면 부모 함수도 호출한다.
 */

#include "Interaction/CarryableComponent.h"
#include "Components/PrimitiveComponent.h"
#include "GameFramework/Actor.h"

UCarryableComponent::UCarryableComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

UPrimitiveComponent* UCarryableComponent::GetCarryPrimitive() const
{
	// 명시적인 설정이 있으면 우선 사용하고, 없으면 루트 컴포넌트를 검사한다.
	return IsValid(CarryPrimitive) ? CarryPrimitive.Get() : Cast<UPrimitiveComponent>(GetOwner()->GetRootComponent());
}

bool UCarryableComponent::CanGrab_Implementation(const FInteractionContext& Context) const
{
	// 이미 운반 중이거나 물리·충돌 준비가 안 된 물건은 잡을 수 없다.
	const UPrimitiveComponent* Body = GetCarryPrimitive();
	return bCanBeCarried && !CurrentCarrier.IsValid() && IsValid(Context.Interactor)
		&& IsValid(Body) && Body->GetOwner() == GetOwner() && Body->IsSimulatingPhysics()
		&& Body->GetCollisionEnabled() == ECollisionEnabled::QueryAndPhysics;
}

bool UCarryableComponent::CanDrop_Implementation(const FInteractionContext& Context) const
{
	// 현재 운반 중인 주체만 일반 내려놓기를 요청할 수 있다.
	return CurrentCarrier.Get() == Context.Interactor;
}

bool UCarryableComponent::CanThrow_Implementation(const FInteractionContext& Context) const
{
	// 고정 허용 설정과 현재 문맥의 내려놓기 조건을 함께 사용한다.
	return bCanThrow && CanDrop(Context);
}

// 기본 알림은 비워두고, 자식 C++/BP에서 필요한 효과만 추가한다.
void UCarryableComponent::OnGrabbed_Implementation(const FInteractionContext& Context) {}
void UCarryableComponent::OnReleased_Implementation(const FInteractionContext& Context) {}
