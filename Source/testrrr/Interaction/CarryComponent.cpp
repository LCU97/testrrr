/*
 * 플레이어의 잡기·내려놓기·던지기 구현
 * Physics Handle로 물리 충돌을 유지하며 물건을 카메라 앞에 따라오게 한다.
 * 운반 위치는 매 프레임 갱신하며, 던지기는 질량을 반영하는 충격량을 한 번 적용한다.
 */

#include "Interaction/CarryComponent.h"
#include "Interaction/CarryableComponent.h"
#include "Interaction/InteractionView.h"
#include "Interaction/ItemThrowPhysics.h"
#include "Interaction/UsableItem.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "PhysicsEngine/PhysicsHandleComponent.h"
#include "Templates/UnrealTemplate.h"
#include "TimerManager.h"

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
	ForceRelease();
	GetWorld()->GetTimerManager().ClearTimer(PawnCollisionRestoreTimer);
	// 플레이어가 사라지면 더 이상 자신의 몸과 충돌할 일이 없으므로 원래 설정으로 정리한다.
	for (const FPendingPawnCollisionRestore& Pending : PendingPawnCollisionRestores)
	{
		if (UPrimitiveComponent* Body = Pending.Body.Get())
		{
			if (Body->GetCollisionResponseToChannel(ECC_Pawn) == ECR_Ignore)
			{
				Body->SetCollisionResponseToChannel(ECC_Pawn, Pending.OriginalResponse);
			}
		}
	}
	PendingPawnCollisionRestores.Reset();
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
	if (bCarryTransition || IsHolding() || !IsValid(PhysicsHandle) || !IsValid(Target)
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
	TGuardValue<bool> TransitionGuard(bCarryTransition, true);
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
	// 발사 직후 다시 잡았다면 임시 Ignore 대신 발사 전의 원래 응답을 이어받는다.
	PreviousPawnResponse = TakePendingPawnResponse(Body);
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
	if (bCarryTransition)
	{
		return false;
	}
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
	TGuardValue<bool> TransitionGuard(bCarryTransition, true);
	ReleaseHeldObject(ECarryReleaseReason::Drop);
	return true;
}

bool UCarryComponent::CanThrowHeldObject() const
{
	// 던지기 금지 설정은 확장된 조건 함수와 별개로 항상 확인한다.
	return IsHolding() && HeldCarryable->bCanThrow && HeldCarryable->CanThrow(MakeContext());
}

bool UCarryComponent::IsOverlappingThrower(UPrimitiveComponent* Body) const
{
	TArray<FOverlapResult> Overlaps;
	FComponentQueryParams Params(SCENE_QUERY_STAT(ThrowerOverlap), Body->GetOwner());
	// 운반 중부터 Pawn을 무시하므로 실제 물리 형상으로 자신의 몸과의 겹침만 조회한다.
	// 바닥·벽 접촉은 발사 취소 조건으로 사용하지 않는다.
	Body->BodyInstance.OverlapMulti(Overlaps, GetWorld(), nullptr, Body->GetComponentLocation(),
		Body->GetComponentQuat(), Body->GetCollisionObjectType(), Params, FCollisionResponseParams(ECR_Block),
		FCollisionObjectQueryParams(ECC_Pawn));
	for (const FOverlapResult& Overlap : Overlaps)
	{
		const UPrimitiveComponent* Other = Overlap.GetComponent();
		if (IsValid(Other) && Other->GetOwner() == GetOwner()
			&& CollisionEnabledHasPhysics(Other->GetCollisionEnabled())
			&& Other->GetCollisionResponseToChannel(Body->GetCollisionObjectType()) == ECR_Block)
		{
			return true;
		}
	}
	return false;
}

