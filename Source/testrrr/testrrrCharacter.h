/*
 * 1인칭 캐릭터와 상호작용 입력 연결
 * 기존 이동·점프 구조에 Enhanced Input 기반 E/G/아이템 사용 입력과 공통 컴포넌트를 추가한다.
 * 입력은 요청만 전달하고 대상별 동작과 운반 상태 관리는 각 컴포넌트에 맡긴다.
 */

// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Logging/LogMacros.h"
#include "InputCoreTypes.h"
#include "testrrrCharacter.generated.h"

class UInputComponent;
class USkeletalMeshComponent;
class UCameraComponent;
class UInputAction;
struct FInputActionValue;
class UCarryComponent;
class UInteractionComponent;
class UItemUseComponent;
class UInputMappingContext;
class UEnhancedInputLocalPlayerSubsystem;

DECLARE_LOG_CATEGORY_EXTERN(LogTemplateCharacter, Log, All);

/**
 *  A basic first person character
 */
UCLASS(abstract)
class TESTRRR_API AtestrrrCharacter : public ACharacter
{
	GENERATED_BODY()

	/** Pawn mesh: first person view (arms; seen only by self) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	USkeletalMeshComponent* FirstPersonMesh;

	/** First person camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	UCameraComponent* FirstPersonCameraComponent;

protected:
	// 운반 상태 관리와 대상 탐색을 별도 컴포넌트로 분리한다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Interaction")
	TObjectPtr<UCarryComponent> CarryComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Interaction")
	TObjectPtr<UInteractionComponent> InteractionComponent;

	// 사용 시작·유지·종료와 아이템 교체 시 취소는 사용 컴포넌트가 담당한다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Interaction")
	TObjectPtr<UItemUseComponent> ItemUseComponent;

	// 에셋을 지정하지 않으면 C++에서 Boolean Input Action을 생성한다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Input|Interaction")
	TObjectPtr<UInputAction> InteractAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Input|Interaction")
	TObjectPtr<UInputAction> DropAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Input|Interaction")
	TObjectPtr<UInputAction> UseItemAction;

	UPROPERTY(EditDefaultsOnly, Category="Input|Interaction")
	TObjectPtr<UInputAction> RotateItemAction;

	UPROPERTY(EditDefaultsOnly, Category="Input|Interaction")
	FKey RotateItemKey = EKeys::R;

	// BP 기본값에서 키를 변경하면 안내 문구의 키 이름도 함께 갱신된다.
	UPROPERTY(EditDefaultsOnly, Category="Input|Interaction")
	FKey InteractKey = EKeys::E;

	UPROPERTY(EditDefaultsOnly, Category="Input|Interaction")
	FKey DropKey = EKeys::G;

	UPROPERTY(EditDefaultsOnly, Category="Input|Interaction")
	FKey UseItemKey = EKeys::LeftMouseButton;

	/** Jump Input Action */
	UPROPERTY(EditAnywhere, Category ="Input")
	UInputAction* JumpAction;

	/** Move Input Action */
	UPROPERTY(EditAnywhere, Category ="Input")
	UInputAction* MoveAction;

	/** Look Input Action */
	UPROPERTY(EditAnywhere, Category ="Input")
	class UInputAction* LookAction;

	/** Mouse Look Input Action */
	UPROPERTY(EditAnywhere, Category ="Input")
	class UInputAction* MouseLookAction;
	
public:
	AtestrrrCharacter();

protected:

	/** Called from Input Actions for movement input */
	void MoveInput(const FInputActionValue& Value);

	/** Called from Input Actions for looking input */
	void LookInput(const FInputActionValue& Value);

	/** Handles aim inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoAim(float Yaw, float Pitch);

	/** Handles move inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoMove(float Right, float Forward);

	/** Handles jump start inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoJumpStart();

	/** Handles jump end inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoJumpEnd();

protected:

	/** Set up input action bindings */
	virtual void SetupPlayerInputComponent(UInputComponent* InputComponent) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void UnPossessed() override;
	// 입력 함수는 요청 전달만 담당하며, 개별 물건과 스위치의 종류를 구별하지 않는다.
	void DoInteract();
	void DoDrop();
	void DoRotateItem();
	void DoBeginUseItem();
	void DoEndUseItem();
	void DoCancelUseItem();
	void RemoveInteractionMapping();

	// 이 캐릭터가 만든 문맥만 보관하여 빙의 해제 시 기존 이동 문맥을 건드리지 않는다.
	UPROPERTY(Transient)
	TObjectPtr<UInputMappingContext> InteractionMappingContext;

	UPROPERTY(Transient)
	TWeakObjectPtr<UEnhancedInputLocalPlayerSubsystem> InteractionInputSubsystem;
	

public:

	/** Returns the first person mesh **/
	USkeletalMeshComponent* GetFirstPersonMesh() const { return FirstPersonMesh; }

	/** Returns first person camera component **/
	UCameraComponent* GetFirstPersonCameraComponent() const { return FirstPersonCameraComponent; }

};

