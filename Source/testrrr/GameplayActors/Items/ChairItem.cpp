#include "GameplayActors/Items/ChairItem.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"

AChairItem::AChairItem()
{
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Chair(TEXT("/Game/Resources/chair1.chair1"));
	if (Chair.Succeeded())
	{
		Mesh->SetStaticMesh(Chair.Object);
	}
	ItemMass = 8.f;
}
