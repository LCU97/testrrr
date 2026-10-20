# 100 BUTTONS 상호작용 연동 안내

## 사용 방법

기존 `AtestrrrCharacter`와 그 자식 BP에 C++ 컴포넌트가 추가된다. 기존 이동·점프 입력은 유지된다.

- E: 현재 바라보는 대상에 상호작용 요청. 운반 중에는 전부 차단.
- G: 현재 운반 중인 물건 내려놓기. 빈손이면 아무 동작 없음.
- 왼쪽 Shift + 마우스: 들고 있는 물건 수평/수직 회전. 회전 중에는 마우스 시점 회전을 차단하고, Shift 해제나 물건 해제 시 자동 복구한다. R 회전은 제거했다. 선택한 물건 자세는 카메라를 돌린 뒤에도 유지한다.
- 대상 검사: 기본 거리 300cm, 반경 10cm, 0.2초 간격 Visibility Sphere Trace. E 입력 시 즉시 재검사. 최초 차단 충돌만 사용하므로 벽 뒤의 물건은 선택하지 않는다.
- 캐릭터의 InteractionComponent에서 Interaction / Trace / Trace Radius로 감지 반경을 조절한다. EditAnywhere, BlueprintReadWrite로 노출한다.
- Interaction / Debug / Draw Debug Trace를 켜면 DrawDebug 함수로 스피어가 지나가는 부피(캡슐), 충돌 시 스피어와 실제 접촉점을 표시한다. 녹색은 상호작용 액터 감지, 빨간색은 대상 없음 또는 일반 장애물, 노란 점은 접촉점이다. 사용 가능 여부와 별개로 대상 종류만 색으로 구분한다. 기본값은 꺼짐이며 EditAnywhere, BlueprintReadWrite로 런타임 토글도 가능하다. 표시는 탐색 주기 동안 잠시 남고, 운반 중 탐색이 멈추면 사라진다.
- 안내: 화면 높이 43% 위치. 사용 가능하면 `[E] 잡기` 또는 `[E] 누르기`, 운반 중에는 `[G] 내려놓기`.
- 기본 키는 캐릭터 BP의 Input / Interaction에서 바꿀 수 있다. 안내의 키 이름도 함께 변경된다.

Enhanced Input의 Boolean Input Action과 Mapping Context를 C++에서 기본 생성한다. 별도 입력 에셋을 만들거나 기존 BP에 E/G 이벤트를 추가할 필요가 없다. 선택적으로 `InteractAction` / `DropAction` 에셋을 지정할 수 있다. 이번 단발 입력용 에셋에는 Hold/Pulse 트리거를 넣지 않는다. 같은 행동을 기존 BP에서 중복 처리하지 않는다.

## 상속 관계와 협업 경계

```text
AActor
└─ AInteractableActor (추상)
   ├─ APickupActor
   │  └─ 개별 아이템 C++ / Blueprint
   └─ AButtonSwitchActor (추상)
      ├─ AToggleSwitchActor (배치 가능한 토글 예제)
      └─ 개별 스위치 C++ / Blueprint
```

플레이어 쪽에는 아이템 이름·스위치 종류에 따른 분기가 없다. 액터가 IInteractable 공통 계약을 구현한다. 입력은 캐릭터만 받고 대상은 함수 호출을 받는다. 대상이 직접 EnableInput 하거나 E/G 이벤트를 만들지 않는다.

### 아이템 작업자

1. APickupActor를 상속한다. 배치 가능한 기본 큐브가 있으므로 해당 클래스 자체도 잡기 확인에 사용할 수 있다.
2. Mesh를 원하는 Static Mesh로 교체한다. 단순 충돌, Movable, Simulate Physics, Query and Physics가 필요하다. Visibility를 Block해야 탐색된다.
3. Carryable의 `bCanBeCarried`, `bCanThrow`를 설정한다. 메시가 여러 개라면 CarryPrimitive에 실제 운반할 물리 컴포넌트를 지정한다.
4. 특별한 운반 조건은 CarryableComponent 자식의 `CanGrab` / `CanDrop` / `CanThrow`로 확장한다. 기본 제약도 유지하려면 부모 함수를 호출한다.
5. `OnGrabbed` / `OnReleased`는 알림용 확장점이다. CurrentCarrier는 프레임워크가 관리하므로 직접 변경하지 않는다.