void UCarryComponent::QueuePawnCollisionRestore(UPrimitiveComponent* Body, ECollisionResponse OriginalResponse)
{
	if (OriginalResponse == ECR_Ignore)
	{
		return;
	}
	FPendingPawnCollisionRestore Pending;
	Pending.Body = Body;
	Pending.OriginalResponse = OriginalResponse;
	Pending.RestoreAfterTime = GetWorld()->GetTimeSeconds() + 0.15;
	PendingPawnCollisionRestores.Add(Pending);
	if (!GetWorld()->GetTimerManager().IsTimerActive(PawnCollisionRestoreTimer))
	{
		GetWorld()->GetTimerManager().SetTimer(PawnCollisionRestoreTimer, this,
			&UCarryComponent::UpdatePendingPawnCollisions, 0.03f, true);
	}
}

ECollisionResponse UCarryComponent::TakePendingPawnResponse(UPrimitiveComponent* Body)
{
	ECollisionResponse Response = Body->GetCollisionResponseToChannel(ECC_Pawn);
	for (int32 Index = PendingPawnCollisionRestores.Num() - 1; Index >= 0; --Index)
	{
		if (PendingPawnCollisionRestores[Index].Body.Get() == Body)
		{
			// 재잡기 후 예전 타이머가 운반 중인 물건의 충돌을 복원하지 않도록 기록도 제거한다.
			if (Response == ECR_Ignore)
			{
				Response = PendingPawnCollisionRestores[Index].OriginalResponse;
			}
			PendingPawnCollisionRestores.RemoveAtSwap(Index);
		}
	}
	if (PendingPawnCollisionRestores.IsEmpty())
	{
		GetWorld()->GetTimerManager().ClearTimer(PawnCollisionRestoreTimer);
	}
	return Response;
}

void UCarryComponent::UpdatePendingPawnCollisions()
{
	for (int32 Index = PendingPawnCollisionRestores.Num() - 1; Index >= 0; --Index)
	{
		const FPendingPawnCollisionRestore Pending = PendingPawnCollisionRestores[Index];
		UPrimitiveComponent* Body = Pending.Body.Get();
		if (!IsValid(Body) || Body->GetCollisionResponseToChannel(ECC_Pawn) != ECR_Ignore)
		{
			// 아이템이 파괴되거나 외부 기능이 충돌 설정을 바꿨으면 그 결정을 덮어쓰지 않는다.
			PendingPawnCollisionRestores.RemoveAtSwap(Index);
			continue;
		}
		if (GetWorld()->GetTimeSeconds() >= Pending.RestoreAfterTime && !IsOverlappingThrower(Body))
		{
			Body->SetCollisionResponseToChannel(ECC_Pawn, Pending.OriginalResponse);
			PendingPawnCollisionRestores.RemoveAtSwap(Index);
		}
	}
	if (PendingPawnCollisionRestores.IsEmpty())
	{
		GetWorld()->GetTimerManager().ClearTimer(PawnCollisionRestoreTimer);
	}
}

