# 아이템 사용과 던지기 연동 안내

## 현재 조작

- E: 빈손일 때 잡기·스위치 상호작용. 운반 중에는 계속 차단한다.
- 좌클릭: 현재 들고 있는 아이템에 설정된 사용 동작 실행.
- G: 일반 내려놓기. 던지기와 별도 행동이다.

Enhanced Input의 IA_UseItem을 C++에서 기본 생성하여 기존 상호작용 Mapping Context에 연결했다. Started는 사용 시작, Completed는 정상 종료, Canceled는 입력 취소다. 유지 효과는 ItemUseComponent의 Tick으로 갱신하므로 Triggered에 효과를 중복 연결하지 않는다. 선택적으로 캐릭터 BP의 UseItemAction에 Boolean 에셋을 지정할 수 있으며, 이번 기본 사용 방식에서는 Hold/Pulse 트리거를 넣지 않는다.

## 추가 파일과 책임

| 파일 묶음 | 역할 |
|---|---|
| ItemUseTypes.h | 사용 문맥·단발/지속 결과·던지기 설정/결과·운반 해제 사유 |
| UsableItem.h | 사용 동작 조회, 사용 조건, 던져진 뒤 반응에 대한 공통 인터페이스 |
| ItemUseBehavior.h / .cpp | C++/BP가 확장하는 사용 전략의 부모 |
| ItemUseComponent.h / .cpp | 플레이어의 사용 입력, 사용 대상 고정, 지속 갱신과 종료·취소 |
| ThrowItemUseBehavior.h / .cpp | 사용 방식 중 하나인 단발 던지기 |
| ItemThrowPhysics.h / .cpp | 실제 질량을 사용하는 충격량 계산과 UE 단위 변환 |
| ThrowablePickupActor.h / .cpp | 던지기 전략을 미리 설정한 배치용 아이템 자식 |
| Tests/ItemUseTestTypes.h / .cpp | 효과 대신 호출 횟수·문맥을 기록하는 자동 테스트 전용 객체 |

기존 PickupActor는 IUsableItem을 구현하고 UseBehavior 인스턴스를 소유한다. 기존 CarryComponent는 실제 던지기와 해제 사유를 처리한다. 캐릭터는 좌클릭을 ItemUseComponent에 전달하며, InteractionComponent는 아이템의 사용 안내를 표시한다. 아이템 종류별 입력 분기는 없다.

```text
좌클릭
  → ItemUseComponent
  → 현재 아이템의 IUsableItem 계약 조회
  → 아이템이 소유한 UseBehavior 실행
     ├─ ThrowItemUseBehavior → CarryComponent.TryThrow
     └─ 작업자가 만든 분사·먹기 등의 사용 동작
```

## 아이템 제작자가 설정할 내용

### 사용하는 기능이 없는 물건

APickupActor를 상속하고 UseBehavior를 비워둔다. 잡기와 G 내려놓기는 가능하지만 좌클릭은 아무 효과가 없다. Carryable.bCanThrow가 true라고 자동으로 던져지는 것은 아니다.

### 던지는 물건

방법 A: AThrowablePickupActor를 상속한다. ThrowItemUseBehavior가 기본 설정되어 있고 배치용 큐브는 25cm 크기다. 질량은 실제 물리 설정으로 계산된다.

방법 B: APickupActor 자식 BP의 Item Use / Use Behavior에서 ThrowItemUseBehavior 인스턴스를 선택한다.

실제 메쉬의 물리 질량을 적절하게 설정한다. 큰 기본 큐브를 작은 공처럼 던지고 싶다면 외형뿐 아니라 실제 크기·밀도 또는 Mass Override도 공에 맞춰야 한다. 힘 계산용 가짜 질량 변수는 만들지 않는다. 단순 충돌과 Simulate Physics가 필요하며, 초고속·작은 물건은 상황에 따라 CCD도 검토한다.

- Carryable.bCanThrow: 물리적으로 던지기를 허용하는 제한 조건.
- ThrowSettings.ImpulseNewtonSeconds: 기본 12 N·s의 충격량.
- ThrowSettings.MaxAddedSpeedMetersPerSecond: 기본 18m/s의 추가 속도 상한.
- 플레이어 CarryComponent.ThrowStrengthMultiplier: 기본 4배. 아이템의 충격량에 곱하는 플레이어의 던지는 힘이다. 이미 저장된 아이템의 충격량을 직접 변경하지 않고 적용된다.
- bItemUseEnabled / CanUseItem: 아이템 자체의 사용 허용 여부와 문맥별 조건.

### 던진 뒤 반응이 있는 물건

