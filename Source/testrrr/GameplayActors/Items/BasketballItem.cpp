#include "GameplayActors/Items/BasketballItem.h"
#include "Interaction/CarryableComponent.h"
#include "Interaction/ThrowItemUseBehavior.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "Components/StaticMeshComponent.h"

ABasketballItem::ABasketballItem()
{
	MassKg = 0.9f;
	BallColor = FLinearColor::FromSRGBColor(FColor(151, 78, 35));
	Roughness = 0.8f;
	BallPhysicsMaterial->Friction = 0.6f;
	BallPhysicsMaterial->Restitution = 0.55f;
	Mesh->SetLinearDamping(0.12f);
	Mesh->SetAngularDamping(0.15f);
	BallPhysicsMaterial->bOverrideRestitutionCombineMode = true;
	BallPhysicsMaterial->RestitutionCombineMode = EFrictionCombineMode::Max;
	Carryable->bCanBeCarried = true;
	Carryable->bCanThrow = true;
	UThrowItemUseBehavior* ThrowBehavior = CreateDefaultSubobject<UThrowItemUseBehavior>(TEXT("ThrowUseBehavior"));
	// 기존 플레이어의 힘 배율(기본 4배)을 그대로 사용한다.
	ThrowBehavior->ThrowSettings.ImpulseNewtonSeconds = 3.f;
	ThrowBehavior->ThrowSettings.MaxAddedSpeedMetersPerSecond = 10.f;
	UseBehavior = ThrowBehavior;
}
