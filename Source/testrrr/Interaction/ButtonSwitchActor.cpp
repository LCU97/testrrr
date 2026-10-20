/*
 * 스위치 공통 실행 흐름
 * 사용 가능 여부 확인 → 자식 Activate 호출 → 성공 시 쿨다운 적용 순서로 처리한다.
 * 이벤트에서 같은 스위치를 다시 호출해도 실행이 재진입하지 않게 보호한다.
 */

#include "Interaction/ButtonSwitchActor.h"
#include "Components/BoxComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Interaction/CarryableComponent.h"
#include "Materials/MaterialInterface.h"
#include "Engine/World.h"
#include "Templates/UnrealTemplate.h"
#include "UObject/ConstructorHelpers.h"

AButtonSwitchActor::AButtonSwitchActor()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;
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

	PhysicalTrigger = CreateDefaultSubobject<UBoxComponent>(TEXT("PhysicalTrigger"));
	PhysicalTrigger->SetupAttachment(SwitchRoot);
	PhysicalTrigger->SetBoxExtent(FVector(20.f));
	PhysicalTrigger->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	PhysicalTrigger->SetCollisionObjectType(ECC_WorldDynamic);
	PhysicalTrigger->SetCollisionResponseToAllChannels(ECR_Ignore);
	PhysicalTrigger->SetCollisionResponseToChannel(ECC_PhysicsBody, ECR_Overlap);
	PhysicalTrigger->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Overlap);
	PhysicalTrigger->SetGenerateOverlapEvents(true);
	PhysicalTrigger->OnComponentBeginOverlap.AddDynamic(this, &AButtonSwitchActor::OnPhysicalTriggerBeginOverlap);
	PhysicalTrigger->OnComponentEndOverlap.AddDynamic(this, &AButtonSwitchActor::OnPhysicalTriggerEndOverlap);

	InteractionPrompt = NSLOCTEXT("Interaction", "Press", "누르기");
	DisabledReason = NSLOCTEXT("Interaction", "SwitchUnavailable", "지금은 사용할 수 없습니다");
}

void AButtonSwitchActor::BeginPlay()
{
	Super::BeginPlay();
	// 인스턴스별 메시 위치를 보관해 BP에서 버튼 위치를 조정해도 그 위치로 정확히 돌아간다.
	ReleasedMeshLocation = Mesh->GetRelativeLocation();
}

void AButtonSwitchActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	UpdatePressAnimation();
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
			BeginPressAnimation();
		}
	}
	if (!Result.bSucceeded)
	{
		Result.FailureReason = DisabledReason;
	}
	return Result;
}

void AButtonSwitchActor::BeginPressAnimation()
{
	if (!GetWorld() || !Mesh)
	{
		return;
	}

	// 연속 입력은 쿨다운으로 막히지만, 물리 입력 등으로 재시작될 경우에도 현재 위치에서 자연스럽게 이어간다.
	PressStartTime = GetWorld()->GetTimeSeconds();
	ReleaseStartTime = NextActivationTime;
	bIsPressed = true;
	SetActorTickEnabled(true);
}

void AButtonSwitchActor::UpdatePressAnimation()
{
	if (!bIsPressed || !GetWorld() || !Mesh)
	{
		SetActorTickEnabled(false);
		return;
	}

	const double Now = GetWorld()->GetTimeSeconds();
	const FVector PressedLocation = ReleasedMeshLocation + FVector(-FMath::Max(0.f, PressDepth), 0.f, 0.f);
	FVector NewLocation = PressedLocation;
	if (Now < PressStartTime + PressInDuration)
	{
		const float Alpha = FMath::InterpEaseOut(0.f, 1.f,
			FMath::Clamp(static_cast<float>((Now - PressStartTime) / PressInDuration), 0.f, 1.f), 2.f);
		NewLocation = FMath::Lerp(ReleasedMeshLocation, PressedLocation, Alpha);
	}
	else if (Now >= ReleaseStartTime)
	{
		const float Alpha = FMath::InterpEaseIn(0.f, 1.f,
			FMath::Clamp(static_cast<float>((Now - ReleaseStartTime) / ReleaseDuration), 0.f, 1.f), 2.f);
		NewLocation = FMath::Lerp(PressedLocation, ReleasedMeshLocation, Alpha);
		if (Alpha >= 1.f)
		{
			bIsPressed = false;
			SetActorTickEnabled(false);
		}
	}
	Mesh->SetRelativeLocation(NewLocation);
}

bool AButtonSwitchActor::Activate_Implementation(const FInteractionContext& Context)
{
	// 구체적인 효과가 없는 부모 구현은 실패를 반환한다.
	return false;
}

void AButtonSwitchActor::OnPhysicalTriggerBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	AInteractableActor* Interactable = Cast<AInteractableActor>(OtherActor);
	if (!bPhysicalOverlapEnabled || !IsValid(Interactable) || OtherActor == this)
	{
		return;
	}

	const TWeakObjectPtr<AActor> OverlappingActor(OtherActor);
	int32& OverlapCount = PhysicalOverlapCounts.FindOrAdd(OverlappingActor);
	const bool bFirstOverlap = OverlapCount++ == 0;
	if (!bFirstOverlap)
	{
		return;
	}

	// 들고 있는 아이템은 충돌 입력이 아니다. 트리거 밖으로 완전히 나갔다 들어와야 다시 검사한다.
	const UCarryableComponent* Carryable = Interactable->FindComponentByClass<UCarryableComponent>();
	if ((bRequireSimulatingPhysics && (!IsValid(OtherComp) || !OtherComp->IsSimulatingPhysics()))
		|| (Carryable && Carryable->CurrentCarrier.IsValid()))
	{
		return;
	}

	FInteractionContext Context;
	Context.Interactor = Interactable;
	Context.Hit = SweepResult;
	IInteractable::Execute_Interact(this, Context);
}

void AButtonSwitchActor::OnPhysicalTriggerEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (!IsValid(OtherActor))
	{
		return;
	}

	const TWeakObjectPtr<AActor> OverlappingActor(OtherActor);
	if (int32* OverlapCount = PhysicalOverlapCounts.Find(OverlappingActor))
	{
		if (--(*OverlapCount) <= 0)
		{
			PhysicalOverlapCounts.Remove(OverlappingActor);
		}
	}
}
