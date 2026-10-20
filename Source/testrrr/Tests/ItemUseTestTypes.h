/*
 * 아이템 사용 자동 테스트 전용 관찰 객체
 * BP 콘텐츠 없이도 시작·유지·종료·취소와 던지기 성공 알림의 횟수를 검증한다.
 * 게임용 아이템이 아니므로 편집기의 클래스 선택 목록에서는 숨긴다.
 */

#pragma once

#include "CoreMinimal.h"
#include "Interaction/ItemUseBehavior.h"
#include "Interaction/ThrowablePickupActor.h"
#include "testrrrCharacter.h"
#include "ItemUseTestTypes.generated.h"

UCLASS(Transient, NotBlueprintable, HideDropdown)
class UItemUseTestBehavior : public UItemUseBehavior
{
	GENERATED_BODY()

public:
	bool bContinuous = true;
	bool bFailBegin = false;
	bool bDropDuringBegin = false;
	int32 BeginCount = 0;
	int32 UpdateCount = 0;
	int32 EndCount = 0;
	int32 CancelCount = 0;
	FVector LastDirection = FVector::ZeroVector;

	virtual FItemUseResult BeginUse_Implementation(const FItemUseContext& Context) override;
	virtual void UpdateUse_Implementation(const FItemUseContext& Context, float DeltaTime) override;
	virtual void EndUse_Implementation(const FItemUseContext& Context) override;
	virtual void CancelUse_Implementation(const FItemUseContext& Context) override;
};

UCLASS(Transient, NotBlueprintable, HideDropdown)
class AItemUseTestPickupActor : public AThrowablePickupActor
{
	GENERATED_BODY()

public:
	int32 ThrownCount = 0;
	FItemThrowInfo LastThrow;
	virtual void OnItemThrown_Implementation(const FItemThrowInfo& Info) override;
};

UCLASS(Transient, NotBlueprintable, HideDropdown)
class AInteractionTestCharacter : public AtestrrrCharacter
{
	GENERATED_BODY()
public:
	AInteractionTestCharacter();
	void AimForTest(float Yaw, float Pitch) { DoAim(Yaw, Pitch); }
	void BeginRotateForTest() { DoBeginRotateItem(); }
	void EndRotateForTest() { DoEndRotateItem(); }
};
