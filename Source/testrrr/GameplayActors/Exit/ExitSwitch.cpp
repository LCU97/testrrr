#include "GameplayActors/Exit/ExitSwitch.h"

#include "GameplayActors/Exit/ExitDoor.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"

AExitSwitch::AExitSwitch()
{
	InteractionPrompt = NSLOCTEXT("Exit", "ActivateExit", "탈출 버튼 누르기");
	DisabledReason = NSLOCTEXT("Exit", "ExitUnavailable", "탈출구를 열 수 없습니다");
	Cooldown = 0.25f;
	// 다른 스위치와 같이 물리 아이템의 첫 오버랩으로도 작동한다.
	// 아래의 태그·속도·정면 판정은 별도의 보조 입력 경로로 유지한다.
	bPhysicalOverlapEnabled = true;
	Mesh->SetNotifyRigidBodyCollision(true);
	Mesh->OnComponentHit.AddDynamic(this, &AExitSwitch::OnTargetHit);
}

bool AExitSwitch::Activate_Implementation(const FInteractionContext& Context)
{
	return ActivateExit();
}

bool AExitSwitch::ActivateExit()
{
	if (!IsValid(ExitDoor))
	{
		return false;
	}
	ExitDoor->ActivateExit();
	OnExitActivated.Broadcast();
	return true;
}

void AExitSwitch::OnTargetHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
	FVector NormalImpulse, const FHitResult& Hit)
{
	if (!IsValid(OtherActor) || !IsValid(OtherComp) || !OtherComp->IsSimulatingPhysics()
		|| !OtherActor->ActorHasTag(TriggerableTag) || !GetWorld())
	{
		return;
	}

	const FVector Velocity = OtherComp->GetPhysicsLinearVelocity();
	if (Velocity.Size() < MinimumImpactSpeed)
	{
		return;
	}
	const FVector IncomingDirection = -Velocity.GetSafeNormal();
	if (FVector::DotProduct(IncomingDirection, GetActorForwardVector()) < MinimumFrontApproachDot)
	{
		return;
	}
	const double Now = GetWorld()->GetTimeSeconds();
	if (Now < LastPhysicalActivationTime + PhysicalActivationCooldown)
	{
		return;
	}
	if (ActivateExit())
	{
		LastPhysicalActivationTime = Now;
	}
}
