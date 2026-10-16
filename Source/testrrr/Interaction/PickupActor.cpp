/*
 * 아이템 액터의 상호작용 구현
 * 물건 쪽에서는 잡기 허용 여부를 판단하고, 실제 운반은 플레이어의 CarryComponent에 요청한다.
 * 기본 큐브는 배치·검증용이며 자식 액터에서 교체할 수 있다.
 */

#include "Interaction/PickupActor.h"
#include "Interaction/CarryableComponent.h"
#include "Interaction/CarryComponent.h"
#include "Interaction/ItemUseBehavior.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"

APickupActor::APickupActor()
{
	// 메쉬를 루트로 사용하여 물리 시뮬레이션과 액터 위치가 함께 움직이게 한다.
	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	SetRootComponent(Mesh);
	Mesh->SetCollisionProfileName(TEXT("PhysicsActor"));
	// 물리 충돌 설정과 별도로 시선 탐색의 Visibility 채널도 막아야 선택된다.
	Mesh->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	Mesh->SetSimulatePhysics(true);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (Cube.Succeeded())
	{
		Mesh->SetStaticMesh(Cube.Object);
	}
	// 여러 메쉬가 있는 자식은 CarryPrimitive를 실제로 운반할 물리 부위로 바꾼다.
	Carryable = CreateDefaultSubobject<UCarryableComponent>(TEXT("Carryable"));
	Carryable->CarryPrimitive = Mesh;
	InteractionPrompt = NSLOCTEXT("Interaction", "Grab", "잡기");
	DisabledReason = NSLOCTEXT("Interaction", "CannotGrab", "지금은 잡을 수 없습니다");
}

bool APickupActor::CanInteract_Implementation(const FInteractionContext& Context) const
{
	if (!Super::CanInteract_Implementation(Context))
	{
		return false;
	}
	// 플레이어가 빈손인지와 물건 자체의 운반 조건을 함께 확인한다.
	const UCarryComponent* Carrier = Context.Interactor->FindComponentByClass<UCarryComponent>();
	return Carrier && !Carrier->IsHolding() && Carryable->CanGrab(Context);
}

FInteractionResult APickupActor::Interact_Implementation(const FInteractionContext& Context)
{
	FInteractionResult Result;
	// BP에서 재정의한 조건도 반영하고, 상태 기록은 잡기 성공 후 운반 컴포넌트가 수행한다.
	if (IInteractable::Execute_CanInteract(this, Context))
	{
		if (UCarryComponent* Carrier = Context.Interactor->FindComponentByClass<UCarryComponent>())
		{
			Result.bSucceeded = Carrier->TryGrab(Carryable, Context);
		}
	}
	if (!Result.bSucceeded)
	{
		Result.FailureReason = DisabledReason;
	}
	return Result;
}

UItemUseBehavior* APickupActor::GetItemUseBehavior_Implementation() const
{
	return UseBehavior;
}

bool APickupActor::CanUseItem_Implementation(const FItemUseContext& Context) const
{
	// 사용 입력은 현재 이 물건을 들고 있는 주체만 보낼 수 있다.
	return bItemUseEnabled && IsValid(UseBehavior) && Context.Item == this && IsValid(Context.User)
		&& IsValid(Context.Carrier) && Context.Carrier->GetOwner() == Context.User
		&& Context.Carrier->GetHeldActor() == this;
}

void APickupActor::OnItemThrown_Implementation(const FItemThrowInfo& Info)
{
	// 기본 물건은 날아가기만 한다. 폭탄 등은 자식 C++/BP에서 자신의 후속 기능을 구현한다.
}
