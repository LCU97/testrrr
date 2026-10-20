#include "GameplayActors/Items/PlacementItem.h"
#include "Interaction/CarryableComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "PhysicalMaterials/PhysicalMaterial.h"

APlacementItem::APlacementItem()
{
	Carryable->bCanRotateWhileCarried = true;
	Carryable->bCanThrow = false;
	Mesh->SetMobility(EComponentMobility::Movable);
	Mesh->SetLinearDamping(0.1f);
	Mesh->SetAngularDamping(0.5f);
	UPhysicalMaterial* Material = CreateDefaultSubobject<UPhysicalMaterial>(TEXT("ItemPhysicsMaterial"));
	Material->Friction = 0.8f;
	Material->Restitution = 0.05f;
	Mesh->SetPhysMaterialOverride(Material);
}

void APlacementItem::SetDefaultItemDimensions(const FVector& Dimensions)
{
	if (UStaticMesh* Asset = Mesh->GetStaticMesh(); Asset && Dimensions.GetMin() > 0.f)
	{
		const FVector Size = Asset->GetBoundingBox().GetSize();
		if (Size.GetMin() > UE_SMALL_NUMBER)
		{
			Mesh->SetRelativeScale3D(Dimensions / Size);
		}
	}
}

void APlacementItem::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	Mesh->SetMassOverrideInKg(NAME_None, FMath::Max(0.1f, ItemMass));
}
