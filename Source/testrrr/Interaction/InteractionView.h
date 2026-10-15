/*
 * 탐색과 운반이 공유하는 시점 조회
 * 플레이어는 실제 카메라 시점을 사용하고, 컨트롤러가 없는 액터는 기본 눈 위치를 사용한다.
 * 탐색 방향과 물건을 들고 있는 방향이 같은 기준을 따르도록 한다.
 */

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

namespace InteractionView
{
	inline void GetViewPoint(const AActor* Owner, FVector& Location, FRotator& Rotation)
	{
		const APawn* Pawn = Cast<APawn>(Owner);
		if (const APlayerController* PC = Pawn ? Cast<APlayerController>(Pawn->GetController()) : nullptr)
		{
			// 캐릭터 원점이나 메쉬 방향 대신 플레이어가 실제로 보고 있는 카메라를 기준으로 삼는다.
			PC->GetPlayerViewPoint(Location, Rotation);
			return;
		}
		// 컨트롤러가 없는 테스트 액터 등은 엔진의 기본 눈 위치를 사용한다.
		Owner->GetActorEyesViewPoint(Location, Rotation);
	}
}