아이템 자식에서 OnItemThrown(Info)를 구현한다. 이 이벤트는 물리 핸들 해제와 충격량 적용 요청이 성공한 뒤 한 번 호출된다. Info에 던진 주체, 실제 질량, 정규화 방향, 적용 충격량과 추가 속도가 담긴다.

폭탄의 예:

1. OnItemThrown에서 자신의 점화 상태를 확인한다.
2. 아직 점화되지 않았다면 자신의 타이머를 시작한다.
3. 타이머 종료 시 폭탄 자신이 폭발 효과를 실행한다.
4. 다시 잡거나 재차 던져도 타이머를 초기화할지는 폭탄이 결정한다. 초기화를 원하지 않으면 점화 상태로 중복 시작을 막는다.

이번 작업은 공통 프레임워크와 던지기까지다. 실제 폭탄의 폭발·소화기의 분사·먹기 콘텐츠는 해당 아이템 작업자가 구현한다. 기본 물건의 OnItemThrown은 비어 있어 물리적으로 날아가기만 한다.

## 새로운 도구 사용 동작 만들기

UItemUseBehavior를 C++ 또는 BP로 상속하고 아이템의 UseBehavior로 지정한다. 동작은 아이템마다 별도 인스턴스를 소유해야 한다. 여러 아이템이 하나의 상태 있는 동작 객체를 공유하지 않는다.

| 함수 | 구현 규칙 |
|---|---|
| CanUse | 연료·상태 등의 조회만 수행한다. 효과나 소모를 실행하지 않는다. |
| BeginUse | 효과 시작. 실패는 bSucceeded=false. 단발 성공은 true/false, 지속 성공은 true/true를 반환한다. |
| UpdateUse | 지속 사용 중 매 프레임 호출. DeltaTime으로 초당 소모·효과량을 계산한다. |
| EndUse | 클릭을 뗐을 때 분사·소리·타이머 등 지속 효과를 멈춘다. |
| CancelUse | 내려놓기·파괴·빙의 해제·사용 불가 등으로 중단될 때 정리한다. 기본 구현은 EndUse를 호출한다. |

분사 예: BeginUse에서 분사 효과 시작 → bSucceeded=true, bKeepUsing=true → UpdateUse에서 연료를 초당 소모하고 현재 AimDirection으로 효과 적용 → EndUse에서 효과 중지. 연료가 바닥나면 CanUse가 false가 되어 CancelUse로 정리된다.

UsePrompt에는 키 이름을 제외한 행동명(분사, 먹기 등)을 설정한다. 사용 가능하면 화면에 사용 키와 G 안내가 함께 표시된다. 외부 상태만 바뀌었을 때 안내를 즉시 갱신하려면 플레이어 ItemUseComponent의 NotifyUseAvailabilityChanged를 호출한다. 실제 실행에서는 언제나 조건을 다시 검사한다.

### 사용 수명주기

- 클릭 시작 시 아이템과 전략 객체를 저장한다. 클릭 종료를 현재 손에 든 다른 물건으로 전달하지 않는다.
- 물건을 내려놓거나 잃으면 진행 중 사용을 취소한다. 같은 누름을 유지한 채 다른 물건을 잡아도 사용을 자동 시작하지 않는다.
- BeginUse 중에 내려놓기·파괴가 발생하면 재진입 취소를 보류한 뒤 정리한다.
- 던지기는 BeginUse 안에서 운반을 해제하지만 단발 성공이므로 뒤늦은 EndUse/CancelUse를 중복 호출하지 않는다.
- 시작에 실패해도 준비 중 생성한 효과가 있을 수 있으므로 CancelUse가 호출된다. 취소 정리는 여러 상태에서 안전하게 실행되도록 작성한다.
- CancelUse 문맥의 아이템은 이미 파괴 중일 수 있다. 아이템에 접근할 때 IsValid를 확인하고, 외부 소리·효과 등의 정리는 안전하게 수행한다.
- 사용 중에는 CanUseItem과 전략의 CanUse를 다시 확인한다. 일반 조회 함수는 상태를 바꾸지 않는다.

## 물리 계산과 해제 구분

```text
m = 현재 운반 중인 PrimitiveComponent.GetMass()   [kg]
J = min(아이템 충격량 × 플레이어 힘 배율,
        m × 최대 추가 속도)                     [N·s]
추가 속도 = J / m                                [m/s]
UE 충격량 = 정규화한 던지기 방향 × J × 100       [kg·cm/s]
```

