# Unreal 맵 저장 정책

## 새 맵의 기본 방식

게임플레이 및 프로토타입 맵은 액터 데이터를 하나의 `.umap` 파일에 저장하는 일반 레벨 방식으로 만든다.

- **File → New Level**에서 **Basic** 또는 **Empty Level**을 선택한다.
- **Open World** 템플릿은 선택하지 않는다.
- 처음 저장하기 전에 표시되는 경우 **Enable World Partition**과 **Use External Actors**를 끈다.
- 맵은 `Content/` 아래에 저장하며, 저장 후 대응되는 `Content/__ExternalActors__/...` 폴더가 생성되지 않았는지 확인한다.

## 기존 World Partition 맵

이미 World Partition 또는 One File Per Actor 방식을 사용하는 맵은 `__ExternalActors__` 파일을 직접 삭제해서 변환하지 않는다. 새 일반 맵을 만든 뒤 필요한 액터를 복사하고, 저장 및 동작을 검증한 후 기존 맵과 외부 액터 폴더를 제거한다.

## 소스 관리

일반 맵은 `.umap` 파일과 의도적으로 만든 일반 에셋만 커밋한다. `Saved/`, `Intermediate/`, `DerivedDataCache/` 같은 자동 생성 에디터 캐시는 커밋하지 않는다.
