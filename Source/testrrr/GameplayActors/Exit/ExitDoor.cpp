#include "GameplayActors/Exit/ExitDoor.h"

#include "GameplayActors/Exit/EscapeSuccessWidget.h"
#include "Blueprint/UserWidget.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

AExitDoor::AExitDoor()
{
	PrimaryActorTick.bCanEverTick = true;
	DoorMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DoorMesh"));
	SetRootComponent(DoorMesh);
	DoorMesh->SetCollisionProfileName(TEXT("BlockAll"));
	DoorMesh->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	DoorMesh->SetRelativeScale3D(FVector(0.18f, 4.f, 3.f));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (Cube.Succeeded())
	{
		DoorMesh->SetStaticMesh(Cube.Object);
	}

	EscapeTrigger = CreateDefaultSubobject<UBoxComponent>(TEXT("EscapeTrigger"));
	EscapeTrigger->SetupAttachment(DoorMesh);
	EscapeTrigger->SetRelativeLocation(FVector(100.f, 0.f, 0.f));
	EscapeTrigger->SetBoxExtent(FVector(100.f, 180.f, 150.f));
	EscapeTrigger->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	EscapeTrigger->SetCollisionResponseToAllChannels(ECR_Ignore);
	EscapeTrigger->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	EscapeTrigger->OnComponentBeginOverlap.AddDynamic(this, &AExitDoor::OnEscapeTriggerBeginOverlap);
	SuccessWidgetClass = UEscapeSuccessWidget::StaticClass();
}

void AExitDoor::BeginPlay()
{
	Super::BeginPlay();
	ClosedLocation = GetActorLocation();
}

void AExitDoor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	const FVector TargetLocation = bIsOpen ? ClosedLocation + OpenOffset : ClosedLocation;
	const float Speed = FMath::Max(1.f, OpenOffset.Size() / FMath::Max(TravelDuration, KINDA_SMALL_NUMBER));
	SetActorLocation(FMath::VInterpConstantTo(GetActorLocation(), TargetLocation, DeltaSeconds, Speed));
}

void AExitDoor::ActivateExit()
{
	const bool bWasOpen = bIsOpen;
	bIsOpen = true;
	bEscapeReported = false;
	GetWorldTimerManager().ClearTimer(CloseTimer);
	GetWorldTimerManager().SetTimer(CloseTimer, this, &AExitDoor::CloseExit, FMath::Max(0.1f, OpenDuration), false);
	if (!bWasOpen)
	{
		OnExitDoorStateChanged.Broadcast(true);
	}
}

void AExitDoor::CloseExit()
{
	if (!bIsOpen)
	{
		return;
	}
	bIsOpen = false;
	OnExitDoorStateChanged.Broadcast(false);
}

void AExitDoor::OnEscapeTriggerBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!bIsOpen || bEscapeReported || !Cast<APawn>(OtherActor))
	{
		return;
	}
	bEscapeReported = true;
	OnEscapeSucceeded.Broadcast();
	ShowSuccess();
}

void AExitDoor::ShowSuccess()
{
	APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0);
	if (!PlayerController || !SuccessWidgetClass)
	{
		return;
	}
	if (UEscapeSuccessWidget* Widget = CreateWidget<UEscapeSuccessWidget>(PlayerController, SuccessWidgetClass))
	{
		Widget->AddToViewport(50);
	}
}
