/*
 * 상호작용 공통 액터 구현
 * 공통 안내와 활성화 조건만 제공하며, 구체적인 효과는 자식 액터가 구현한다.
 * 아무 동작도 없는 기본 Interact는 실패를 반환한다.
 */

#include "Interaction/InteractableActor.h"

AInteractableActor::AInteractableActor()
{
	PrimaryActorTick.bCanEverTick = false;
	InteractionPrompt = NSLOCTEXT("Interaction", "Interact", "Interact");
	DisabledReason = NSLOCTEXT("Interaction", "Unavailable", "Unavailable");
}

FInteractionInfo AInteractableActor::GetInteractionInfo_Implementation(const FInteractionContext& Context) const
{
	// 액터에 설정된 문구만 반환하고 게임 상태는 변경하지 않는다.
	FInteractionInfo Info;
	Info.Prompt = InteractionPrompt;
	Info.UnavailableReason = DisabledReason;
	return Info;
}

bool AInteractableActor::CanInteract_Implementation(const FInteractionContext& Context) const
{
	// 추가 조건이 필요한 자식은 이 공통 조건과 자신의 조건을 함께 검사한다.
	return bInteractionEnabled && IsValid(Context.Interactor);
}

FInteractionResult AInteractableActor::Interact_Implementation(const FInteractionContext& Context)
{
	// 실제 기능이 없는 부모 구현은 성공으로 보고하지 않는다.
	return FInteractionResult();
}
