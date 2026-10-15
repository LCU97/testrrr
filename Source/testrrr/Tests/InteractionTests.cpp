/*
 * 상호작용 프레임워크 자동 검증
 * 임시 월드에서 탐색 주기·최신 조준·차폐·거리·운반 중 차단·내려놓기·파괴 복구를 확인한다.
 * 실제 맵을 변경하지 않으며 테스트 월드는 종료 시 정리한다.
 */

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Components/BoxComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Interaction/InteractionComponent.h"
#include "Interaction/CarryComponent.h"
#include "Interaction/CarryableComponent.h"
#include "Interaction/PickupActor.h"
#include "Interaction/ToggleSwitchActor.h"

namespace InteractionTests
{
	// 각 테스트가 독립된 월드와 플레이어 컴포넌트를 사용하도록 준비·종료를 묶는다.
	struct FWorldFixture
	{
		UWorld* World;
		AActor* Player;
		UCarryComponent* Carry;
		UInteractionComponent* Interaction;

		FWorldFixture()
		{
			const UWorld::InitializationValues InitValues = UWorld::InitializationValues()
				.AllowAudioPlayback(false).RequiresHitProxies(false).CreatePhysicsScene(true)
				.CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(true).SetTransactional(false);
			// CreateWorld가 초기화도 수행하므로 별도로 InitializeNewWorld를 중복 호출하지 않는다.
			World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true, ERHIFeatureLevel::Num, &InitValues);
			GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
			Player = World->SpawnActor<AActor>();
			// 기본 Actor도 회전할 수 있도록 루트를 만들어 조준 방향 변경을 검증한다.
			USceneComponent* Root = NewObject<USceneComponent>(Player);
			Player->SetRootComponent(Root);
			Player->AddInstanceComponent(Root);
			Root->RegisterComponent();
			Carry = NewObject<UCarryComponent>(Player);
			Player->AddInstanceComponent(Carry);
			Carry->RegisterComponent();
			Interaction = NewObject<UInteractionComponent>(Player);
			Interaction->bShowDefaultPrompt = false;
			Player->AddInstanceComponent(Interaction);
			Interaction->RegisterComponent();
			World->InitializeActorsForPlay(FURL());
			World->SetBegunPlay(true);
			Player->DispatchBeginPlay();
			Tick(0.001f);
		}

		void Tick(float DeltaTime)
		{
			// TimerManager는 프레임당 한 번만 실행되므로 임시 월드의 프레임도 전진시킨다.
			++GFrameCounter;
			World->Tick(LEVELTICK_All, DeltaTime);
		}

		~FWorldFixture()
		{
			// 컴포넌트 EndPlay와 이벤트 정리가 실행된 후 월드와 엔진 문맥을 제거한다.
			World->EndPlay(EEndPlayReason::Quit);
			World->DestroyWorld(false);
			GEngine->DestroyWorldContext(World);
		}