잡기·놓기는 Physics Handle로 수행한다. 월드 충돌을 유지하고 운반 중 Pawn 채널만 무시한 뒤 원래 응답을 복원한다. 이 MVP는 1인칭 싱글플레이용이며, 여러 플레이어에 대한 복제·권한 처리는 포함하지 않는다. 물건이 파괴되거나 물리 시뮬레이션이 꺼지거나 플레이어와 400cm 이상 벌어지면 강제 해제한다.

`bCanThrow=false`는 내려놓기를 막지 않는다. 좌클릭 사용과 던지기는 후속 구현인 [아이템 사용 프레임워크](ItemUseFramework.md)에 설명되어 있다. UseBehavior가 던지기로 설정된 물건만 좌클릭으로 던져지며, 실제 실행은 CarryComponent의 `CanThrowHeldObject()`를 통과해야 한다.

### 스위치 작업자

1. AButtonSwitchActor를 상속한다.
2. `Activate(Context)`를 재정의하여 오브젝트 생성, 조명 변경 등 실제 효과를 실행한다. 효과 요청을 받아들였을 때만 true를 반환한다.
3. 잠금 등 조건은 `CanInteract(Context)`를 재정의한다. 기본 활성화·쿨다운 조건을 유지하려면 부모 결과와 추가 조건을 AND로 연결한다.
4. 성공 후 기본 0.2초 쿨다운이 적용된다. Cooldown으로 조절할 수 있다.

AToggleSwitchActor는 E로 bIsOn을 전환하고 OnToggled 이벤트를 보낸다. BP/레벨 BP에서 해당 이벤트를 문·조명에 연결할 수 있다. 생성 스위치에 토글 동작을 강요하지 않으며, 생성 스위치는 AButtonSwitchActor에서 직접 상속한다.

### 공통 계약

- `GetInteractionInfo(Context)`: Prompt와 UnavailableReason 조회. 상태를 변경하지 않는다.
- `CanInteract(Context)`: 사용 가능 여부 조회. 상태를 변경하지 않는다.
- `Interact(Context)`: 실제 실행 및 성공/실패 반환. 기본 Pickup/Switch 구현은 실행 직전 조건을 다시 확인한다.
- Context: Interactor와 실제 HitResult를 전달한다. 메시 이름에 의존하지 않고 Hit.Component, Hit.ImpactPoint 등이 필요한 경우에만 사용한다.
- UI의 사용 가능 여부는 CanInteract에서 계산한다. 안내 조회와 실행 조건의 기준을 동일하게 유지한다.

기본 액터의 InteractionPrompt / DisabledReason / bInteractionEnabled를 편집할 수 있다. 물건을 들고 있을 때의 전역 E 차단은 InteractionComponent가 담당한다. 개별 대상은 플레이어 입력을 직접 실행하는 대신 공통 진입점으로 요청해야 한다.

## UI 교체

기본 C++ UInteractionPromptWidget이 별도 BP 없이 생성된다. 자체 UI를 쓰려면 InteractionComponent의 bShowDefaultPrompt를 끄고 OnPromptChanged에 연결하거나, PromptWidgetClass를 자식 위젯으로 지정하여 SetPrompt를 재정의한다. 기본 키 안내는 E/G 키 변경 설정을 반영하며, 별도 패드 매핑 추가 시에는 해당 장치용 안내도 추가해야 한다.

## 에디터에서 확인

1. C++ 빌드 후 기존 FirstPerson 맵을 열고 APickupActor와 AToggleSwitchActor를 시작점 앞에 배치한다.
2. 빈손으로 버튼을 바라보면 안내가 보이며 E 한 번에 토글 한 번이 실행된다.
3. 상자를 E로 잡고 버튼을 보아도 E는 작동하지 않는다. G로 내려놓으면 다시 작동한다.
4. 벽 뒤나 300cm 밖의 대상은 선택되지 않는다.
5. Carryable의 bCanThrow를 꺼도 잡기·내려놓기는 가능하다.

맵·BP 에셋은 자동 변경하지 않는다. 배치용 액터는 편집기의 C++ Classes / testrrr / Interaction에서 찾거나 해당 클래스를 부모로 BP를 만든다.

## 자동 검증

Unreal Automation: `HundredButtons.Interaction`

- 0.2초 대상 갱신, E 입력 시 최신 조준 재검사, 쿨다운·비활성화
- 일반 벽에 의한 차폐, 상호작용 거리
- 잡기, 운반 중 E 차단, 내려놓기와 충돌 복원, 던지기 허용 설정, 운반 물건 삭제 시 상태 복구

자동 검증은 게임 로직 대상이다. 실제 화면 배치, 물건 크기별 운반 감각, 기존 BP의 중복 키 사용 여부는 에디터 플레이에서 확인한다.
