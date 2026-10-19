# C++ 배치용 아이템과 생성 스위치

블루프린트 생성 없이 C++ 클래스를 직접 레벨에 배치한다.

## 클래스 위치

- `C++ Classes / testrrr / GameplayActors / Items`: WoodenBlockRectangle, WoodenBlockTriangle, ChairItem, BoxItem.
- `C++ Classes / testrrr / GameplayActors / Switches`: ItemSpawnSwitch.
- PlacementItem은 공통 설정을 묶는 추상 부모이므로 직접 배치하지 않는다.

## 아이템

기존 PickupActor의 E 줍기 / G 내려놓기를 사용한다. 네 아이템 모두 R을 누르고 있는 동안 월드 수직축 기준 초당 90도로 부드럽게 회전하며, 손을 떼면 목표 회전 갱신이 멈춘다. CarryComponent의 Held Rotation Speed로 속도를 조절한다. 시선의 위아래 움직임 때문에 배치용 아이템이 기울지 않도록 수평을 유지한다. 회전 중에도 벽·바닥의 물리 충돌은 유지한다. 잡는 거리와 화면 가림은 기존 설정을 유지한다.

- 직사각형: 기본 Cube, 전체 크기 100 × 100 × 50cm, 20kg.
- 삼각형: `/Game/LevelPrototyping/Meshes/SM_Ramp`, 전체 크기 100 × 100 × 50cm, 15kg.
- 상자: 기본 Cube, 60 × 60 × 60cm, 5kg.
- 의자: `/Game/Resources/chair1`, 원본 크기, 8kg.

크기는 일반 Transform / Scale 또는 자식 블루프린트의 Mesh Scale로 변경한다. C++ 생성자에서 최초 기본 크기만 지정하며, 편집·재구성·플레이 시작 시 스케일을 덮어쓰지 않는다. Item Mass는 BlueprintReadWrite로 노출한다. 나무 외관용 재질은 아직 별도로 지정하지 않았다. 기존 메시 에셋은 수정하지 않는다.

## 농구공과 볼링공

같은 Items 폴더에서 BasketballItem과 BowlingBallItem을 배치하거나 생성 스위치의 Spawn Class로 선택한다. 별도 BP는 필요 없다.

- BasketballItem: 갈색 무광 구체, 지름 24cm, 게임 감각용 질량 0.9kg, 반발력 0.55. E로 줍고 좌클릭으로 던지며 G로 내려놓는다. 기존 던지기 동작과 플레이어 힘 배율을 사용한다.
- BowlingBallItem: 짙은 청남색 광택 구체, 지름 22cm, 게임 감각용 질량 30kg, 반발력 0.02. 줍기·던지기는 금지하며 중력에 의해 떨어지고 경사면을 따라 굴러간다. 생성 스위치의 SpawnPoint를 천장 쪽으로 옮기면 낙하 시험이 가능하다.
- Ball 항목의 Mass Kg, Ball Color, Roughness, Ball Material은 BlueprintReadWrite로 노출한다. 크기는 일반 Transform / Scale 또는 자식 블루프린트의 Mesh Scale로 변경한다. 공은 구형 충돌을 유지하려면 XYZ를 같은 비율로 조절한다.
- 기존 M_FlatCol의 Base Color/Roughness 매개변수를 사용하는 개별 동적 재질이다. 원본 재질과 기존 메시 에셋은 수정하지 않는다. 현재 외형은 단색 구체이며 농구공 줄무늬·볼링공 손가락 구멍은 포함하지 않는다.

## 생성 스위치 설정

1. ItemSpawnSwitch를 레벨에 배치한다.
2. Details / Spawning / Spawn Class에서 생성할 C++ 클래스를 선택한다. 기본값은 BoxItem이다.
3. 컴포넌트에서 SpawnPoint 화살표를 선택하고 상대 위치·회전을 조절한다. 스위치를 옮기거나 돌리면 화살표도 따라간다.
4. 플레이 중 스위치를 바라보고 E를 누른다. 화살표의 월드 위치와 방향으로 아이템이 생성된다. 기본 재사용 간격은 0.2초다.

Spawning / Spawn Collision Handling에서 충돌 시 생성 정책을 선택한다. 기본값은 Always Spawn이며 플레이어나 물체가 생성 위치를 막아도 그대로 생성한다. 겹친 물리 물체는 생성 후 충돌로 밀려날 수 있다. 겹침을 피하려면 Adjust If Possible But Dont Spawn If Colliding을 선택한다. 반복 생성은 허용하며 이전 아이템은 자동 삭제하지 않는다. 기본 버튼 외형을 그대로 사용한다.

## 검증

Unreal Automation: `HundredButtons.Placement`.
아이템 메시의 단순 충돌·물리·줍기·회전·내려놓기, 두 블록의 실제 크기 일치, 회전된 스위치의 화살표를 통한 생성·쿨다운·재시도를 검증한다.

새 네이티브 클래스와 입력 속성을 추가했으므로 에디터가 이미 실행 중이었다면 완전히 닫고 다시 열어 사용한다.

캐릭터 접촉으로 물체가 과하게 날아가지 않도록 공통 밀기 힘을 낮췄다. 이 설정은 공을 포함한 모든 물리 물체와의 캐릭터 접촉에 적용된다. 기존 배치 인스턴스에서 Mass Kg를 직접 수정했다면 기본값으로 재설정해야 새 질량을 사용한다.
