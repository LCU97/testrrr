/*
 * 던지기 계산식 구현
 * J적용 = min(J설정, 질량 × 최대추가속도), 추가속도 = J적용 / 질량.
 * 기본 UE 물리 단위는 cm이므로 N·s에서 kg·cm/s로 바꿀 때 100을 곱한다.
 */

#include "Interaction/ItemThrowPhysics.h"

bool ItemThrowPhysics::CalculateImpulse(const FItemThrowSettings& Settings, float MassKg, const FVector& Direction,
	FItemThrowInfo& OutInfo, FVector& OutImpulseUE)
{
	OutInfo = FItemThrowInfo();
	OutImpulseUE = FVector::ZeroVector;
	if (!FMath::IsFinite(MassKg) || MassKg <= UE_SMALL_NUMBER || Direction.ContainsNaN()
		|| !FMath::IsFinite(Settings.ImpulseNewtonSeconds) || Settings.ImpulseNewtonSeconds <= 0.f
		|| !FMath::IsFinite(Settings.MaxAddedSpeedMetersPerSecond) || Settings.MaxAddedSpeedMetersPerSecond <= 0.f)
	{
		return false;
	}
	const FVector Aim = Direction.GetSafeNormal();
	if (Aim.IsNearlyZero())
	{
		return false;
	}
	// 속도를 직접 지정하지 않고, 같은 충격량에 대한 질량 차이를 엔진 물리에 반영한다.
	const double ImpulseNs = FMath::Min(static_cast<double>(Settings.ImpulseNewtonSeconds),
		static_cast<double>(MassKg) * Settings.MaxAddedSpeedMetersPerSecond);
	OutInfo.MassKg = MassKg;
	OutInfo.Direction = Aim;
	OutInfo.AppliedImpulseNewtonSeconds = ImpulseNs;
	OutInfo.AddedSpeedMetersPerSecond = ImpulseNs / MassKg;
	OutImpulseUE = Aim * ImpulseNs * 100.0;
	return true;
}
