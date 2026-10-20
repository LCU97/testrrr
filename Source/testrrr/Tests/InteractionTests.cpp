/*
 * 상호작용 프레임워크 자동 검증
 * 임시 월드에서 상호작용·운반·아이템 사용 수명주기와 질량을 반영한 던지기를 확인한다.
 * 실제 맵을 변경하지 않으며 테스트 월드는 종료 시 정리한다.
 */

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/PlayerController.h"
#include "Engine/LocalPlayer.h"
#include "Components/BoxComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Interaction/InteractionComponent.h"
#include "Interaction/CarryComponent.h"
#include "Interaction/CarryableComponent.h"
#include "Interaction/PickupActor.h"
#include "Interaction/ToggleSwitchActor.h"
#include "Interaction/ItemUseComponent.h"
#include "Interaction/ThrowItemUseBehavior.h"
#include "Interaction/ItemThrowPhysics.h"
#include "Tests/ItemUseTestTypes.h"
#include "GameplayActors/Items/WoodenBlockRectangle.h"
#include "GameplayActors/Items/WoodenBlockTriangle.h"
#include "GameplayActors/Items/BoxItem.h"
#include "GameplayActors/Items/ChairItem.h"
#include "GameplayActors/Items/DominoItem.h"
#include "GameplayActors/Items/BasketballItem.h"
#include "GameplayActors/Items/BowlingBallItem.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "GameplayActors/Switches/ItemSpawnSwitch.h"
#include "Components/ArrowComponent.h"
#include "PhysicsEngine/PhysicsHandleComponent.h"
#include "PhysicsEngine/BodySetup.h"
#include "HAL/FileManager.h"
#include "Misc/PackageName.h"
#include "UObject/UObjectIterator.h"

