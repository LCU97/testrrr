/*
 * 상호작용 공통 데이터
 * 입력 처리, 월드 액터, UI가 주고받는 문맥·안내·실행 결과를 정의한다.
 * 조회 데이터와 실행 결과를 분리하여 개별 아이템 구현이 입력 코드에 의존하지 않게 한다.
 */

#pragma once

#include "CoreMinimal.h"
#include "ButtonInteractionTypes.generated.h"

// 누가 어떤 충돌 부위를 대상으로 요청했는지 전달한다.
USTRUCT(BlueprintType)
struct TESTRRR_API FInteractionContext
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category="Interaction")
	TObjectPtr<AActor> Interactor = nullptr;

	UPROPERTY(BlueprintReadOnly, Category="Interaction")
	FHitResult Hit;
};

// UI 문구만 담는다. 사용 가능 여부는 대상의 CanInteract에서 따로 조회한다.
USTRUCT(BlueprintType)
struct TESTRRR_API FInteractionInfo
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Interaction")
	FText Prompt;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Interaction")
	FText UnavailableReason;
};

// 실제 실행 결과이며, 기본값은 실패이므로 성공한 구현에서 명시적으로 설정한다.
USTRUCT(BlueprintType)
struct TESTRRR_API FInteractionResult
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Interaction")
	bool bSucceeded = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Interaction")
	FText FailureReason;
};
