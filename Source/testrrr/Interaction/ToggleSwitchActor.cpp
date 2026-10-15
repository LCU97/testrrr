/*
 * 토글 스위치 효과 구현
 * 부모가 입력 요청과 쿨다운을 처리하므로 자식은 상태 변경과 알림에만 집중한다.
 */

#include "Interaction/ToggleSwitchActor.h"

bool AToggleSwitchActor::Activate_Implementation(const FInteractionContext& Context)
{
	// 부모가 실행 가능 여부를 확인했으므로 여기서는 토글 효과만 처리한다.
	bIsOn = !bIsOn;
	// 실제 문 열기·조명 변경 등은 이 알림을 구독한 쪽에서 수행한다.
	OnToggled.Broadcast(bIsOn);
	return true;
}
