#include "GameplayActors/Items/BallItem.h"
#include "Interaction/CarryableComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "UObject/ConstructorHelpers.h"

ABallItem::ABallItem()
{
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Sphere(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Material(TEXT("/Game/LevelPrototyping/Materials/M_FlatCol.M_FlatCol"));
	if (Sphere.Succeeded()) { Mesh->SetStaticMesh(Sphere.Object); }
	// 기본 Sphere 지름은 100cm. 농구공 기본 지름은 50cm로 사용한다.
	Mesh->SetRelativeScale3D(FVector(0.5f));
	if (Material.Succeeded())
	{
		BallMaterial = Material.Object;
		Mesh->SetMaterial(0, BallMaterial);
	}
	BallPhysicsMaterial = CreateDefaultSubobject<UPhysicalMaterial>(TEXT("BallPhysicsMaterial"));
	Mesh->SetPhysMaterialOverride(BallPhysicsMaterial);
	Mesh->SetEnableGravity(true);
	Mesh->SetUseCCD(true);
	Mesh->SetLinearDamping(0.02f);
	Mesh->SetAngularDamping(0.03f);
	Carryable->bCanRotateWhileCarried = true;
}

void ABallItem::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	ApplyBallSettings();
}

void ABallItem::BeginPlay()
{
	Super::BeginPlay();
	// 저장한 맵을 다시 열어 실행해도 각 공의 동적 색상을 복원한다.
	ApplyBallSettings();
}

void ABallItem::ApplyBallSettings()
{
	Mesh->SetMassOverrideInKg(NAME_None, FMath::Max(0.01f, MassKg));
	if (BallMaterial)
	{
		UMaterialInstanceDynamic* ColorMaterial = Mesh->CreateDynamicMaterialInstance(0, BallMaterial);
		if (ColorMaterial)
		{
			ColorMaterial->SetVectorParameterValue(TEXT("Base Color"), BallColor);
			ColorMaterial->SetScalarParameterValue(TEXT("Roughness"), Roughness);
		}
	}
}
