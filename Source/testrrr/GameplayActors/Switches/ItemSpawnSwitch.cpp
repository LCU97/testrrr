#include "GameplayActors/Switches/ItemSpawnSwitch.h"
#include "GameplayActors/Items/BoxItem.h"
#include "Components/ArrowComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"

AItemSpawnSwitch::AItemSpawnSwitch()
{
	// 별도 루트를 두어 버튼 메시의 축소 비율이 생성 지점에 적용되지 않도록 한다.
	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("SwitchRoot"));
	SetRootComponent(Root);
	Mesh->SetupAttachment(Root);
	SpawnPoint = CreateDefaultSubobject<UArrowComponent>(TEXT("SpawnPoint"));
	SpawnPoint->SetupAttachment(Root);
	SpawnPoint->SetRelativeLocation(FVector(150.f, 0.f, 100.f));
	SpawnPoint->ArrowColor = FColor::Green;
	SpawnClass = ABoxItem::StaticClass();
}

bool AItemSpawnSwitch::CanInteract_Implementation(const FInteractionContext& Context) const
{
	return Super::CanInteract_Implementation(Context) && SpawnClass
		&& !SpawnClass->HasAnyClassFlags(CLASS_Abstract);
}

bool AItemSpawnSwitch::Activate_Implementation(const FInteractionContext& Context)
{
	if (!GetWorld() || !SpawnClass || SpawnClass->HasAnyClassFlags(CLASS_Abstract))
	{
		return false;
	}
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = SpawnCollisionHandling;
	LastSpawnedActor = GetWorld()->SpawnActor<AActor>(SpawnClass, SpawnPoint->GetComponentLocation(),
		SpawnPoint->GetComponentRotation(), Params);
	return LastSpawnedActor.IsValid();
}
