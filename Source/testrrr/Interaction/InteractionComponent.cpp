/*
 * 상호작용 탐색·실행·안내 구현
 * 정기 탐색은 안내용이며, E를 누르면 현재 시선으로 다시 검사한 대상에만 요청한다.
 * 운반 상태 변경 이벤트를 받아 탐색 타이머와 E/G 안내를 즉시 전환한다.
 */

#include "Interaction/InteractionComponent.h"
#include "Interaction/Interactable.h"
#include "Interaction/CarryComponent.h"
#include "Interaction/ItemUseComponent.h"
#include "Interaction/InteractionPromptWidget.h"
#include "Interaction/InteractionView.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "TimerManager.h"
#include "DrawDebugHelpers.h"

UInteractionComponent::UInteractionComponent()
{
	// 매 프레임 탐색하지 않고 반복 타이머를 사용한다.
	PrimaryComponentTick.bCanEverTick = false;
	PromptWidgetClass = UInteractionPromptWidget::StaticClass();
}

void UInteractionComponent::BeginPlay()
{
	Super::BeginPlay();
	// 운반 상태는 CarryComponent에서 조회하고 변경 이벤트로 탐색을 켜거나 끈다.
	Carry = GetOwner()->FindComponentByClass<UCarryComponent>();
	ItemUse = GetOwner()->FindComponentByClass<UItemUseComponent>();
	if (ItemUse)
	{
		ItemUse->OnUseStateChanged.AddDynamic(this, &UInteractionComponent::HandleUseStateChanged);
	}
	if (Carry)
	{
		Carry->OnCarryStateChanged.AddDynamic(this, &UInteractionComponent::HandleCarryStateChanged);
	}
	HandleCarryStateChanged();
}

void UInteractionComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// 종료 후 타이머·이벤트가 이 컴포넌트를 다시 호출하지 않도록 해제한다.
	GetWorld()->GetTimerManager().ClearTimer(TraceTimer);
	if (ItemUse)
	{
		ItemUse->OnUseStateChanged.RemoveDynamic(this, &UInteractionComponent::HandleUseStateChanged);
	}
	if (Carry)
	{
		Carry->OnCarryStateChanged.RemoveDynamic(this, &UInteractionComponent::HandleCarryStateChanged);
	}
	if (PromptWidget)
	{
		PromptWidget->RemoveFromParent();
	}
	Super::EndPlay(EndPlayReason);
}

bool UInteractionComponent::CanInteractNow() const
{
	const APawn* Pawn = Cast<APawn>(GetOwner());
	// 물건을 들고 있으면 E 요청 전체를 차단한다. 플레이어 Pawn은 로컬 입력만 처리한다.
	return bInteractionEnabled && (!Carry || !Carry->IsHolding()) && (!Pawn || Pawn->IsLocallyControlled());
}

bool UInteractionComponent::FindTarget(FInteractionContext& OutContext) const
{
	if (!CanInteractNow())
	{
		return false;
	}
	OutContext.Interactor = GetOwner();
	FVector ViewLocation;
	FRotator ViewRotation;
	InteractionView::GetViewPoint(GetOwner(), ViewLocation, ViewRotation);
	// 자기 자신은 무시하고 실제 카메라 중앙 방향으로 단순 충돌을 검사한다.
	FCollisionQueryParams Params(SCENE_QUERY_STAT(InteractionTrace), false, GetOwner());
	// 첫 번째 가로막는 충돌만 사용하므로 일반 벽 뒤의 상호작용 대상으로 넘어가지 않는다.
	const FVector TraceEnd = ViewLocation + ViewRotation.Vector() * FMath::Max(1.f, InteractionDistance);
	const float Radius = FMath::Max(0.1f, TraceRadius);
	const bool bHit = GetWorld()->SweepSingleByChannel(OutContext.Hit, ViewLocation, TraceEnd,
		FQuat::Identity, TraceChannel, FCollisionShape::MakeSphere(Radius), Params);
	AActor* Target = OutContext.Hit.GetActor();
	// 메쉬 이름으로 종류를 판별하지 않고, 맞은 액터가 공통 계약을 지원하는지 확인한다.
	const bool bFoundTarget = bHit && IsValid(Target) && Target->GetClass()->ImplementsInterface(UInteractable::StaticClass());
#if ENABLE_DRAW_DEBUG
	if (bDrawDebugTrace)
	{
		const FColor Color = bFoundTarget ? FColor::Green : FColor::Red;
		// 스피어가 지나간 전체 부피를 캡슐로 표시한다. Hit.Location은 충돌 시 구 중심이다.
		const FVector Path = TraceEnd - ViewLocation;
		const float Lifetime = FMath::Max(0.01f, TraceInterval) + 0.02f;
		DrawDebugCapsule(GetWorld(), (ViewLocation + TraceEnd) * 0.5f,
			Path.Size() * 0.5f + Radius, Radius,
			FQuat::FindBetweenNormals(FVector::UpVector, Path.GetSafeNormal()), Color, false, Lifetime, 0, 0.8f);
		if (bHit)
		{
			DrawDebugSphere(GetWorld(), OutContext.Hit.Location, Radius, 16, Color, false, Lifetime);
			DrawDebugPoint(GetWorld(), OutContext.Hit.ImpactPoint, 10.f, FColor::Yellow, false, Lifetime);
		}
	}
#endif
	return bFoundTarget;
}

