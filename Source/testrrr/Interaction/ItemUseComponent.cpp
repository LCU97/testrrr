/*
 * 아이템 사용 세션 구현
 * 입력 시작 시 대상을 고정하여 종료·취소 알림이 다른 아이템에 전달되지 않게 한다.
 * 던지기 시작 중 운반이 풀리는 경우에도 단발 성공과 지속 사용 중단을 구별한다.
 */

#include "Interaction/ItemUseComponent.h"
#include "Interaction/CarryComponent.h"
#include "Interaction/ItemUseBehavior.h"
#include "Interaction/UsableItem.h"
#include "Interaction/InteractionView.h"
#include "GameFramework/Pawn.h"

UItemUseComponent::UItemUseComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UItemUseComponent::BeginPlay()
{
	Super::BeginPlay();
	Carry = GetOwner()->FindComponentByClass<UCarryComponent>();
	if (Carry)
	{
		Carry->OnCarryStateChanged.AddDynamic(this, &UItemUseComponent::HandleCarryStateChanged);
	}
}

void UItemUseComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	CancelUseInput();
	if (Carry)
	{
		Carry->OnCarryStateChanged.RemoveDynamic(this, &UItemUseComponent::HandleCarryStateChanged);
	}
	Super::EndPlay(EndPlayReason);
}

bool UItemUseComponent::IsLocalUser() const
{
	const APawn* Pawn = Cast<APawn>(GetOwner());
	return !Pawn || Pawn->IsLocallyControlled();
}

bool UItemUseComponent::IsUsing() const
{
	return IsValid(ActiveBehavior);
}

FItemUseContext UItemUseComponent::MakeContext(AActor* Item) const
{
	FItemUseContext Context;
	Context.User = GetOwner();
	Context.Item = Item;
	Context.Carrier = Carry;
	FRotator ViewRotation;
	InteractionView::GetViewPoint(GetOwner(), Context.ViewOrigin, ViewRotation);
	Context.AimDirection = ViewRotation.Vector();
	return Context;
}

FItemUseResult UItemUseComponent::BeginUseInput()
{
	FItemUseResult Result;
	// 같은 누름에서 재호출되거나 물건을 바꿔 잡아도 새 사용을 시작하지 않는다.
	if (bInputHeld || IsUsing() || !IsLocalUser())
	{
		return Result;
	}
	bInputHeld = true;
	AActor* Item = Carry ? Carry->GetHeldActor() : nullptr;
	if (!IsValid(Item) || !Item->GetClass()->ImplementsInterface(UUsableItem::StaticClass()))
	{
		return Result;
	}
	const FItemUseContext Context = MakeContext(Item);
	UItemUseBehavior* Behavior = IUsableItem::Execute_GetItemUseBehavior(Item);
	if (!IsValid(Behavior) || Behavior->GetTypedOuter<AActor>() != Item
		|| !IUsableItem::Execute_CanUseItem(Item, Context) || !Behavior->CanUse(Context))
	{
		return Result;
	}

	ActiveItem = Item;
	ActiveBehavior = Behavior;
	ActiveContext = Context;
	bCancelRequested = false;
	bBeginningUse = true;
	Result = Behavior->BeginUse(Context);
	bBeginningUse = false;
	if (Result.bSucceeded && !Result.bKeepUsing)
	{
		// 던지기처럼 즉시 완료된 동작에는 End/Cancel을 중복 통지하지 않는다.
		ClearSession();
	}
	else if (Result.bSucceeded && !bCancelRequested && IsValid(Item) && Carry->GetHeldActor() == Item)
	{
		SetComponentTickEnabled(true);
	}
	else
	{
		// 시작 도중 내려놓기·파괴가 일어났거나 준비에 실패한 지속 효과도 정리한다.
		FinishSession(true);
		Result.bSucceeded = false;
		Result.bKeepUsing = false;
	}
	OnUseStateChanged.Broadcast();
	return Result;
}

void UItemUseComponent::EndUseInput()
{
	bInputHeld = false;
	if (bBeginningUse)
	{
		bCancelRequested = true;
		return;
	}
	FinishSession(false);
}

void UItemUseComponent::CancelUseInput()
{
	bInputHeld = false;
	CancelUse();
}

void UItemUseComponent::CancelUse()
{
	if (bBeginningUse)
	{
		// BeginUse 안의 운반 해제 콜백에서 전략을 재진입 호출하지 않는다.
		bCancelRequested = true;
		return;
	}
	FinishSession(true);
}

void UItemUseComponent::ClearSession()
{
	ActiveBehavior = nullptr;
	ActiveItem.Reset();
	ActiveContext = FItemUseContext();
	bCancelRequested = false;
	SetComponentTickEnabled(false);
}

void UItemUseComponent::FinishSession(bool bCanceled)
{
	UItemUseBehavior* Behavior = ActiveBehavior;
	const FItemUseContext Context = ActiveContext;
	// 종료 효과가 다른 입력을 호출하더라도 이전 세션은 이미 끝난 상태로 보이게 한다.
	ClearSession();
	if (IsValid(Behavior))
	{
		if (bCanceled)
		{
			Behavior->CancelUse(Context);
		}
		else
		{
			Behavior->EndUse(Context);
		}
		OnUseStateChanged.Broadcast();
	}
}

void UItemUseComponent::HandleCarryStateChanged()
{
	if (IsUsing() && (!Carry || Carry->GetHeldActor() != ActiveItem.Get() || !ActiveItem.IsValid()))
	{
		CancelUse();
	}
}

void UItemUseComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	AActor* Item = ActiveItem.Get();
	if (!IsUsing() || !IsLocalUser() || !IsValid(Item) || !Carry || Carry->GetHeldActor() != Item)
	{
		CancelUse();
		return;
	}
	ActiveContext = MakeContext(Item);
	if (!IUsableItem::Execute_CanUseItem(Item, ActiveContext) || !ActiveBehavior->CanUse(ActiveContext))
	{
		CancelUse();
		return;
	}
	// 지속 사용만 Tick을 켜므로 단발 던지기에는 유지 비용과 반복 효과가 발생하지 않는다.
	ActiveBehavior->UpdateUse(ActiveContext, DeltaTime);
}

FText UItemUseComponent::GetUsePrompt() const
{
	AActor* Item = Carry ? Carry->GetHeldActor() : nullptr;
	if (!IsLocalUser() || !IsValid(Item) || !Item->GetClass()->ImplementsInterface(UUsableItem::StaticClass()))
	{
		return FText::GetEmpty();
	}
	const FItemUseContext Context = MakeContext(Item);
	UItemUseBehavior* Behavior = IUsableItem::Execute_GetItemUseBehavior(Item);
	return IsValid(Behavior) && IUsableItem::Execute_CanUseItem(Item, Context) && Behavior->CanUse(Context)
		? Behavior->UsePrompt : FText::GetEmpty();
}

void UItemUseComponent::NotifyUseAvailabilityChanged()
{
	// 실제 허용 여부는 UI와 실행 시점의 조회에서 계산하므로 상태 플래그를 중복 저장하지 않는다.
	OnUseStateChanged.Broadcast();
}