		template<class T>
		T* Spawn(const FVector& Location)
		{
			FActorSpawnParameters Params;
			Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			return World->SpawnActor<T>(Location, FRotator::ZeroRotator, Params);
		}
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FInteractionTraceTest, "HundredButtons.Interaction.TraceAndFreshTarget",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FInteractionTraceTest::RunTest(const FString& Parameters)
{
	InteractionTests::FWorldFixture F;
	AToggleSwitchActor* Front = F.Spawn<AToggleSwitchActor>(FVector(200, 0, 0));
	AToggleSwitchActor* Side = F.Spawn<AToggleSwitchActor>(FVector(0, 200, 0));
	// 0.2초 이전에는 대상을 갱신하지 않고, 주기를 넘기면 탐색하는지 확인한다.
	F.Tick(0.1f);
	TestNull(TEXT("Discovery waits for the 0.2-second timer"), F.Interaction->GetFocusedActor());
	F.Tick(0.11f);
	TestEqual(TEXT("Timer finds the forward switch"), F.Interaction->GetFocusedActor(), static_cast<AActor*>(Front));
	// 탐색 캐시와 현재 조준이 달라지는 상황을 만들어 E 처리의 재검사를 확인한다.
	F.Player->SetActorRotation(FRotator(0, 90, 0));
	TestTrue(TEXT("E re-traces before the next timer update"), F.Interaction->TryInteract().bSucceeded);
	TestTrue(TEXT("Newly aimed switch activated"), Side->bIsOn);
	TestFalse(TEXT("Cached switch was not activated"), Front->bIsOn);
	TestFalse(TEXT("Cooldown prevents duplicate activation"), F.Interaction->TryInteract().bSucceeded);
	Side->bInteractionEnabled = false;
	F.Tick(0.3f);
	TestFalse(TEXT("Disabled switch is rejected"), F.Interaction->TryInteract().bSucceeded);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FInteractionOcclusionTest, "HundredButtons.Interaction.OcclusionAndRange",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FInteractionOcclusionTest::RunTest(const FString& Parameters)
{
	InteractionTests::FWorldFixture F;
	AToggleSwitchActor* Target = F.Spawn<AToggleSwitchActor>(FVector(200, 0, 0));
	// 인터페이스가 없는 일반 벽도 앞을 막으면 뒤 스위치에 도달하지 않아야 한다.
	AActor* Wall = F.Spawn<AActor>(FVector(100, 0, 0));
	UBoxComponent* Box = NewObject<UBoxComponent>(Wall);
	Wall->SetRootComponent(Box);
	Box->SetBoxExtent(FVector(10, 100, 100));
	Box->SetCollisionProfileName(TEXT("BlockAll"));
	Box->RegisterComponent();
	Box->SetWorldLocation(FVector(100, 0, 0));
	TestFalse(TEXT("Non-interactable wall blocks the switch"), F.Interaction->TryInteract().bSucceeded);
	TestFalse(TEXT("Occluded switch stays off"), Target->bIsOn);
	// 벽의 충돌을 끈 뒤에도 사거리 밖이면 실행되지 않는지 독립적으로 확인한다.
	Box->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Target->SetActorLocation(FVector(500, 0, 0));
	TestFalse(TEXT("Out-of-range switch is rejected"), F.Interaction->TryInteract().bSucceeded);
	Target->SetActorLocation(FVector(200, 0, 0));
	TestTrue(TEXT("Visible in-range switch works"), F.Interaction->TryInteract().bSucceeded);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FInteractionCarryTest, "HundredButtons.Interaction.CarryDropAndDestruction",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FInteractionCarryTest::RunTest(const FString& Parameters)
{
	InteractionTests::FWorldFixture F;
	APickupActor* Item = F.Spawn<APickupActor>(FVector(180, 0, 0));
	AToggleSwitchActor* Switch = F.Spawn<AToggleSwitchActor>(FVector(0, 200, 0));
	// 잡기 전 충돌 응답을 저장해 내려놓기 후 정확히 복원되는지 비교한다.
	const ECollisionResponse OriginalResponse = Item->Mesh->GetCollisionResponseToChannel(ECC_Pawn);
	TestTrue(TEXT("E grabs physical item"), F.Interaction->TryInteract().bSucceeded);
	TestTrue(TEXT("Carry state recorded"), F.Carry->IsHolding());
	// 잡은 물건 대신 스위치를 바라봐도 운반 중 E가 차단되는지 확인한다.
	F.Player->SetActorRotation(FRotator(0, 90, 0));
	TestFalse(TEXT("Holding blocks E even when aiming at switch"), F.Interaction->TryInteract().bSucceeded);
	TestFalse(TEXT("Switch was not activated while holding"), Switch->bIsOn);
	TestTrue(TEXT("Throw allowed by default"), F.Carry->CanThrowHeldObject());
	// 던지기 금지와 내려놓기 허용이 서로 독립적인지 확인한다.
	Item->Carryable->bCanThrow = false;
	TestFalse(TEXT("Per-item throw restriction applied"), F.Carry->CanThrowHeldObject());
	TestTrue(TEXT("G still drops a non-throwable item"), F.Carry->TryDrop());
	TestEqual(TEXT("Pawn collision restored"), Item->Mesh->GetCollisionResponseToChannel(ECC_Pawn), OriginalResponse);
	TestFalse(TEXT("Empty-hand drop is harmless"), F.Carry->TryDrop());
	TestTrue(TEXT("E is available again after drop"), F.Interaction->TryInteract().bSucceeded);

	F.Player->SetActorRotation(FRotator::ZeroRotator);
	TestTrue(TEXT("Item can be grabbed again"), F.Interaction->TryInteract().bSucceeded);
	// G를 누르지 않고 물건이 사라져도 운반 상태와 E 차단이 풀려야 한다.
	Item->Destroy();
	TestFalse(TEXT("Destroyed held item clears carry state"), F.Carry->IsHolding());
	TestTrue(TEXT("Destroyed held item unlocks E"), F.Interaction->CanInteractNow());
	return true;
}

#endif