namespace InteractionTests
{
	// 각 테스트가 독립된 월드와 플레이어 컴포넌트를 사용하도록 준비·종료를 묶는다.
	struct FWorldFixture
	{
		UWorld* World;
		AActor* Player;
		UCarryComponent* Carry;
		UItemUseComponent* ItemUse;
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
			ItemUse = NewObject<UItemUseComponent>(Player);
			Player->AddInstanceComponent(ItemUse);
			ItemUse->RegisterComponent();
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FInteractionSphereTraceTest, "HundredButtons.Interaction.SphereTraceToleranceAndOcclusion",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FInteractionSphereTraceTest::RunTest(const FString& Parameters)
{
	InteractionTests::FWorldFixture F;
	// 버튼 반폭은 25cm. 중심선에서 32cm 떨어져 라인은 빗나가지만 반경 10cm는 닿는다.
	AToggleSwitchActor* Target = F.Spawn<AToggleSwitchActor>(FVector(200, 32, 0));
	F.Interaction->TraceRadius = 2.f;
	TestFalse(TEXT("Narrow sweep misses an off-axis button"), F.Interaction->TryInteract().bSucceeded);
	F.Interaction->TraceRadius = 10.f;
	F.Interaction->bDrawDebugTrace = true;
	TestTrue(TEXT("Sphere accepts a slightly off-axis button with debug enabled"), F.Interaction->TryInteract().bSucceeded);
	TestTrue(TEXT("Off-axis target activated"), Target->bIsOn);
	F.Interaction->bDrawDebugTrace = false;
	AActor* Wall = F.Spawn<AActor>(FVector(100, 0, 0));
	UBoxComponent* Box = NewObject<UBoxComponent>(Wall);
	Wall->SetRootComponent(Box);
	Box->SetBoxExtent(FVector(10, 100, 100));
	Box->SetCollisionProfileName(TEXT("BlockAll"));
	Box->RegisterComponent();
	Box->SetWorldLocation(FVector(100, 0, 0));
	F.Tick(0.25f);
	TestNull(TEXT("Sphere cannot focus through a wall"), F.Interaction->GetFocusedActor());
	TestFalse(TEXT("Wall still blocks E after cooldown"), F.Interaction->TryInteract().bSucceeded);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPhysicalSwitchTriggerTest, "HundredButtons.Interaction.PhysicalSwitchTrigger",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPhysicalSwitchTriggerTest::RunTest(const FString& Parameters)
{
	InteractionTests::FWorldFixture F;
	AToggleSwitchActor* Switch = F.Spawn<AToggleSwitchActor>(FVector::ZeroVector);
	APickupActor* Item = F.Spawn<APickupActor>(FVector(100.f, 0.f, 0.f));
	FHitResult Hit;

	TestEqual(TEXT("Physical trigger is wider than the 25cm visible button"),
		Switch->PhysicalTrigger->GetUnscaledBoxExtent(), FVector(20.f));
	TestTrue(TEXT("Physical trigger generates overlaps"), Switch->PhysicalTrigger->GetGenerateOverlapEvents());
	TestEqual(TEXT("Physical trigger is query-only"), Switch->PhysicalTrigger->GetCollisionEnabled(),
		ECollisionEnabled::QueryOnly);
	TestEqual(TEXT("Physical trigger overlaps physics bodies"),
		Switch->PhysicalTrigger->GetCollisionResponseToChannel(ECC_PhysicsBody), ECR_Overlap);
	TestEqual(TEXT("Pickup uses the physics-body object channel"), Item->Mesh->GetCollisionObjectType(), ECC_PhysicsBody);
	TestTrue(TEXT("Pickup body simulates physics"), Item->Mesh->IsSimulatingPhysics());

	Switch->PhysicalTrigger->OnComponentBeginOverlap.Broadcast(
		Switch->PhysicalTrigger, Item, Item->Mesh, 0, false, Hit);
	TestTrue(TEXT("First physical contact activates the switch"), Switch->bIsOn);

	Switch->PhysicalTrigger->OnComponentBeginOverlap.Broadcast(
		Switch->PhysicalTrigger, Item, Item->Mesh, 0, false, Hit);
	TestTrue(TEXT("Repeated begin overlap while touching does not activate again"), Switch->bIsOn);
	Switch->PhysicalTrigger->OnComponentEndOverlap.Broadcast(Switch->PhysicalTrigger, Item, Item->Mesh, 0);
	Switch->PhysicalTrigger->OnComponentEndOverlap.Broadcast(Switch->PhysicalTrigger, Item, Item->Mesh, 0);
	F.Tick(0.21f);
	Switch->PhysicalTrigger->OnComponentBeginOverlap.Broadcast(
		Switch->PhysicalTrigger, Item, Item->Mesh, 0, false, Hit);
	TestFalse(TEXT("Leaving and re-entering activates again"), Switch->bIsOn);

	AToggleSwitchActor* HeldItemSwitch = F.Spawn<AToggleSwitchActor>(FVector(500.f, 0.f, 0.f));
	APickupActor* HeldItem = F.Spawn<APickupActor>(FVector(600.f, 0.f, 0.f));
	HeldItem->Carryable->CurrentCarrier = F.Player;
	HeldItemSwitch->PhysicalTrigger->OnComponentBeginOverlap.Broadcast(
		HeldItemSwitch->PhysicalTrigger, HeldItem, HeldItem->Mesh, 0, false, Hit);
	TestFalse(TEXT("An item carried by the player cannot activate a physical switch"), HeldItemSwitch->bIsOn);
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FItemUseDispatchTest, "HundredButtons.ItemUse.DispatchAndRestrictions",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FItemUseDispatchTest::RunTest(const FString& Parameters)
{
	InteractionTests::FWorldFixture F;
	APickupActor* Item = F.Spawn<APickupActor>(FVector(180, 0, 0));
	TestTrue(TEXT("Grab a plain item"), F.Interaction->TryInteract().bSucceeded);
	// 던지기 허용 플래그만 있다고 사용 동작 없는 물건이 자동으로 던져져서는 안 된다.
	TestFalse(TEXT("No behavior means no left-click action"), F.ItemUse->BeginUseInput().bSucceeded);
	TestTrue(TEXT("Plain item stays held"), F.Carry->IsHolding());
	F.ItemUse->EndUseInput();
	UItemUseTestBehavior* Behavior = NewObject<UItemUseTestBehavior>(Item);
	Behavior->bContinuous = false;
	Item->UseBehavior = Behavior;
	TestTrue(TEXT("Assigned non-throw action is dispatched"), F.ItemUse->BeginUseInput().bSucceeded);
	F.ItemUse->EndUseInput();
	TestEqual(TEXT("Single use begins once"), Behavior->BeginCount, 1);
	TestEqual(TEXT("Single use does not receive a trailing EndUse"), Behavior->EndCount, 0);
	TestTrue(TEXT("Non-throw action keeps the item held"), F.Carry->IsHolding());
	Item->UseBehavior = NewObject<UThrowItemUseBehavior>(Item);
	Item->Carryable->bCanThrow = false;
	TestFalse(TEXT("Throw restriction is respected"), F.ItemUse->BeginUseInput().bSucceeded);
	TestTrue(TEXT("Rejected throw keeps item held"), F.Carry->IsHolding());
	F.ItemUse->EndUseInput();
	Item->Carryable->bCanThrow = true;
	Item->bItemUseEnabled = false;
	TestFalse(TEXT("Item can disable its own use"), F.ItemUse->BeginUseInput().bSucceeded);
	F.ItemUse->EndUseInput();
	TestTrue(TEXT("Disabled use does not block G drop"), F.Carry->TryDrop());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FItemUseLifecycleTest, "HundredButtons.ItemUse.LifecycleAndNoRetarget",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FItemUseLifecycleTest::RunTest(const FString& Parameters)
{
	InteractionTests::FWorldFixture F;
	APickupActor* First = F.Spawn<APickupActor>(FVector(180, 0, 0));
	APickupActor* Second = F.Spawn<APickupActor>(FVector(0, 180, 0));
	UItemUseTestBehavior* A = NewObject<UItemUseTestBehavior>(First);
	UItemUseTestBehavior* B = NewObject<UItemUseTestBehavior>(Second);
	First->UseBehavior = A;
	Second->UseBehavior = B;
	F.Interaction->TryInteract();
	TestTrue(TEXT("Continuous use begins"), F.ItemUse->BeginUseInput().bKeepUsing);
	TestFalse(TEXT("Repeated start while pressed is ignored"), F.ItemUse->BeginUseInput().bSucceeded);
	TestEqual(TEXT("Begin is delivered once"), A->BeginCount, 1);
	F.Player->SetActorRotation(FRotator(0, 90, 0));
	F.Tick(0.016f);
	TestTrue(TEXT("Continuous action is updated"), A->UpdateCount > 0);
	TestTrue(TEXT("Update uses current aim"), A->LastDirection.Equals(FVector::RightVector, 0.001));
	F.ItemUse->EndUseInput();
	TestEqual(TEXT("Release ends the original action"), A->EndCount, 1);
	TestEqual(TEXT("Normal release is not cancellation"), A->CancelCount, 0);
	TestFalse(TEXT("Release clears session"), F.ItemUse->IsUsing());
	F.ItemUse->BeginUseInput();
	F.Carry->TryDrop();
	TestEqual(TEXT("Dropping cancels active use once"), A->CancelCount, 1);
	// 첫 물건이 물리적으로 움직여도 두 번째 물건을 안정적으로 선택하도록 멀리 옮긴다.
	First->SetActorLocation(FVector(1000, 1000, 0));
	F.Interaction->TryInteract();
	TestEqual(TEXT("Second item is held"), F.Carry->GetHeldActor(), static_cast<AActor*>(Second));
	TestFalse(TEXT("Held click cannot start the newly grabbed item"), F.ItemUse->BeginUseInput().bSucceeded);
	F.ItemUse->EndUseInput();
	TestEqual(TEXT("Old release is not sent to new item"), B->EndCount, 0);
	TestTrue(TEXT("Fresh click starts new item"), F.ItemUse->BeginUseInput().bSucceeded);
	Second->Destroy();
	TestFalse(TEXT("Item destruction clears session"), F.ItemUse->IsUsing());
	TestEqual(TEXT("Destruction cancels effect once"), B->CancelCount, 1);
	F.ItemUse->CancelUseInput();
	TestEqual(TEXT("Subsequent cancellation is harmless"), B->CancelCount, 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FItemUseBeginCancelTest, "HundredButtons.ItemUse.FailedAndInterruptedBegin",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FItemUseBeginCancelTest::RunTest(const FString& Parameters)
{
	InteractionTests::FWorldFixture F;
	APickupActor* Item = F.Spawn<APickupActor>(FVector(180, 0, 0));
	UItemUseTestBehavior* Behavior = NewObject<UItemUseTestBehavior>(Item);
	Item->UseBehavior = Behavior;
	F.Interaction->TryInteract();
	Behavior->bFailBegin = true;
	TestFalse(TEXT("Failed Begin reports failure"), F.ItemUse->BeginUseInput().bSucceeded);
	TestFalse(TEXT("Failed Begin leaves no session"), F.ItemUse->IsUsing());
	TestEqual(TEXT("Partially initialized effects are cleaned up"), Behavior->CancelCount, 1);
	F.ItemUse->EndUseInput();
	Behavior->bFailBegin = false;
	Behavior->bDropDuringBegin = true;
	TestFalse(TEXT("Drop during continuous Begin interrupts start"), F.ItemUse->BeginUseInput().bSucceeded);
	TestFalse(TEXT("Interrupted Begin leaves no session"), F.ItemUse->IsUsing());
	TestFalse(TEXT("Drop inside callback completes"), F.Carry->IsHolding());
	TestEqual(TEXT("Interrupted Begin cancels exactly once"), Behavior->CancelCount, 2);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FItemThrowPhysicsTest, "HundredButtons.ItemUse.MassImpulseFormula",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FItemThrowPhysicsTest::RunTest(const FString& Parameters)
{
	FItemThrowSettings Settings;
	FItemThrowInfo Info;
	FVector Impulse;
	TestTrue(TEXT("Light body has valid impulse"), ItemThrowPhysics::CalculateImpulse(Settings, 0.6f, FVector(2, 0, 0), Info, Impulse));
	TestTrue(TEXT("Light body added speed capped at 18m/s"), FMath::IsNearlyEqual(Info.AddedSpeedMetersPerSecond, 18.f));
	TestTrue(TEXT("Direction normalized and SI converted to UE units"), Impulse.Equals(FVector(1080, 0, 0), 0.01));
	ItemThrowPhysics::CalculateImpulse(Settings, 2.f, FVector::ForwardVector, Info, Impulse);
	TestTrue(TEXT("2kg receives 6m/s added speed"), FMath::IsNearlyEqual(Info.AddedSpeedMetersPerSecond, 6.f));
	ItemThrowPhysics::CalculateImpulse(Settings, 10.f, FVector::ForwardVector, Info, Impulse);
	TestTrue(TEXT("10kg receives 1.2m/s added speed"), FMath::IsNearlyEqual(Info.AddedSpeedMetersPerSecond, 1.2f));
	TestFalse(TEXT("Zero mass rejected"), ItemThrowPhysics::CalculateImpulse(Settings, 0.f, FVector::ForwardVector, Info, Impulse));
	TestFalse(TEXT("Zero direction rejected"), ItemThrowPhysics::CalculateImpulse(Settings, 1.f, FVector::ZeroVector, Info, Impulse));
	Settings.ImpulseNewtonSeconds = -1.f;
	TestFalse(TEXT("Negative impulse rejected"), ItemThrowPhysics::CalculateImpulse(Settings, 1.f, FVector::ForwardVector, Info, Impulse));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FItemThrowIntegrationTest, "HundredButtons.ItemUse.ThrowPhysicsAndReaction",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FItemThrowIntegrationTest::RunTest(const FString& Parameters)
{
	// 식뿐 아니라 실제 엔진 물리에서도 같은 충격량에 대한 질량 차이를 확인한다.
	for (const float Mass : {10.f, 20.f})
	{
		InteractionTests::FWorldFixture F;
		AItemUseTestPickupActor* Item = F.Spawn<AItemUseTestPickupActor>(FVector(180, 0, 0));
		Item->Mesh->SetMassOverrideInKg(NAME_None, Mass, true);
		Item->Mesh->SetEnableGravity(false);
		Item->Mesh->SetLinearDamping(0.f);
		F.Interaction->TryInteract();
		const FItemUseResult Result = F.ItemUse->BeginUseInput();
		TestTrue(TEXT("Throw behavior accepts use"), Result.bSucceeded);
		TestFalse(TEXT("Throw is a single action"), Result.bKeepUsing);
		TestFalse(TEXT("Throw releases held object"), F.Carry->IsHolding());
		TestFalse(TEXT("Throw does not leave a use session"), F.ItemUse->IsUsing());
		TestEqual(TEXT("OnThrown delivered once"), Item->ThrownCount, 1);
		TestTrue(TEXT("Reaction receives actual body mass"), FMath::IsNearlyEqual(Item->LastThrow.MassKg, Mass, 0.01f));
		TestEqual(TEXT("Reaction identifies thrower"), Item->LastThrow.Thrower.Get(), F.Player);
		TestEqual(TEXT("Release classified as Throw"), Item->Carryable->LastReleaseReason, ECarryReleaseReason::Throw);
		F.Tick(0.02f);
		F.Tick(0.02f);
		// 기본 힘 4배에서도 속도 상한에 걸리지 않는 질량으로 J/m 비율을 검증한다.
		const float ExpectedSpeedCm = 4800.f / Mass;
		TestTrue(TEXT("Engine impulse respects mass (cm/s)"), FMath::IsNearlyEqual(
			static_cast<float>(Item->Mesh->GetPhysicsLinearVelocity().X), ExpectedSpeedCm, 2.f));
		F.ItemUse->EndUseInput();
		TestEqual(TEXT("Input release does not throw again"), Item->ThrownCount, 1);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FItemDropReasonTest, "HundredButtons.ItemUse.DropAndFailedThrowDoNotReact",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FItemDropReasonTest::RunTest(const FString& Parameters)
{
	InteractionTests::FWorldFixture F;
	AItemUseTestPickupActor* Item = F.Spawn<AItemUseTestPickupActor>(FVector(180, 0, 0));
	F.Interaction->TryInteract();
	F.Carry->TryDrop();
	TestEqual(TEXT("G drop is classified separately"), Item->Carryable->LastReleaseReason, ECarryReleaseReason::Drop);
	TestEqual(TEXT("G drop does not arm a thrown reaction"), Item->ThrownCount, 0);
	F.Interaction->TryInteract();
	UThrowItemUseBehavior* Throw = Cast<UThrowItemUseBehavior>(Item->UseBehavior);
	Throw->ThrowSettings.ImpulseNewtonSeconds = 0.f;
	TestFalse(TEXT("Invalid throw settings reject use"), F.ItemUse->BeginUseInput().bSucceeded);
	TestTrue(TEXT("Failed throw retains held item"), F.Carry->IsHolding());
	TestEqual(TEXT("Failed throw does not call OnThrown"), Item->ThrownCount, 0);
	F.Carry->ForceRelease();
	TestEqual(TEXT("Forced release is classified separately"), Item->Carryable->LastReleaseReason, ECarryReleaseReason::Forced);
	TestEqual(TEXT("Forced release does not arm reaction"), Item->ThrownCount, 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FItemThrowStableLaunchTest, "HundredButtons.ItemUse.StableLaunchAfterCameraMotion",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FItemThrowStableLaunchTest::RunTest(const FString& Parameters)
{
	const FVector Directions[] = {FVector::ForwardVector, FVector::UpVector,
		FVector(1, 0, 1).GetSafeNormal(), FVector::RightVector};
	for (const FVector& Direction : Directions)
	{
		for (const FVector CarryVelocity : {FVector::ZeroVector, -Direction * 900.f, FVector(1800, -1200, -900)})
		{
			InteractionTests::FWorldFixture F;
			AItemUseTestPickupActor* Item = F.Spawn<AItemUseTestPickupActor>(FVector(180, 0, 0));
			Item->Mesh->SetMassOverrideInKg(NAME_None, 2.f, true);
			Item->Mesh->SetEnableGravity(false);
			Item->Mesh->SetLinearDamping(0.f);
			Item->Mesh->SetAngularDamping(0.f);
			FInteractionContext GrabContext;
			GrabContext.Interactor = F.Player;
			F.Carry->TryGrab(Item->Carryable, GrabContext);
			F.Player->SetActorRotation(Direction.Rotation());
			// 물리 핸들이 빠른 시점 이동을 따라오며 만든 운동을 재현한다.
			Item->Mesh->SetPhysicsLinearVelocity(CarryVelocity);
			Item->Mesh->SetPhysicsAngularVelocityInRadians(FVector(20, -30, 40));
			TestTrue(TEXT("Throw succeeds at every camera pitch"), F.ItemUse->BeginUseInput().bSucceeded);
			F.Tick(0.02f);
			F.Tick(0.02f);
			TestTrue(TEXT("Launch ignores carry velocity and follows current aim"),
				Item->Mesh->GetPhysicsLinearVelocity().Equals(Direction * 1800.f, 2.f));
			TestTrue(TEXT("Carry rotation does not create unwanted launch spin"),
				Item->Mesh->GetPhysicsAngularVelocityInRadians().IsNearlyZero(0.01));
			TestEqual(TEXT("Stable launch notifies item once"), Item->ThrownCount, 1);
		}
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FItemThrowStartOverlapTest, "HundredButtons.ItemUse.ThrowStartOverlapSafety",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FItemThrowStartOverlapTest::RunTest(const FString& Parameters)
{
	for (int32 Case = 0; Case < 4; ++Case)
	{
		InteractionTests::FWorldFixture F;
		AItemUseTestPickupActor* Item = F.Spawn<AItemUseTestPickupActor>(FVector(180, 0, 0));
		if (Case == 3)
		{
			Item->Mesh->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
		}
		F.Interaction->TryInteract();
		// 실제 플레이어와의 겹침도 검사하도록 Pawn 유형은 사용 주체에 부착한다.
		AActor* Blocker = (Case == 1 || Case == 3) ? F.Player : F.Spawn<AActor>(FVector::ZeroVector);
		UBoxComponent* Box = NewObject<UBoxComponent>(Blocker);
		Blocker->AddInstanceComponent(Box);
		Box->SetBoxExtent(FVector(8.f));
		Box->SetCollisionEnabled(Case == 2 ? ECollisionEnabled::QueryOnly : ECollisionEnabled::QueryAndPhysics);
		Box->SetCollisionObjectType((Case == 1 || Case == 3) ? ECC_Pawn : ECC_WorldStatic);
		Box->SetCollisionResponseToAllChannels(ECR_Block);
		Box->RegisterComponent();
		Box->SetWorldLocation(Item->Mesh->GetComponentLocation());
		// 접촉 중인 물건도 클릭 한 번으로 발사한다. 몸 충돌은 해제 후에 별도로 복원한다.
		TestTrue(TEXT("Contact does not swallow the throw click"), F.ItemUse->BeginUseInput().bSucceeded);
		TestFalse(TEXT("Click releases the held item"), F.Carry->IsHolding());
		TestEqual(TEXT("One click produces one thrown reaction"), Item->ThrownCount, 1);
		TestEqual(TEXT("Pawn collision stays ignored during launch grace"),
			Item->Mesh->GetCollisionResponseToChannel(ECC_Pawn), ECR_Ignore);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FItemDropMomentumTest, "HundredButtons.ItemUse.DropKeepsExistingMomentum",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FItemDropMomentumTest::RunTest(const FString& Parameters)
{
	InteractionTests::FWorldFixture F;
	AItemUseTestPickupActor* Item = F.Spawn<AItemUseTestPickupActor>(FVector(180, 0, 0));
	Item->Mesh->SetEnableGravity(false);
	Item->Mesh->SetLinearDamping(0.f);
	F.Interaction->TryInteract();
	const FVector ExistingVelocity(100, 200, -50);
	Item->Mesh->SetPhysicsLinearVelocity(ExistingVelocity);
	F.Carry->TryDrop();
	F.Tick(0.02f);
	F.Tick(0.02f);
	// 발사 안정화를 위해 넣은 초기화가 일반 G 내려놓기의 움직임까지 바꾸지 않도록 한다.
	TestTrue(TEXT("G drop preserves ordinary momentum"), Item->Mesh->GetPhysicsLinearVelocity().Equals(ExistingVelocity, 1.f));
	TestEqual(TEXT("G drop still has no thrown reaction"), Item->ThrownCount, 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FItemRestingThrowTest, "HundredButtons.ItemUse.RestingContactAllowsThrow",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FItemRestingThrowTest::RunTest(const FString& Parameters)
{
	InteractionTests::FWorldFixture F;
	AItemUseTestPickupActor* Item = F.Spawn<AItemUseTestPickupActor>(FVector(180, 0, 0));
	Item->Mesh->SetMassOverrideInKg(NAME_None, 2.f, true);
	Item->Mesh->SetEnableGravity(false);
	F.Interaction->TryInteract();
	AActor* Floor = F.Spawn<AActor>(FVector::ZeroVector);
	UBoxComponent* Box = NewObject<UBoxComponent>(Floor);
	Floor->AddInstanceComponent(Box);
	Box->SetBoxExtent(FVector(100.f, 100.f, 8.f));
	Box->SetCollisionProfileName(TEXT("BlockAll"));
	Box->RegisterComponent();
	// 접촉 오차 수준인 0.2cm 겹침을 재현한다. 이전에는 이것만으로 발사 전체가 취소됐다.
	const FVector BodyBottom = Item->Mesh->Bounds.Origin - FVector(0, 0, Item->Mesh->Bounds.BoxExtent.Z);
	Box->SetWorldLocation(BodyBottom - FVector(0, 0, 7.8f));
	TestTrue(TEXT("Single click throws an item touching the floor"), F.ItemUse->BeginUseInput().bSucceeded);
	TestFalse(TEXT("Floor contact no longer leaves item stuck in hand"), F.Carry->IsHolding());
	TestEqual(TEXT("Floor contact still emits exactly one reaction"), Item->ThrownCount, 1);
	TestEqual(TEXT("World collision is still enabled after launch"),
		Item->Mesh->GetCollisionResponseToChannel(ECC_WorldStatic), ECR_Block);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FItemPawnRestoreLifecycleTest, "HundredButtons.ItemUse.ThrownPawnCollisionLifecycle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FItemPawnRestoreLifecycleTest::RunTest(const FString& Parameters)
{
	{
		InteractionTests::FWorldFixture F;
		AItemUseTestPickupActor* Item = F.Spawn<AItemUseTestPickupActor>(FVector(180, 0, 0));
		Item->Mesh->SetEnableGravity(false);
		F.Interaction->TryInteract();
		UBoxComponent* PawnBox = NewObject<UBoxComponent>(F.Player);
		F.Player->AddInstanceComponent(PawnBox);
		PawnBox->SetBoxExtent(FVector(5000.f));
		PawnBox->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		PawnBox->SetCollisionObjectType(ECC_Pawn);
		PawnBox->SetCollisionResponseToAllChannels(ECR_Block);
		PawnBox->RegisterComponent();
		PawnBox->SetWorldLocation(Item->GetActorLocation());
		F.ItemUse->BeginUseInput();
		F.Tick(0.25f);
		TestEqual(TEXT("Collision is not restored while inside the thrower"),
			Item->Mesh->GetCollisionResponseToChannel(ECC_Pawn), ECR_Ignore);
		PawnBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		F.Tick(0.06f);
		TestEqual(TEXT("Collision restores after separation"),
			Item->Mesh->GetCollisionResponseToChannel(ECC_Pawn), ECR_Block);
	}
	{
		InteractionTests::FWorldFixture F;
		AItemUseTestPickupActor* Item = F.Spawn<AItemUseTestPickupActor>(FVector(180, 0, 0));
		Item->Mesh->SetEnableGravity(false);
		F.Interaction->TryInteract();
		F.ItemUse->BeginUseInput();
		F.ItemUse->EndUseInput();
		F.Interaction->TryInteract();
		TestTrue(TEXT("Thrown item can be grabbed again immediately"), F.Carry->IsHolding());
		F.Tick(0.3f);
		TestEqual(TEXT("Old timer does not restore collision during re-grab"),
			Item->Mesh->GetCollisionResponseToChannel(ECC_Pawn), ECR_Ignore);
		F.Carry->TryDrop();
		TestEqual(TEXT("Re-grab remembers original response for G drop"),
			Item->Mesh->GetCollisionResponseToChannel(ECC_Pawn), ECR_Block);
	}
	{
		InteractionTests::FWorldFixture F;
		AItemUseTestPickupActor* Item = F.Spawn<AItemUseTestPickupActor>(FVector(180, 0, 0));
		F.Interaction->TryInteract();
		F.ItemUse->BeginUseInput();
		F.Player->Destroy();
		TestEqual(TEXT("Carrier destruction restores thrown object collision"),
			Item->Mesh->GetCollisionResponseToChannel(ECC_Pawn), ECR_Block);
	}
	{
		InteractionTests::FWorldFixture F;
		AItemUseTestPickupActor* Item = F.Spawn<AItemUseTestPickupActor>(FVector(180, 0, 0));
		F.Interaction->TryInteract();
		F.ItemUse->BeginUseInput();
		Item->Mesh->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
		F.Tick(0.3f);
		TestEqual(TEXT("Explicit item collision changes are not overwritten"),
			Item->Mesh->GetCollisionResponseToChannel(ECC_Pawn), ECR_Overlap);
		Item->Destroy();
		F.Tick(0.1f);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPlacedItemThrowTest, "HundredButtons.ItemUse.PlacedItemConfiguration",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPlacedItemThrowTest::RunTest(const FString& Parameters)
{
	// 테스트 전용 질량만 검사하지 않고 실제 맵의 외부 액터 설정을 읽어 복제한다.
	// 패키지는 읽기만 하며 맵·아이템 에셋을 저장하거나 수정하지 않는다.
	TArray<FString> Files;
	IFileManager::Get().FindFilesRecursive(Files, *(FPaths::ProjectContentDir() / TEXT("__ExternalActors__/FirstPerson/Lvl_FirstPerson")),
		TEXT("*.uasset"), true, false);
	for (const FString& File : Files)
	{
		FString PackageName;
		if (FPackageName::TryConvertFilenameToLongPackageName(File, PackageName))
		{
			LoadPackage(nullptr, *PackageName, LOAD_None);
		}
	}
	TArray<APickupActor*> PlacedItems;
	for (TObjectIterator<APickupActor> It; It; ++It)
	{
		if (!It->IsTemplate() && It->GetExternalPackage())
		{
			PlacedItems.Add(*It);
		}
	}
	for (APickupActor* Template : PlacedItems)
	{
		InteractionTests::FWorldFixture F;
		Template->GetRootComponent()->UpdateComponentToWorld();
		FTransform Transform = Template->GetActorTransform();
		Transform.SetLocation(FVector(180, 0, 0));
		FActorSpawnParameters Params;
		Params.Template = Template;
		// 배치된 크기를 그대로 사용한다. 기본 MultiplyWithRoot는 크기를 한 번 더 곱한다.
		Params.TransformScaleMethod = ESpawnActorScaleMethod::OverrideRootScale;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		APickupActor* Item = Cast<APickupActor>(F.World->SpawnActor(Template->GetClass(), &Transform, Params));
		if (!TestNotNull(TEXT("Placed item can be cloned for inspection"), Item))
		{
			continue;
		}
		UThrowItemUseBehavior* Throw = Cast<UThrowItemUseBehavior>(Item->UseBehavior);
		AddInfo(FString::Printf(TEXT("PLACED_ITEM %s class=%s scale=%s mass=%.3fkg behavior=%s"),
			*Template->GetName(), *Template->GetClass()->GetName(), *Item->GetActorScale3D().ToString(),
			Item->Mesh->GetMass(), *GetNameSafe(Item->UseBehavior)));
		if (!Throw)
		{
			continue;
		}
		FInteractionContext Context;
		Context.Interactor = F.Player;
		Item->Mesh->SetEnableGravity(false);
		Item->Mesh->SetLinearDamping(0.f);
		TestTrue(TEXT("Placed throwing item can be grabbed"), F.Carry->TryGrab(Item->Carryable, Context));
		TestTrue(TEXT("Placed throwing item accepts one click"), F.ItemUse->BeginUseInput().bSucceeded);
		F.Tick(0.02f);
		F.Tick(0.02f);
		AddInfo(FString::Printf(TEXT("PLACED_THROW baseImpulse=%.3fNs strength=%.2f appliedImpulse=%.3fNs velocity=%.3fm/s"),
			Throw->ThrowSettings.ImpulseNewtonSeconds, F.Carry->ThrowStrengthMultiplier,
			Throw->ThrowSettings.ImpulseNewtonSeconds * F.Carry->ThrowStrengthMultiplier,
			Item->Mesh->GetPhysicsLinearVelocity().Size() / 100.0));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSettledHeldThrowTest, "HundredButtons.ItemUse.SettledHeldItemLaunch",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSettledHeldThrowTest::RunTest(const FString& Parameters)
{
	InteractionTests::FWorldFixture F;
	AItemUseTestPickupActor* Item = F.Spawn<AItemUseTestPickupActor>(FVector(180, 0, 0));
	F.Interaction->TryInteract();
	// 실제 플레이처럼 중력을 켠 채 한동안 들고 난 뒤 발사한다.
	for (int32 Frame = 0; Frame < 60; ++Frame)
	{
		F.Tick(1.f / 60.f);
	}
	const FVector BeforeLocation = Item->GetActorLocation();
	AddInfo(FString::Printf(TEXT("SETTLED_THROW mass=%.3fkg location=%s velocity=%s"),
		Item->Mesh->GetMass(), *BeforeLocation.ToString(), *Item->Mesh->GetPhysicsLinearVelocity().ToString()));
	TestTrue(TEXT("Settled held item still accepts use"), F.ItemUse->BeginUseInput().bSucceeded);
	F.Tick(0.02f);
	F.Tick(0.02f);
	AddInfo(FString::Printf(TEXT("SETTLED_LAUNCH velocity=%s"), *Item->Mesh->GetPhysicsLinearVelocity().ToString()));
	TestTrue(TEXT("Physics handle no longer restrains launched item"), Item->GetActorLocation().X > BeforeLocation.X);
	// 질량을 임의로 줄이지 않은 기본 아이템도 '내려놓기'와 구별되는 발사 속도가 필요하다.
	TestTrue(TEXT("Default real-mass item launches at a useful speed"), Item->Mesh->GetPhysicsLinearVelocity().X >= 500.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPlacementItemsTest, "HundredButtons.Placement.ItemsAndHorizontalRotation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPlacementItemsTest::RunTest(const FString& Parameters)
{
	InteractionTests::FWorldFixture F;
	TestFalse(TEXT("Empty hand cannot enter rotation mode"), F.Carry->BeginRotateHeldObject());
	APlacementItem* Items[] = {
		F.Spawn<AWoodenBlockRectangle>(FVector(180, 0, 0)),
		F.Spawn<AWoodenBlockTriangle>(FVector(180, 300, 0)),
		F.Spawn<AChairItem>(FVector(180, 600, 0)),
		F.Spawn<ABoxItem>(FVector(180, 900, 0)) };
	FInteractionContext Context;
	Context.Interactor = F.Player;
	for (APlacementItem* Item : Items)
	{
		TestNotNull(TEXT("Native item spawns"), Item);
		if (!Item) { continue; }
		TestTrue(TEXT("Simple collision exists"), Item->Mesh->GetBodySetup()
			&& Item->Mesh->GetBodySetup()->AggGeom.GetElementCount() > 0);
		TestTrue(TEXT("Item simulates physics"), Item->Mesh->IsSimulatingPhysics());
		F.Player->SetActorLocation(Item->GetActorLocation() - FVector(180, 0, 0));
		TestTrue(TEXT("Item can be picked up"), F.Carry->TryGrab(Item->Carryable, Context));
		F.Player->SetActorRotation(FRotator(35, 0, 0));
		TestFalse(TEXT("Mouse rotation requires Shift mode"), F.Carry->RotateHeldObject(FVector2D(10, 10)));
		TestTrue(TEXT("Shift begins rotation"), F.Carry->BeginRotateHeldObject());
		F.Carry->HeldRotationSensitivity = 1.f;
		TestTrue(TEXT("Mouse rotates held object"), F.Carry->RotateHeldObject(FVector2D(30, 25)));
		FVector Location;
		FRotator Rotation;
		F.Player->FindComponentByClass<UPhysicsHandleComponent>()->GetTargetLocationAndRotation(Location, Rotation);
		TestTrue(TEXT("Mouse yaw rotates horizontally"), FMath::Abs(Rotation.Yaw) > 10.f);
		TestTrue(TEXT("Mouse pitch rotates vertically"), FMath::Abs(Rotation.Pitch) > 10.f);
		const FQuat EditedRotation = Rotation.Quaternion();
		F.Carry->EndRotateHeldObject();
		TestFalse(TEXT("Shift release ends rotation"), F.Carry->IsRotatingHeldObject());
		TestFalse(TEXT("Mouse no longer rotates after Shift release"), F.Carry->RotateHeldObject(FVector2D(90, 90)));
		F.Player->SetActorRotation(FRotator(-20, 90, 0));
		F.Tick(1.f / 60.f);
		F.Player->FindComponentByClass<UPhysicsHandleComponent>()->GetTargetLocationAndRotation(Location, Rotation);
		TestTrue(TEXT("Camera movement preserves chosen item orientation"), Rotation.Quaternion().Equals(EditedRotation, 0.001f));
		F.Carry->BeginRotateHeldObject();
		TestTrue(TEXT("Item drops after rotation"), F.Carry->TryDrop());
		TestFalse(TEXT("Dropping clears rotation mode"), F.Carry->IsRotatingHeldObject());
		F.Player->SetActorRotation(FRotator::ZeroRotator);
	}
	const FVector RectangleSize = Items[0]->Mesh->GetStaticMesh()->GetBoundingBox().GetSize() * Items[0]->Mesh->GetComponentScale();
	const FVector TriangleSize = Items[1]->Mesh->GetStaticMesh()->GetBoundingBox().GetSize() * Items[1]->Mesh->GetComponentScale();
	TestTrue(TEXT("Blocks share actual outer dimensions"), RectangleSize.Equals(TriangleSize, 0.1f));
	TestTrue(TEXT("Block dimensions are 100x100x50 cm"), RectangleSize.Equals(FVector(100, 100, 50), 0.1f));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FItemSpawnSwitchTest, "HundredButtons.Placement.SpawnSwitch",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FItemSpawnSwitchTest::RunTest(const FString& Parameters)
{
	InteractionTests::FWorldFixture F;
	AItemSpawnSwitch* Switch = F.Spawn<AItemSpawnSwitch>(FVector(0, 500, 0));
	Switch->SetActorRotation(FRotator(0, 90, 0));
	Switch->SpawnPoint->SetRelativeLocation(FVector(250, 0, 100));
	Switch->SpawnClass = AWoodenBlockTriangle::StaticClass();
	FInteractionContext Context;
	Context.Interactor = F.Player;
	TestTrue(TEXT("E spawns selected native class"), IInteractable::Execute_Interact(Switch, Context).bSucceeded);
	AActor* First = Switch->LastSpawnedActor.Get();
	TestNotNull(TEXT("Spawn result available"), First);
	if (!First) { return false; }
	TestTrue(TEXT("Selected item class used"), First->IsA<AWoodenBlockTriangle>());
	TestTrue(TEXT("Spawn arrow position follows rotated switch"), First->GetActorLocation().Equals(Switch->SpawnPoint->GetComponentLocation(), 0.1f));
	TestTrue(TEXT("Spawn arrow orientation used"), First->GetActorRotation().Equals(Switch->SpawnPoint->GetComponentRotation(), 0.1f));
	TestFalse(TEXT("Cooldown prevents duplicate press"), IInteractable::Execute_Interact(Switch, Context).bSucceeded);
	First->Destroy();
	F.Tick(0.25f);
	TestTrue(TEXT("Can spawn again after cooldown"), IInteractable::Execute_Interact(Switch, Context).bSucceeded);
	Switch->SpawnClass = nullptr;
	TestFalse(TEXT("Missing class safely rejected"), IInteractable::Execute_Interact(Switch, Context).bSucceeded);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBallItemsTest, "HundredButtons.Balls.ColorCarryThrowAndBounce",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBallItemsTest::RunTest(const FString& Parameters)
{
	InteractionTests::FWorldFixture F;
	ABasketballItem* Basketball = F.Spawn<ABasketballItem>(FVector(180, 0, 100));
	ABowlingBallItem* Bowling = F.Spawn<ABowlingBallItem>(FVector(400, 100, 100));
	TestTrue(TEXT("Basketball uses 0.9 kilogram mass"), FMath::IsNearlyEqual(Basketball->Mesh->GetMass(), 0.9f, 0.01f));
	TestTrue(TEXT("Bowling ball uses thirty kilogram mass"), FMath::IsNearlyEqual(Bowling->Mesh->GetMass(), 30.f, 0.01f));
	for (ABallItem* Ball : TArray<ABallItem*>{ Basketball, Bowling })
	{
		TestTrue(TEXT("Native sphere collision is available"), Ball->Mesh->GetBodySetup()->AggGeom.SphereElems.Num() > 0);
		UMaterialInterface* Material = Ball->Mesh->GetMaterial(0);
		FLinearColor Color;
		TestTrue(TEXT("Material exposes the actual base color parameter"), Material && Material->GetVectorParameterValue(FMaterialParameterInfo(TEXT("Base Color")), Color));
		TestTrue(TEXT("Visible material uses requested ball color"), Color.Equals(Ball->BallColor, 0.001f));
	}
	FInteractionContext Context;
	Context.Interactor = F.Player;
	TestFalse(TEXT("Bowling ball cannot be picked up"), F.Carry->TryGrab(Bowling->Carryable, Context));
	TestFalse(TEXT("Bowling ball E interaction is unavailable"), IInteractable::Execute_CanInteract(Bowling, Context));
	TestTrue(TEXT("Basketball can be picked up"), F.Carry->TryGrab(Basketball->Carryable, Context));
	TestTrue(TEXT("Basketball uses existing left-click throw"), F.ItemUse->BeginUseInput().bSucceeded);
	TestFalse(TEXT("Throw releases basketball"), F.Carry->IsHolding());
	// Chaos의 임펄스는 다음 물리 스텝에서 속도에 반영된다.
	F.Tick(0.02f);
	TestTrue(TEXT("Throw applies forward velocity"), Basketball->Mesh->GetPhysicsLinearVelocity().X > 500.f);
	Basketball->Destroy();
	Bowling->Destroy();
	// 동일한 바닥과 낙하 높이에서 실제 반발 차이를 확인한다.
	AActor* Floor = F.World->SpawnActor<AActor>();
	UBoxComponent* FloorBody = NewObject<UBoxComponent>(Floor);
	Floor->SetRootComponent(FloorBody);
	Floor->AddInstanceComponent(FloorBody);
	FloorBody->SetBoxExtent(FVector(1000, 1000, 25));
	FloorBody->SetCollisionProfileName(TEXT("BlockAll"));
	FloorBody->RegisterComponent();
	Floor->SetActorLocation(FVector(0, 0, -25));
	Basketball = F.Spawn<ABasketballItem>(FVector(500, -100, 200));
	Bowling = F.Spawn<ABowlingBallItem>(FVector(500, 100, 200));
	float BasketballRebound = 0.f;
	float BowlingRebound = 0.f;
	for (int32 Frame = 0; Frame < 180; ++Frame)
	{
		F.Tick(1.f / 120.f);
		if (Frame > 85)
		{
			BasketballRebound = FMath::Max(BasketballRebound, Basketball->GetActorLocation().Z);
			BowlingRebound = FMath::Max(BowlingRebound, Bowling->GetActorLocation().Z);
		}
	}
	AddInfo(FString::Printf(TEXT("Rebound heights: basketball %.1fcm, bowling %.1fcm"), BasketballRebound, BowlingRebound));
	TestTrue(TEXT("Basketball visibly bounces"), BasketballRebound > 45.f);
	TestTrue(TEXT("Bowling ball stays low after impact"), BowlingRebound < 25.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FItemScaleEditingTest, "HundredButtons.Placement.ManualScaleSurvivesReconstruction",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FItemScaleEditingTest::RunTest(const FString& Parameters)
{
	InteractionTests::FWorldFixture F;
	AActor* Items[] = { F.Spawn<AWoodenBlockTriangle>(FVector(0, 500, 0)),
		F.Spawn<AWoodenBlockRectangle>(FVector(0, 800, 0)),
		F.Spawn<ABoxItem>(FVector(0, 1100, 0)),
		F.Spawn<ABasketballItem>(FVector(0, 1400, 0)),
		F.Spawn<ABowlingBallItem>(FVector(0, 1700, 0)) };
	for (AActor* Item : Items)
	{
		const FVector EditedScale(1.5f, 0.75f, 2.f);
		Item->SetActorScale3D(EditedScale);
		// 이동·속성 편집 시 에디터가 실행하는 재구성에서도 수동 Scale이 유지되어야 한다.
		Item->RerunConstructionScripts();
		TestTrue(*FString::Printf(TEXT("%s preserves edited scale"), *Item->GetClass()->GetName()),
			Item->GetActorScale3D().Equals(EditedScale, 0.001f));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMouseItemRotationTest, "HundredButtons.Placement.ShiftMouseCameraAndCleanup",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMouseItemRotationTest::RunTest(const FString& Parameters)
{
	InteractionTests::FWorldFixture F;
	AInteractionTestCharacter* Character = F.Spawn<AInteractionTestCharacter>(FVector(0, 0, 200));
	APlayerController* Controller = F.World->SpawnActor<APlayerController>();
	// AddControllerYaw/PitchInput은 로컬 플레이어의 컨트롤러만 받는다.
	Controller->Player = NewObject<ULocalPlayer>(GEngine);
	Controller->Possess(Character);
	UCarryComponent* Carry = Character->FindComponentByClass<UCarryComponent>();
	ADominoItem* Item = F.Spawn<ADominoItem>(FVector(180, 0, 200));
	FInteractionContext Context;
	Context.Interactor = Character;
	TestTrue(TEXT("Domino can be picked up"), Carry->TryGrab(Item->Carryable, Context));
	Character->BeginRotateForTest();
	TestTrue(TEXT("Shift enables held-item rotation"), Carry->IsRotatingHeldObject());
	const FRotator BodyRotation = Character->GetActorRotation();
	const FRotator ViewRotation = Controller->GetControlRotation();
	Controller->RotationInput = FRotator::ZeroRotator;
	Character->AimForTest(50, 30);
	TestTrue(TEXT("Mouse does not queue camera rotation while Shift is held"), Controller->RotationInput.IsNearlyZero());
	TestTrue(TEXT("Camera orientation stays fixed"), Controller->GetControlRotation().Equals(ViewRotation));
	TestTrue(TEXT("Character body stays fixed"), Character->GetActorRotation().Equals(BodyRotation));
	FVector Location;
	FRotator Rotation;
	Character->FindComponentByClass<UPhysicsHandleComponent>()->GetTargetLocationAndRotation(Location, Rotation);
	TestFalse(TEXT("Mouse rotated held item on both axes"), FMath::IsNearlyZero(Rotation.Yaw) || FMath::IsNearlyZero(Rotation.Pitch));
	Character->EndRotateForTest();
	Character->AimForTest(5, 3);
	TestFalse(TEXT("Camera input resumes on Shift release"), Controller->RotationInput.IsNearlyZero());
	Controller->RotationInput = FRotator::ZeroRotator;
	Character->BeginRotateForTest();
	Carry->TryDrop();
	Character->AimForTest(5, 3);
	TestFalse(TEXT("Dropping restores camera input even while Shift was held"), Controller->RotationInput.IsNearlyZero());
	TestFalse(TEXT("Drop clears rotation state"), Carry->IsRotatingHeldObject());
	TestTrue(TEXT("Domino can be grabbed again"), Carry->TryGrab(Item->Carryable, Context));
	Character->BeginRotateForTest();
	Item->Destroy();
	TestFalse(TEXT("Destroyed item clears rotation state"), Carry->IsRotatingHeldObject());
	Controller->RotationInput = FRotator::ZeroRotator;
	Character->AimForTest(5, 3);
	TestFalse(TEXT("Destroyed item restores camera input"), Controller->RotationInput.IsNearlyZero());
	Character->BeginRotateForTest();
	TestFalse(TEXT("Empty-hand Shift cannot lock camera"), Carry->IsRotatingHeldObject());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDominoPhysicsTest, "HundredButtons.Domino.PhysicalChain",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDominoPhysicsTest::RunTest(const FString& Parameters)
{
	InteractionTests::FWorldFixture F;
	AActor* Floor = F.World->SpawnActor<AActor>();
	UBoxComponent* FloorBody = NewObject<UBoxComponent>(Floor);
	Floor->SetRootComponent(FloorBody);
	Floor->AddInstanceComponent(FloorBody);
	FloorBody->SetBoxExtent(FVector(1500, 1000, 25));
	FloorBody->SetCollisionProfileName(TEXT("BlockAll"));
	FloorBody->RegisterComponent();
	Floor->SetActorLocation(FVector(0, 0, -25));
	TArray<ADominoItem*> Dominoes;
	for (int32 Index = 0; Index < 8; ++Index)
	{
		Dominoes.Add(F.Spawn<ADominoItem>(FVector(500 + Index * 60, 300, 45.5f)));
	}
	for (int32 Frame = 0; Frame < 60; ++Frame) { F.Tick(1.f / 120.f); }
	for (const ADominoItem* Domino : Dominoes)
	{
		TestTrue(TEXT("Domino stands before being pushed"), Domino->GetActorUpVector().Z > 0.98f);
	}
	Dominoes[0]->Mesh->AddImpulseAtLocation(FVector(150, 0, 0), Dominoes[0]->GetActorLocation() + FVector(0, 0, 42));
	for (int32 Frame = 0; Frame < 600; ++Frame) { F.Tick(1.f / 120.f); }
	for (int32 Index = 0; Index < Dominoes.Num(); ++Index)
	{
		TestTrue(*FString::Printf(TEXT("Domino %d falls through actual physics"), Index), Dominoes[Index]->GetActorUpVector().Z < 0.7f);
	}
	return true;
}

#endif
