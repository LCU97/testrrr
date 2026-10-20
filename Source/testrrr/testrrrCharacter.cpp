/*
 * 1인칭 캐릭터 구현 및 E/G/좌클릭 입력 배선
 * 기존 이동·시점·점프 처리에 상호작용용 Input Action과 Mapping Context를 연결한다.
 * 빙의 해제·종료 시에는 이 캐릭터가 추가한 입력 문맥을 정리한다.
 */

// Copyright Epic Games, Inc. All Rights Reserved.

#include "testrrrCharacter.h"
#include "Animation/AnimInstance.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "EnhancedInputComponent.h"
#include "InputActionValue.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "testrrr.h"
#include "Interaction/CarryComponent.h"
#include "Interaction/InteractionComponent.h"
#include "Interaction/ItemUseComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/PlayerController.h"
#include "InputAction.h"
#include "InputMappingContext.h"

AtestrrrCharacter::AtestrrrCharacter()
{
	// 자식 캐릭터 BP에도 공통 상호작용과 운반 기능이 상속되도록 기본 컴포넌트로 만든다.
	CarryComponent = CreateDefaultSubobject<UCarryComponent>(TEXT("CarryComponent"));
	ItemUseComponent = CreateDefaultSubobject<UItemUseComponent>(TEXT("ItemUseComponent"));
	InteractionComponent = CreateDefaultSubobject<UInteractionComponent>(TEXT("InteractionComponent"));
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(55.f, 96.0f);
	
	// Create the first person mesh that will be viewed only by this character's owner
	FirstPersonMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("First Person Mesh"));

	FirstPersonMesh->SetupAttachment(GetMesh());
	FirstPersonMesh->SetOnlyOwnerSee(true);
	FirstPersonMesh->FirstPersonPrimitiveType = EFirstPersonPrimitiveType::FirstPerson;
	FirstPersonMesh->SetCollisionProfileName(FName("NoCollision"));

	// Create the Camera Component	
	FirstPersonCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("First Person Camera"));
	FirstPersonCameraComponent->SetupAttachment(FirstPersonMesh, FName("head"));
	FirstPersonCameraComponent->SetRelativeLocationAndRotation(FVector(-2.8f, 5.89f, 0.0f), FRotator(0.0f, 90.0f, -90.0f));
	FirstPersonCameraComponent->bUsePawnControlRotation = true;
	FirstPersonCameraComponent->bEnableFirstPersonFieldOfView = true;
	FirstPersonCameraComponent->bEnableFirstPersonScale = true;
	FirstPersonCameraComponent->FirstPersonFieldOfView = 70.0f;
	FirstPersonCameraComponent->FirstPersonScale = 0.6f;

	// configure the character comps
	GetMesh()->SetOwnerNoSee(true);
	GetMesh()->FirstPersonPrimitiveType = EFirstPersonPrimitiveType::WorldSpaceRepresentation;

	GetCapsuleComponent()->SetCapsuleSize(34.0f, 96.0f);

	// Configure character movement
	GetCharacterMovement()->BrakingDecelerationFalling = 1500.0f;
	GetCharacterMovement()->AirControl = 0.5f;
	// 기본 750000의 밀기 힘은 가벼운 공을 접촉만으로 날려 보낸다.
	// 질량에 비례해 힘을 키우지 않아 무거운 볼링공이 더 버티도록 한다.
	GetCharacterMovement()->InitialPushForceFactor = 100.f;
	GetCharacterMovement()->PushForceFactor = 3000.f;
	GetCharacterMovement()->TouchForceFactor = 0.2f;
	GetCharacterMovement()->RepulsionForce = 0.2f;
	GetCharacterMovement()->bPushForceScaledToMass = false;
	GetCharacterMovement()->bTouchForceScaledToMass = false;
}

void AtestrrrCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{	
	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		// 입력 재설정 시 이전에 추가한 상호작용 문맥을 먼저 제거한다.
		RemoveInteractionMapping();
		// 별도 입력 에셋 없이도 동작하도록 미지정된 액션만 기본 생성한다.
		if (!InteractAction)
		{
			InteractAction = NewObject<UInputAction>(this, TEXT("IA_Interact"));
			InteractAction->ValueType = EInputActionValueType::Boolean;
		}
		if (!DropAction)
		{
			DropAction = NewObject<UInputAction>(this, TEXT("IA_Drop"));
			DropAction->ValueType = EInputActionValueType::Boolean;
		}
		if (!UseItemAction)
		{
			UseItemAction = NewObject<UInputAction>(this, TEXT("IA_UseItem"));
			UseItemAction->ValueType = EInputActionValueType::Boolean;
		}
		// 사용 동작은 액터가 선택하며, 입력 계층은 사용 요청과 키 배정만 관리한다.
		if (!RotateHoldAction)
		{
			RotateHoldAction = NewObject<UInputAction>(this, TEXT("IA_RotateHeldItem"));
			RotateHoldAction->ValueType = EInputActionValueType::Boolean;
		}
		InteractionMappingContext = NewObject<UInputMappingContext>(this);
		InteractionMappingContext->MapKey(InteractAction, InteractKey);
		InteractionMappingContext->MapKey(DropAction, DropKey);
		InteractionMappingContext->MapKey(UseItemAction, UseItemKey);
		InteractionMappingContext->MapKey(RotateHoldAction, RotateHoldKey);
		// 실제 키 배정과 화면에 표시하는 키 이름이 일치하도록 맞춘다.
		InteractionComponent->InteractInputLabel = InteractKey.GetDisplayName();
		InteractionComponent->DropInputLabel = DropKey.GetDisplayName();
		InteractionComponent->UseInputLabel = UseItemKey.GetDisplayName();
		InteractionComponent->RotateInputLabel = RotateHoldKey.GetDisplayName();
		if (APlayerController* PC = Cast<APlayerController>(GetController()))
		{
			if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
			{
				// 로컬 플레이어의 Enhanced Input에만 상호작용 문맥을 등록한다.
				Subsystem->AddMappingContext(InteractionMappingContext, 1);
				InteractionInputSubsystem = Subsystem;
			}
		}
		// Started를 사용하여 키를 누른 한 번에 한 번만 요청한다. 누르는 동안 반복하지 않는다.
		EnhancedInputComponent->BindAction(InteractAction, ETriggerEvent::Started, this, &AtestrrrCharacter::DoInteract);
		EnhancedInputComponent->BindAction(DropAction, ETriggerEvent::Started, this, &AtestrrrCharacter::DoDrop);
		EnhancedInputComponent->BindAction(RotateHoldAction, ETriggerEvent::Started, this, &AtestrrrCharacter::DoBeginRotateItem);
		EnhancedInputComponent->BindAction(RotateHoldAction, ETriggerEvent::Triggered, this, &AtestrrrCharacter::DoBeginRotateItem);
		EnhancedInputComponent->BindAction(RotateHoldAction, ETriggerEvent::Completed, this, &AtestrrrCharacter::DoEndRotateItem);
		EnhancedInputComponent->BindAction(RotateHoldAction, ETriggerEvent::Canceled, this, &AtestrrrCharacter::DoEndRotateItem);
		// 단발·지속 동작이 같은 액션을 사용한다. 지속 갱신은 사용 세션의 Tick이 담당한다.
		EnhancedInputComponent->BindAction(UseItemAction, ETriggerEvent::Started, this, &AtestrrrCharacter::DoBeginUseItem);
		EnhancedInputComponent->BindAction(UseItemAction, ETriggerEvent::Completed, this, &AtestrrrCharacter::DoEndUseItem);
		EnhancedInputComponent->BindAction(UseItemAction, ETriggerEvent::Canceled, this, &AtestrrrCharacter::DoCancelUseItem);

		// Jumping
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &AtestrrrCharacter::DoJumpStart);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &AtestrrrCharacter::DoJumpEnd);

		// Moving
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AtestrrrCharacter::MoveInput);

		// Looking/Aiming
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AtestrrrCharacter::LookInput);
		EnhancedInputComponent->BindAction(MouseLookAction, ETriggerEvent::Triggered, this, &AtestrrrCharacter::LookInput);
	}
	else
	{
		UE_LOG(Logtestrrr, Error, TEXT("'%s' Failed to find an Enhanced Input Component! This template is built to use the Enhanced Input system. If you intend to use the legacy system, then you will need to update this C++ file."), *GetNameSafe(this));
	}
}

