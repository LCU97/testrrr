#include "GameplayActors/Items/BowlingBallItem.h"
#include "Interaction/CarryableComponent.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "Components/StaticMeshComponent.h"

ABowlingBallItem::ABowlingBallItem()
{
	Mesh->SetRelativeScale3D(FVector(0.22f));
	MassKg = 30.f;
	BallColor = FLinearColor::FromSRGBColor(FColor(22, 35, 88));
	Roughness = 0.2f;
	BallPhysicsMaterial->Friction = 0.3f;
	BallPhysicsMaterial->Restitution = 0.02f;
	Mesh->SetLinearDamping(0.08f);
	Mesh->SetAngularDamping(0.1f);
	BallPhysicsMaterial->bOverrideRestitutionCombineMode = true;
	BallPhysicsMaterial->RestitutionCombineMode = EFrictionCombineMode::Min;
	Carryable->bCanBeCarried = false;
	Carryable->bCanThrow = false;
	bItemUseEnabled = false;
	DisabledReason = NSLOCTEXT("BallItem", "BowlingNotCarryable", "볼링공은 집을 수 없습니다");
}
