/*
 * 아이템 사용과 던지기의 공통 데이터
 * 입력 문맥, 단발·지속 사용 결과, 운반 해제 사유와 던진 결과를 정의한다.
 * 수치는 편집 시 이해하기 쉬운 kg, m/s, N·s를 사용하고 물리 적용 지점에서 UE 단위로 변환한다.
 */

#pragma once

#include "CoreMinimal.h"
#include "ItemUseTypes.generated.h"

class UCarryComponent;

// 일반 내려놓기와 던지기를 구별한다. Forced는 파괴·거리 초과·종료 등의 강제 해제다.
UENUM(BlueprintType)
enum class ECarryReleaseReason : uint8
{
	Drop,
	Throw,
	Forced
};

USTRUCT(BlueprintType)
struct TESTRRR_API FItemUseContext
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category="Item Use")
	TObjectPtr<AActor> User = nullptr;

	UPROPERTY(BlueprintReadOnly, Category="Item Use")
	TObjectPtr<AActor> Item = nullptr;

	UPROPERTY(BlueprintReadOnly, Category="Item Use")
	TObjectPtr<UCarryComponent> Carrier = nullptr;

	UPROPERTY(BlueprintReadOnly, Category="Item Use")
	FVector ViewOrigin = FVector::ZeroVector;

	// 정규화한 카메라 전방 방향. 지속 사용 중에는 현재 시선으로 다시 갱신한다.
	UPROPERTY(BlueprintReadOnly, Category="Item Use")
	FVector AimDirection = FVector::ForwardVector;
};

USTRUCT(BlueprintType)
struct TESTRRR_API FItemUseResult
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Item Use")
	bool bSucceeded = false;

	// true면 UpdateUse를 매 프레임 호출하고, 입력 종료 또는 상태 변경 시 정리한다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Item Use")
	bool bKeepUsing = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Item Use")
	FText FailureReason;
};

USTRUCT(BlueprintType)
struct TESTRRR_API FItemThrowSettings
{
	GENERATED_BODY()

	// 같은 충격량이면 질량이 큰 물건일수록 추가되는 속도가 작다: Δv = J / m.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Throw", meta=(ClampMin="0", DisplayName="Impulse (N s)"))
	float ImpulseNewtonSeconds = 12.f;

	// 던지기 시 운반 속도를 정리한 뒤 적용할 속도 상한이다. 기존 필드명은 BP 호환을 위해 유지한다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Throw", meta=(ClampMin="0", Units="m/s"))
	float MaxAddedSpeedMetersPerSecond = 18.f;
};

USTRUCT(BlueprintType)
struct TESTRRR_API FItemThrowInfo
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category="Throw")
	TObjectPtr<AActor> Thrower = nullptr;

	UPROPERTY(BlueprintReadOnly, Category="Throw")
	float MassKg = 0.f;

	UPROPERTY(BlueprintReadOnly, Category="Throw")
	FVector Direction = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category="Throw")
	float AppliedImpulseNewtonSeconds = 0.f;

	UPROPERTY(BlueprintReadOnly, Category="Throw")
	float AddedSpeedMetersPerSecond = 0.f;
};
