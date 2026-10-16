/*
 * 던지기의 질량·충격량 계산
 * 입력 설정은 SI 단위로 받고 실제 AddImpulse에 전달할 값만 kg·cm/s로 변환한다.
 * 월드나 액터 상태를 변경하지 않는 순수 계산으로 물리 적용과 검증이 같은 식을 사용한다.
 */

#pragma once

#include "CoreMinimal.h"
#include "ItemUseTypes.h"

namespace ItemThrowPhysics
{
	// 잘못된 질량·방향·설정은 실패한다. 성공 시 정규화 방향과 계산 결과를 함께 반환한다.
	TESTRRR_API bool CalculateImpulse(const FItemThrowSettings& Settings, float MassKg, const FVector& Direction,
		FItemThrowInfo& OutInfo, FVector& OutImpulseUE);
}
