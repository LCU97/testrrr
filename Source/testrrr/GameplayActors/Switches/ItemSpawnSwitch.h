#pragma once

#include "CoreMinimal.h"
#include "Interaction/ButtonSwitchActor.h"
#include "ItemSpawnSwitch.generated.h"

class UArrowComponent;

UCLASS(meta=(DisplayName="Item Spawn Switch"))
class TESTRRR_API AItemSpawnSwitch : public AButtonSwitchActor
{
	GENERATED_BODY()
public:
	AItemSpawnSwitch();
	virtual bool Activate_Implementation(const FInteractionContext& Context) override;
	virtual bool CanInteract_Implementation(const FInteractionContext& Context) const override;

	UPROPERTY(EditAnywhere, Category="Spawning")
	TSubclassOf<AActor> SpawnClass;

	// 충돌 위치에 겹쳐 있더라도 생성할지 에디터에서 선택한다.
	UPROPERTY(EditAnywhere, Category="Spawning")
	ESpawnActorCollisionHandlingMethod SpawnCollisionHandling = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	// 에디터에서 이 화살표를 이동/회전해 생성 위치와 방향을 지정한다.
	UPROPERTY(VisibleAnywhere, Category="Spawning")
	TObjectPtr<UArrowComponent> SpawnPoint;

	UPROPERTY(VisibleInstanceOnly, Category="Spawning")
	TWeakObjectPtr<AActor> LastSpawnedActor;
};