FInteractionResult UInteractionComponent::TryInteract()
{
	FInteractionResult Result;
	FInteractionContext Context;
	// 0.2초 전 대상을 그대로 실행하지 않고 E를 누른 순간의 시선으로 재검사한다.
	if (FindTarget(Context))
	{
		AActor* Target = Context.Hit.GetActor();
		// BP에서 재정의한 조건과 실행도 반영하도록 인터페이스의 Execute_ 경로를 사용한다.
		if (IInteractable::Execute_CanInteract(Target, Context) && IsValid(Target))
		{
			Result = IInteractable::Execute_Interact(Target, Context);
		}
		else if (IsValid(Target))
		{
			Result.FailureReason = IInteractable::Execute_GetInteractionInfo(Target, Context).UnavailableReason;
		}
	}
	// 성공 여부를 외부에 알리고, 실행으로 달라진 상태를 안내에도 즉시 반영한다.
	OnInteractionFinished.Broadcast(Result);
	RefreshTarget();
	return Result;
}

void UInteractionComponent::HandleCarryStateChanged()
{
	// 운반 중에는 탐색을 멈추고, 내려놓기·파괴 해제 후에는 반복 탐색을 재개한다.
	GetWorld()->GetTimerManager().ClearTimer(TraceTimer);
	if (!Carry || !Carry->IsHolding())
	{
		GetWorld()->GetTimerManager().SetTimer(TraceTimer, this, &UInteractionComponent::RefreshTarget,
			FMath::Max(0.01f, TraceInterval), true);
	}
	RefreshTarget();
}

void UInteractionComponent::RefreshTarget()
{
	// 지난 대상을 먼저 비워 시선 밖으로 나간 액터가 안내에 남지 않게 한다.
	FocusedActor.Reset();
	const APawn* Pawn = Cast<APawn>(GetOwner());
	if (Pawn && !Pawn->IsLocallyControlled())
	{
		SetPrompt(FText::GetEmpty(), false);
		return;
	}
	// 운반 중 E는 계속 차단하되, 아이템에 설정된 사용 동작과 G 내려놓기 안내를 표시한다.
	if (Carry && Carry->IsHolding())
	{
		FText DropPrompt = FText::Format(NSLOCTEXT("Interaction", "DropHint", "[{0}] 내려놓기"), DropInputLabel);
		if (Carry->CanRotateHeldObject())
		{
			DropPrompt = FText::Format(NSLOCTEXT("Interaction", "MouseRotateAndDropHint", "[{0}] + 마우스 회전   ·   {1}"), RotateInputLabel, DropPrompt);
		}
		const FText UsePrompt = ItemUse ? ItemUse->GetUsePrompt() : FText::GetEmpty();
		SetPrompt(UsePrompt.IsEmpty() ? DropPrompt
			: FText::Format(NSLOCTEXT("ItemUse", "UseAndDropHint", "[{0}] {1}   ·   {2}"), UseInputLabel, UsePrompt, DropPrompt), true);
		return;
	}
	FInteractionContext Context;
	if (!FindTarget(Context))
	{
		SetPrompt(FText::GetEmpty(), false);
		return;
	}
	AActor* Target = Context.Hit.GetActor();
	FocusedActor = Target;
	// 안내 문구와 실행 가능 여부를 각각 대상에게 조회한다. 조회 함수는 상태를 바꾸지 않는다.
	const FInteractionInfo Info = IInteractable::Execute_GetInteractionInfo(Target, Context);
	const bool bAvailable = IsValid(Target) && IInteractable::Execute_CanInteract(Target, Context);
	SetPrompt(bAvailable ? FText::Format(NSLOCTEXT("Interaction", "InteractHint", "[{0}] {1}"), InteractInputLabel, Info.Prompt)
		: Info.UnavailableReason, bAvailable);
}

void UInteractionComponent::HandleUseStateChanged()
{
	RefreshTarget();
}

void UInteractionComponent::SetPrompt(const FText& Prompt, bool bAvailable)
{
	// BeginPlay 뒤에 컨트롤러가 연결되는 경우도 지원하도록 로컬 플레이어 준비 후 생성한다.
	if (!PromptWidget && bShowDefaultPrompt && PromptWidgetClass)
	{
		const APawn* Pawn = Cast<APawn>(GetOwner());
		APlayerController* PC = Pawn ? Cast<APlayerController>(Pawn->GetController()) : nullptr;
		if (PC && PC->IsLocalController())
		{
			PromptWidget = CreateWidget<UInteractionPromptWidget>(PC, PromptWidgetClass);
			if (PromptWidget)
			{
				PromptWidget->AddToPlayerScreen();
			}
		}
	}
	if (PromptWidget)
	{
		PromptWidget->SetPrompt(Prompt, bAvailable);
	}
	// 문구 또는 사용 가능 여부가 달라졌을 때만 외부 UI에 변경 이벤트를 보낸다.
	if (!LastPrompt.EqualTo(Prompt) || bLastAvailable != bAvailable)
	{
		LastPrompt = Prompt;
		bLastAvailable = bAvailable;
		OnPromptChanged.Broadcast(Prompt, bAvailable);
	}
}
