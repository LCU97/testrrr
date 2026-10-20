#include "GameplayActors/Switches/ItemSpawnSwitch.h"
#include "GameplayActors/Items/BoxItem.h"
#include "Components/ArrowComponent.h"
#include "Engine/World.h"

AItemSpawnSwitch::AItemSpawnSwitch()
{
	// 부모의 1배율 SwitchRoot에 붙여 메시 크기와 독립적으로 배치한다.
	SpawnPoint = CreateDefaultSubobject<UArrowComponent>(TEXT("SpawnPoint"));
	SpawnPoint->SetupAttachment(SwitchRoot);
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