Physics Handle을 해제하고 기존 선속도·각속도를 0으로 정리한 뒤 AddImpulse(UE 충격량, NAME_None, false)를 호출한다. 고개를 빠르게 돌릴 때 물리 핸들이 만들던 속도가 발사에 더해지거나 반대 방향으로 상쇄되지 않게 하기 위한 처리다. bVelChange=false이므로 질량 차이는 실제 발사 속도에 그대로 반영된다. 기존 MaxAddedSpeedMetersPerSecond 필드명은 BP 호환을 위해 유지하지만, 이제 정지 상태에서 충격량을 적용하므로 발사 속도의 상한으로 작동한다. 발사 후 충돌·중력·외부 힘에 의한 속도 변화는 일반 물리가 처리한다. 0·음수·비정상 질량/설정과 길이가 0인 방향은 적용 전에 거부한다.

바닥·벽과의 접촉을 발사 거부 조건으로 사용하지 않는다. 정상적인 접촉 오차까지 겹침으로 판정되어 클릭이 무시되는 문제를 방지하기 위한 정책이다. 물건을 순간이동시키거나 지형 충돌을 끄지 않고, 발사 후 주변 지형과의 접촉은 엔진 물리가 처리한다.

발사 직후 Pawn 충돌은 최소 0.15초 유예하고, 실제 물리 형상이 던진 캐릭터의 몸 밖으로 나온 뒤 원래 응답으로 복원한다. 기존 싱글플레이 운반 구조와 동일하게 Pawn 채널 응답을 사용한다. 복원 대기 중 다시 잡으면 원래 응답을 이어받고 이전 복원 예약을 해제한다. 캐릭터가 종료되면 남은 복원 기록도 정리한다. 아이템이 직접 다른 충돌 응답을 설정한 경우에는 그 설정을 덮어쓰지 않는다. G 내려놓기는 일반 해제 시 즉시 원래 응답으로 복원한다.

아이템 충격량 12 N·s, 플레이어 힘 배율 4, 발사 속도 상한 18m/s의 예:

| 실제 질량 | 추가 속도 |
|---|---:|
| 0.6kg | 18m/s (상한 적용) |
| 2kg | 18m/s (상한 적용) |
| 7.86kg | 약 6.1m/s |
| 10kg | 4.8m/s |
| 20kg | 2.4m/s |

맵에 배치된 기본 25cm 큐브는 실제 질량이 약 7.86kg이었다. 이전 충격량 12 N·s만으로는 1.53m/s밖에 나오지 않아 놓는 느낌이 났다. 기본 힘 배율 4는 이 설정을 기준으로 조정한 값이며, 실제 질량을 변경하거나 무시하지 않는다. 가벼운 물건의 속도 상한도 유지한다.

기존 Carryable.OnReleased는 유지한다. 이유가 필요한 경우 OnReleasedWithReason(Context, Reason) 또는 LastReleaseReason을 사용한다.

- Drop: G로 일반 내려놓기.
- Throw: 던지기를 위한 해제. 최종 성공 후속 효과는 OnItemThrown에서 처리한다.
- Forced: 물건 파괴, 거리 초과, 빙의 해제, 종료 등의 강제 정리.

OnReleased와 OnReleasedWithReason은 물건이 손에서 풀렸다는 알림이다. 던지기 충격량 적용 전에 호출되므로 이 알림만으로 점화 성공을 판단하지 않는다. 콜백 도중 아이템을 파괴하거나 물리를 끄면 실제 던지기는 실패하고 OnItemThrown은 호출되지 않는다.

## 검증과 범위

Unreal Automation의 HundredButtons를 실행하면 기존 상호작용 테스트와 신규 사용 테스트를 함께 실행한다.

- 사용 동작 없는 아이템, 비던지기 동작 선택, 던지기 제한
- 단발·지속 사용, 현재 시선 갱신, 정상 종료·취소, 아이템 교체 및 파괴
- 시작 실패와 시작 도중 내려놓기
- 질량별 식, 추가 속도 상한과 UE 단위 변환
- 실제 엔진 물리에서 10kg·20kg 물건의 속도 차이 및 성공 이벤트
- G 내려놓기·강제 해제·실패한 던지기에서 성공 이벤트가 발생하지 않는지
- 운반 중 잔여 속도와 고개 방향에 무관한 발사 속도, 각속도 정리
- 바닥 접촉·지형/몸 겹침 상태에서도 클릭 한 번으로 발사, 지형 충돌 유지
- 몸 충돌 유예 후 복원, 빠른 재잡기, 캐릭터 종료 및 외부 충돌 설정 변경
- 실제 배치된 아이템 에셋을 읽어 설정 그대로 복제하여 질량·속도 확인 (에셋 저장 없음)
- 중력을 켠 채 1초간 들고 난 뒤 기본 예제의 발사 속도 확인

싱글플레이용이며 멀티플레이 복제·서버 권한 판정은 포함하지 않는다. 실제 화면에서 입력 감각과 물건별 크기·질량·충돌 설정은 에디터 플레이로 조정한다. 회전·거리 조절은 이번 범위에 포함하지 않는다.
