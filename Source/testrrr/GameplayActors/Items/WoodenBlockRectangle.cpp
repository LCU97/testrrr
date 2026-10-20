#include "GameplayActors/Items/WoodenBlockRectangle.h"

AWoodenBlockRectangle::AWoodenBlockRectangle()
{
	// PickupActor에서 제공하는 기본 Cube를 그대로 사용한다.
	ItemMass = 20.f;
	SetDefaultItemDimensions(FVector(150.f, 150.f, 75.f));
}