bool UCarryComponent::TryThrow(const FItemThrowSettings& Settings, FVector Direction)
{
	if (bCarryTransition || !CanThrowHeldObject() || !PhysicsHandle || PhysicsHandle->GetGrabbedComponent() != HeldPrimitive.Get())
	{
		return false;
	}
	UPrimitiveComponent* Body = HeldPrimitive.Get();
	AActor* Item = GetHeldActor();
	FItemThrowInfo Info;
	FVector ImpulseUE;
	// 아이템 고유 충격량에 플레이어의 힘을 곱한다. 저장된 아이템 설정 자체는 바꾸지 않는다.
	FItemThrowSettings AdjustedSettings = Settings;
	AdjustedSettings.ImpulseNewtonSeconds *= ThrowStrengthMultiplier;
	// 질량은 이름표나 별도 변수 대신 실제 운반 중인 물리 부위에서 가져온다.
	if (!Body->IsSimulatingPhysics() || !FMath::IsFinite(ThrowStrengthMultiplier) || ThrowStrengthMultiplier <= 0.f
		|| !ItemThrowPhysics::CalculateImpulse(AdjustedSettings, Body->GetMass(), Direction, Info, ImpulseUE))
	{
		return false;
	}
	// 접촉을 이유로 클릭을 취소하지 않는다. 지형 충돌은 유지하고 자기 몸 충돌만 유예한다.
	TGuardValue<bool> TransitionGuard(bCarryTransition, true);
	ReleaseHeldObject(ECarryReleaseReason::Throw);
	// 해제 콜백에서 아이템을 파괴하거나 물리를 껐다면 던지기 성공 알림을 보내지 않는다.
	if (!IsValid(Body) || !IsValid(Item) || !Body->IsSimulatingPhysics())
	{
		return false;
	}
	// 물리 핸들이 고개 움직임을 따라오며 만든 속도를 발사에 섞지 않는다.
	// 해제 콜백까지 마친 다음 초기화하므로 같은 질량과 조준에는 같은 발사 속도가 나온다.
	Body->SetPhysicsLinearVelocity(FVector::ZeroVector);
	Body->SetPhysicsAngularVelocityInRadians(FVector::ZeroVector);
	// 속도 초기화와 질량 무시는 별개다. false로 충격량을 적용하여 J/m의 질량 차이를 유지한다.
	Body->AddImpulse(ImpulseUE, NAME_None, false);
	Info.Thrower = GetOwner();
	if (Item->GetClass()->ImplementsInterface(UUsableItem::StaticClass()))
	{
		IUsableItem::Execute_OnItemThrown(Item, Info);
	}
	return true;
}

void UCarryComponent::ForceRelease()
{
	TGuardValue<bool> TransitionGuard(bCarryTransition, true);
	ReleaseHeldObject(ECarryReleaseReason::Forced);
}

void UCarryComponent::ReleaseHeldObject(ECarryReleaseReason Reason)
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
		if (Reason == ECarryReleaseReason::Throw)
		{
			// 발사 직후 몸에 부딪혀 멈추지 않도록 잠시 유예하고, 몸 밖으로 나온 뒤 복원한다.
			QueuePawnCollisionRestore(Body, PreviousPawnResponse);
		}
		else
		{
			Body->SetCollisionResponseToChannel(ECC_Pawn, PreviousPawnResponse);
		}
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
		Carryable->LastReleaseReason = Reason;
		Carryable->OnReleased(MakeContext());
		if (IsValid(Carryable))
		{
			Carryable->OnReleasedWithReason(MakeContext(), Reason);
		}
	}
	if (bHadState)
	{
		OnCarryStateChanged.Broadcast();
	}
}

void UCarryComponent::OnHeldActorDestroyed(AActor* DestroyedActor)
{
	ForceRelease();
}

void UCarryComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	// 외부 파괴·물리 해제·빙의 변경으로 운반이 불가능해지면 공통 경로로 강제 해제한다.
	const APawn* Pawn = Cast<APawn>(GetOwner());
	if (!IsHolding() || !PhysicsHandle || PhysicsHandle->GetGrabbedComponent() != HeldPrimitive.Get()
		|| !HeldPrimitive->IsSimulatingPhysics() || (Pawn && !Pawn->IsLocallyControlled()))
	{
		ForceRelease();
		return;
	}
	FVector ViewLocation;
	FRotator ViewRotation;
	InteractionView::GetViewPoint(GetOwner(), ViewLocation, ViewRotation);
	if (FVector::DistSquared(ViewLocation, HeldPrimitive->GetCenterOfMass()) > FMath::Square(MaxCarryDistance))
	{
		// 벽 뒤에 걸린 물건을 계속 끌거나 지나치게 멀리 운반하는 상황을 막는다.
		ForceRelease();
		return;
	}
	// 카메라 앞 목표 위치와 잡을 때의 상대 회전을 매 프레임 물리 핸들에 전달한다.
	PhysicsHandle->SetTargetLocationAndRotation(ViewLocation + ViewRotation.Vector() * HoldDistance,
		(ViewRotation.Quaternion() * RelativeHoldRotation).Rotator());
}