void AtestrrrCharacter::DoInteract()
{
	// 대상 탐색, 운반 중 차단, 실행 조건 판단은 상호작용 컴포넌트에 맡긴다.
	InteractionComponent->TryInteract();
}

void AtestrrrCharacter::DoDrop()
{
	// 일반 내려놓기 요청이므로 물건별 CanDrop 조건을 적용한다.
	CarryComponent->TryDrop();
}

void AtestrrrCharacter::DoBeginRotateItem()
{
	CarryComponent->BeginRotateHeldObject();
}

void AtestrrrCharacter::DoEndRotateItem()
{
	CarryComponent->EndRotateHeldObject();
}

void AtestrrrCharacter::RemoveInteractionMapping()
{
	// 입력 문맥을 바꿀 때 키 종료 이벤트가 오지 않더라도 지속 효과는 반드시 멈춘다.
	ItemUseComponent->CancelUseInput();
	CarryComponent->EndRotateHeldObject();
	// 전체 입력을 지우지 않고 자신이 추가한 문맥만 제거한다.
	if (InteractionInputSubsystem.IsValid() && InteractionMappingContext)
	{
		InteractionInputSubsystem->RemoveMappingContext(InteractionMappingContext);
	}
	InteractionInputSubsystem.Reset();
}

void AtestrrrCharacter::UnPossessed()
{
	// 이전 Pawn의 입력·지속 효과를 정리하고 물건별 일반 내려놓기 제한과 무관하게 해제한다.
	RemoveInteractionMapping();
	CarryComponent->ForceRelease();
	Super::UnPossessed();
}

void AtestrrrCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	RemoveInteractionMapping();
	Super::EndPlay(EndPlayReason);
}

void AtestrrrCharacter::DoBeginUseItem()
{
	ItemUseComponent->BeginUseInput();
}

void AtestrrrCharacter::DoEndUseItem()
{
	ItemUseComponent->EndUseInput();
}

void AtestrrrCharacter::DoCancelUseItem()
{
	ItemUseComponent->CancelUseInput();
}


void AtestrrrCharacter::MoveInput(const FInputActionValue& Value)
{
	// get the Vector2D move axis
	FVector2D MovementVector = Value.Get<FVector2D>();

	// pass the axis values to the move input
	DoMove(MovementVector.X, MovementVector.Y);

}

void AtestrrrCharacter::LookInput(const FInputActionValue& Value)
{
	// get the Vector2D look axis
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	// pass the axis values to the aim input
	DoAim(LookAxisVector.X, LookAxisVector.Y);

}

void AtestrrrCharacter::DoAim(float Yaw, float Pitch)
{
	// 마우스 입력을 물건에만 전달한다. Controller 회전을 쌓지 않으므로 해제 시 튀지 않는다.
	if (CarryComponent->IsRotatingHeldObject())
	{
		CarryComponent->RotateHeldObject(FVector2D(Yaw, Pitch));
		return;
	}
	if (GetController())
	{
		// pass the rotation inputs
		AddControllerYawInput(Yaw);
		AddControllerPitchInput(Pitch);
	}
}

void AtestrrrCharacter::DoMove(float Right, float Forward)
{
	if (GetController())
	{
		// pass the move inputs
		AddMovementInput(GetActorRightVector(), Right);
		AddMovementInput(GetActorForwardVector(), Forward);
	}
}

void AtestrrrCharacter::DoJumpStart()
{
	// pass Jump to the character
	Jump();
}

void AtestrrrCharacter::DoJumpEnd()
{
	// pass StopJumping to the character
	StopJumping();
}
