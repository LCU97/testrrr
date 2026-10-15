/*
 * 플레이어의 잡기·내려놓기 구현
 * Physics Handle로 물리 충돌을 유지하며 물건을 카메라 앞에 따라오게 한다.
 * 대상 탐색의 0.2초 주기와 달리 운반 위치는 들고 있는 동안 매 프레임 갱신한다.
 */

#include "Interaction/CarryComponent.h"
#include "Interaction/CarryableComponent.h"
#include "Interaction/InteractionView.h"
#include "Components/PrimitiveComponent.h"
#include "GameFramework/Pawn.h"
#include "PhysicsEngine/PhysicsHandleComponent.h"

UCarryComponent::UCarryComponent()
{
	// 대상 탐색은 타이머로 처리하지만, 물건의 추종 위치는 운반 중 매 프레임 갱신한다.
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	PrimaryComponentTick.TickGroup = TG_PrePhysics;
}

void UCarryComponent::BeginPlay()
{
	Super::BeginPlay();
	// 소유 액터에 물리 핸들을 등록하고, 급격한 움직임을 보간하도록 설정한다.
	PhysicsHandle = NewObject<UPhysicsHandleComponent>(GetOwner(), TEXT("InteractionPhysicsHandle"));
	PhysicsHandle->RegisterComponent();
	PhysicsHandle->SetInterpolationSpeed(15.f);
}

void UCarryComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// 종료 시에도 기존 물건의 충돌과 운반 상태를 먼저 복원한다.
	ReleaseHeldObject();
	if (IsValid(PhysicsHandle))
	{
		PhysicsHandle->DestroyComponent();
	}
	Super::EndPlay(EndPlayReason);
}

bool UCarryComponent::IsHolding() const
{
	// 별도 상태 플래그 대신 물건 정책과 실제 물리 부위의 유효성으로 판단한다.
	return HeldCarryable.IsValid() && HeldPrimitive.IsValid();
}

AActor* UCarryComponent::GetHeldActor() const
{
	return IsHolding() ? HeldCarryable->GetOwner() : nullptr;
}

FInteractionContext UCarryComponent::MakeContext() const
{
	FInteractionContext Context;
	Context.Interactor = GetOwner();
	return Context;
}

bool UCarryComponent::TryGrab(UCarryableComponent* Target, const FInteractionContext& Context)
{
	// 플레이어 상태와 물건별 조건을 모두 검사하고, 다른 주체의 문맥 요청은 받지 않는다.
	if (IsHolding() || !IsValid(PhysicsHandle) || !IsValid(Target)
		|| Context.Interactor != GetOwner() || !Target->CanGrab(Context))
	{
		return false;
	}

	// 확장된 CanGrab 구현과 별개로 물리 제어에 꼭 필요한 최소 조건을 재확인한다.
	UPrimitiveComponent* Body = Target->GetCarryPrimitive();
	if (!IsValid(Body) || Body->GetOwner() != Target->GetOwner() || !Body->IsSimulatingPhysics()
		|| Target->CurrentCarrier.IsValid())
	{
		return false;
	}

	// 이전 참조나 물리 제약이 남아 있다면 새 물건을 잡기 전에 정리한다.
	ReleaseHeldObject();
	FVector ViewLocation;
	FRotator ViewRotation;
	InteractionView::GetViewPoint(GetOwner(), ViewLocation, ViewRotation);
	const FVector GrabLocation = Body->GetCenterOfMass();
	PhysicsHandle->GrabComponentAtLocationWithRotation(Body, NAME_None, GrabLocation, Body->GetComponentRotation());
	if (PhysicsHandle->GetGrabbedComponent() != Body)
	{
		return false;
	}

	// 실제 물리 핸들 잡기가 성공한 경우에만 운반 상태를 확정한다.
	HeldCarryable = Target;
	HeldPrimitive = Body;
	Target->CurrentCarrier = GetOwner();
	RelativeHoldRotation = ViewRotation.Quaternion().Inverse() * Body->GetComponentQuat();
	// 운반 중 플레이어와의 충돌은 무시하되, 벽·바닥과의 물리 충돌은 유지한다.
	PreviousPawnResponse = Body->GetCollisionResponseToChannel(ECC_Pawn);
	Body->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
	Body->WakeAllRigidBodies();
	// 들고 있던 액터가 삭제되면 G 입력 없이도 즉시 정리한다.
	Target->GetOwner()->OnDestroyed.AddUniqueDynamic(this, &UCarryComponent::OnHeldActorDestroyed);
	SetComponentTickEnabled(true);
	// 상태를 확정한 뒤 물건 알림과 UI/탐색 알림을 전달한다.
	Target->OnGrabbed(Context);
	OnCarryStateChanged.Broadcast();
	return IsHolding();
}

