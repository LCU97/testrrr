#include "GameplayActors/Items/WoodenBlockTriangle.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"

AWoodenBlockTriangle::AWoodenBlockTriangle()
{
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Ramp(TEXT("/Game/LevelPrototyping/Meshes/SM_Ramp.SM_Ramp"));
	if (Ramp.Succeeded())
	{
		Mesh->SetStaticMesh(Ramp.Object);
	}
	ItemMass = 15.f;
	SetDefaultItemDimensions(FVector(150.f, 150.f, 75.f));
}
