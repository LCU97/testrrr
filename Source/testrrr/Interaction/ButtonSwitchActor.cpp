/*
 * 스위치 공통 실행 흐름
 * 사용 가능 여부 확인 → 자식 Activate 호출 → 성공 시 쿨다운 적용 순서로 처리한다.
 * 이벤트에서 같은 스위치를 다시 호출해도 실행이 재진입하지 않게 보호한다.
 */

#include "Interaction/ButtonSwitchActor.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "Engine/World.h"
#include "Templates/UnrealTemplate.h"
#include "UObject/ConstructorHelpers.h"

AButtonSwitchActor::AButtonSwitchActor()
{
	// 월드 배치와 SpawnPoint는 항상 1배율 루트를 사용한다.
	SwitchRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SwitchRoot"));
	SetRootComponent(SwitchRoot);
	SwitchRoot->SetRelativeScale3D(FVector::OneVector);

	// 표시·충돌용 큐브만 축소한다. 따라서 자식 컴포넌트와 생성 액터는 축소되지 않는다.
	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(SwitchRoot);
	Mesh->SetCollisionProfileName(TEXT("BlockAllDynamic"));
	Mesh->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (Cube.Succeeded())
	{
		Mesh->SetStaticMesh(Cube.Object);
	}
	// 모든 스위치는 1m 기본 큐브를 25cm 정육면체 버튼으로 사용한다.
	Mesh->SetRelativeScale3D(FVector(0.25f));

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> RedButtonMaterial(
		TEXT("/Game/HHR/M_HHR_ButtonRed.M_HHR_ButtonRed"));
	if (RedButtonMaterial.Succeeded())
	{
		Mesh->SetMaterial(0, RedButtonMaterial.Object);
	}
	InteractionPrompt = NSLOCTEXT("Interaction", "Press", "누르기");
	DisabledReason = NSLOCTEXT("Interaction", "SwitchUnavailable", "지금은 사용할 수 없습니다");
}

bool AButtonSwitchActor::CanInteract_Implementation(const FInteractionContext& Context) const
{
	// 공통 활성화 조건, 실행 중 여부, 쿨다운을 모두 통과해야 한다.
	return Super::CanInteract_Implementation(Context) && !bActivating
		&& GetWorld() && GetWorld()->GetTimeSeconds() >= NextActivationTime;
}

FInteractionResult AButtonSwitchActor::Interact_Implementation(const FInteractionContext& Context)
{
	FInteractionResult Result;
	if (IInteractable::Execute_CanInteract(this, Context))
	{
		// 효과가 다른 이벤트를 거쳐 자신을 다시 호출해도 중복 실행하지 않는다.
		TGuardValue<bool> Guard(bActivating, true);
		// 여기만 자식별 효과로 바뀐다. 입력과 타이머 코드는 자식에서 구현하지 않는다.
		Result.bSucceeded = Activate(Context);
		if (Result.bSucceeded && IsValid(this))
		{
			// 성공한 경우에만 다음 사용 시간을 예약한다.
			NextActivationTime = GetWorld()->GetTimeSeconds() + FMath::Max(0.f, Cooldown);
		}
	}
	if (!Result.bSucceeded)
	{
		Result.FailureReason = DisabledReason;
	}
	return Result;
}

bool AButtonSwitchActor::Activate_Implementation(const FInteractionContext& Context)
{
	// 구체적인 효과가 없는 부모 구현은 실패를 반환한다.
	return false;
}
