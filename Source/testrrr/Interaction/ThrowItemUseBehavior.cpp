/*
 * 단발 던지기 사용 동작 구현
 * 클릭 시작 시 한 번만 요청하고, 길게 누르는 동안 반복해서 던지지 않는다.
 * 실제 운반 중인 아이템이 문맥의 아이템과 일치하는지도 확인한다.
 */

#include "Interaction/ThrowItemUseBehavior.h"
#include "Interaction/CarryComponent.h"

UThrowItemUseBehavior::UThrowItemUseBehavior()
{
	UsePrompt = NSLOCTEXT("ItemUse", "Throw", "던지기");
}

bool UThrowItemUseBehavior::CanUse_Implementation(const FItemUseContext& Context) const
{
	return Super::CanUse_Implementation(Context) && Context.Carrier->GetOwner() == Context.User
		&& Context.Carrier->GetHeldActor() == Context.Item && Context.Carrier->CanThrowHeldObject();
}

FItemUseResult UThrowItemUseBehavior::BeginUse_Implementation(const FItemUseContext& Context)
{
	FItemUseResult Result;
	if (CanUse(Context))
	{
		// 프레임워크의 물리 적용 경로를 사용해 질량 무시나 중복 해제를 방지한다.
		Result.bSucceeded = Context.Carrier->TryThrow(ThrowSettings, Context.AimDirection);
	}
	if (!Result.bSucceeded)
	{
		Result.FailureReason = NSLOCTEXT("ItemUse", "CannotThrow", "지금은 던질 수 없습니다");
	}
	return Result;
}
