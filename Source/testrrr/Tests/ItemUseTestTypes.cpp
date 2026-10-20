/*
 * 자동 테스트 관찰 객체 구현
 * 효과 대신 호출 횟수와 전달된 문맥을 기록하여 프레임워크의 수명주기를 확인한다.
 * 시작 콜백 안에서 내려놓는 경우도 만들어 재진입과 중단 처리를 검증한다.
 */

#include "Tests/ItemUseTestTypes.h"
#include "Interaction/CarryComponent.h"
#include "Interaction/InteractionComponent.h"
#include "Camera/CameraComponent.h"

AInteractionTestCharacter::AInteractionTestCharacter()
{
	// 테스트에는 캐릭터 메시 에셋이 없으므로 카메라를 head 소켓 대신 루트에 붙인다.
	GetFirstPersonCameraComponent()->SetupAttachment(GetRootComponent());
	InteractionComponent->bShowDefaultPrompt = false;
}

FItemUseResult UItemUseTestBehavior::BeginUse_Implementation(const FItemUseContext& Context)
{
	++BeginCount;
	if (bDropDuringBegin)
	{
		Context.Carrier->TryDrop();
	}
	FItemUseResult Result;
	Result.bSucceeded = !bFailBegin;
	Result.bKeepUsing = bContinuous;
	return Result;
}

void UItemUseTestBehavior::UpdateUse_Implementation(const FItemUseContext& Context, float DeltaTime)
{
	++UpdateCount;
	LastDirection = Context.AimDirection;
}

void UItemUseTestBehavior::EndUse_Implementation(const FItemUseContext& Context)
{
	++EndCount;
}

void UItemUseTestBehavior::CancelUse_Implementation(const FItemUseContext& Context)
{
	++CancelCount;
	// 기본 취소가 자식의 EndUse까지 호출하여 종료 정리를 공유하는지도 확인한다.
	Super::CancelUse_Implementation(Context);
}

void AItemUseTestPickupActor::OnItemThrown_Implementation(const FItemThrowInfo& Info)
{
	++ThrownCount;
	LastThrow = Info;
}
