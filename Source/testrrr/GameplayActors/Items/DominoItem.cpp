#include "GameplayActors/Items/DominoItem.h"
#include "Components/StaticMeshComponent.h"
#include "PhysicalMaterials/PhysicalMaterial.h"

ADominoItem::ADominoItem()
{
	// 기본 Cube + Box Collision. 이후 에디터/자식 BP의 크기를 자동으로 덮어쓰지 않는다.
	SetDefaultItemDimensions(FVector(12.f, 45.f, 90.f));
	ItemMass = 2.f;
	Mesh->SetLinearDamping(0.05f);
	Mesh->SetAngularDamping(0.1f);
	Mesh->SetUseCCD(true);
	UPhysicalMaterial* Material = CreateDefaultSubobject<UPhysicalMaterial>(TEXT("DominoPhysicsMaterial"));
	Material->Friction = 0.65f;
	Material->Restitution = 0.f;
	Mesh->SetPhysMaterialOverride(Material);
}
