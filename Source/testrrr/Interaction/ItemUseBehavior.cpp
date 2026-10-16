/*
 * 사용 동작의 기본 구현
 * 빈 동작이 사용 성공으로 처리되지 않도록 BeginUse는 기본 실패를 반환한다.
 * 취소를 별도로 구현하지 않은 자식도 정상 종료 정리 코드를 공유할 수 있게 한다.
 */

#include "Interaction/ItemUseBehavior.h"
#include "Interaction/CarryComponent.h"
#include "GameFramework/Actor.h"

UWorld* UItemUseBehavior::GetWorld() const
{
	// 클래스 기본 객체는 실제 게임 월드가 없으며 편집기에서 조회될 수도 있다.
	if (IsTemplate())
	{
		return nullptr;
	}
	const AActor* Owner = GetTypedOuter<AActor>();
	return Owner ? Owner->GetWorld() : nullptr;
}

bool UItemUseBehavior::CanUse_Implementation(const FItemUseContext& Context) const
{
	return IsValid(Context.User) && IsValid(Context.Item) && IsValid(Context.Carrier);
}

FItemUseResult UItemUseBehavior::BeginUse_Implementation(const FItemUseContext& Context)
{
	return FItemUseResult();
}

void UItemUseBehavior::UpdateUse_Implementation(const FItemUseContext& Context, float DeltaTime) {}
void UItemUseBehavior::EndUse_Implementation(const FItemUseContext& Context) {}

void UItemUseBehavior::CancelUse_Implementation(const FItemUseContext& Context)
{
	// 분사 종료 등 공통 정리를 재사용한다. 취소만의 효과가 필요하면 자식에서 재정의한다.
	EndUse(Context);
}