bool UCarryComponent::TryDrop()
{
	if (!IsHolding())
	{
		ReleaseHeldObject();
		return false;
	}
	// 일반 G 내려놓기는 물건별 조건을 존중한다. 강제 해제 경로는 별도로 정리한다.
	if (!HeldCarryable->CanDrop(MakeContext()))
	{
		return false;
	}
	ReleaseHeldObject();
	return true;
}

bool UCarryComponent::CanThrowHeldObject() const
{
	// 던지기 금지 설정은 확장된 조건 함수와 별개로 항상 확인한다.
	return IsHolding() && HeldCarryable->bCanThrow && HeldCarryable->CanThrow(MakeContext());
}

void UCarryComponent::ReleaseHeldObject()
{
	UCarryableComponent* Carryable = HeldCarryable.Get();
	UPrimitiveComponent* Body = HeldPrimitive.Get();
	// 정리할 상태가 있었을 때만 변경 알림을 보낸다.
	const bool bHadState = Carryable || Body || (PhysicsHandle && PhysicsHandle->GetGrabbedComponent());
	if (IsValid(PhysicsHandle))
	{
		PhysicsHandle->ReleaseComponent();
	}
	if (IsValid(Body))
	{
		// 잡기 전에 저장한 응답을 복원한다. 무조건 Block으로 덮어쓰지 않는다.
		Body->SetCollisionResponseToChannel(ECC_Pawn, PreviousPawnResponse);
		Body->WakeAllRigidBodies();
	}
	// 콜백에서 상태를 조회하거나 다시 요청하더라도 해제된 상태로 보이게 먼저 비운다.
	HeldCarryable.Reset();
	HeldPrimitive.Reset();
	SetComponentTickEnabled(false);
	if (IsValid(Carryable))
	{
		Carryable->GetOwner()->OnDestroyed.RemoveDynamic(this, &UCarryComponent::OnHeldActorDestroyed);
		Carryable->CurrentCarrier.Reset();
		Carryable->OnReleased(MakeContext());
	}
	if (bHadState)
	{
		OnCarryStateChanged.Broadcast();
	}
}

void UCarryComponent::OnHeldActorDestroyed(AActor* DestroyedActor)
{
	ReleaseHeldObject();
}

void UCarryComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	// 외부 파괴·물리 해제·빙의 변경으로 운반이 불가능해지면 공통 경로로 강제 해제한다.
	const APawn* Pawn = Cast<APawn>(GetOwner());
	if (!IsHolding() || !PhysicsHandle || PhysicsHandle->GetGrabbedComponent() != HeldPrimitive.Get()
		|| !HeldPrimitive->IsSimulatingPhysics() || (Pawn && !Pawn->IsLocallyControlled()))
	{
		ReleaseHeldObject();
		return;
	}
	FVector ViewLocation;
	FRotator ViewRotation;
	InteractionView::GetViewPoint(GetOwner(), ViewLocation, ViewRotation);
	if (FVector::DistSquared(ViewLocation, HeldPrimitive->GetCenterOfMass()) > FMath::Square(MaxCarryDistance))
	{
		// 벽 뒤에 걸린 물건을 계속 끌거나 지나치게 멀리 운반하는 상황을 막는다.
		ReleaseHeldObject();
		return;
	}
	// 카메라 앞 목표 위치와 잡을 때의 상대 회전을 매 프레임 물리 핸들에 전달한다.
	PhysicsHandle->SetTargetLocationAndRotation(ViewLocation + ViewRotation.Vector() * HoldDistance,
		(ViewRotation.Quaternion() * RelativeHoldRotation).Rotator());
}
