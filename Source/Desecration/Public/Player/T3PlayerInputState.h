// T3PlayerInputState.h

#pragma once

#include "CoreMinimal.h"
#include "T3PlayerInputState.generated.h"

UENUM(BlueprintType)
enum class ERollDirection : uint8
{
	Forward,
	ForwardRight,
	Right,
	BackRight,
	Back,
	BackLeft,
	Left,
	ForwardLeft,
	Neutral
};

UENUM(BlueprintType)
enum class ECharacterClass : uint8
{
	Paladin,
};

UENUM(BlueprintType)
enum class EGaitState : uint8
{
	Walk,
	Run
};

UENUM(BlueprintType)
enum class EMovementState : uint8
{
	Idle,
	Moving
};

USTRUCT(BlueprintType)
struct DESECRATION_API FT3PlayerInputState
{
	GENERATED_BODY()
	
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PlayerInputState")
	bool bWantsToMove = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PlayerInputState")
	bool bWantsToStop = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PlayerInputState")
	bool bWantsToRoll = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PlayerInputState")
	bool bIsMoving = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PlayerInputState")
	bool bIsCombatState = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PlayerInputState")
	bool bIsLockOn = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PlayerInputState")
	bool bIsInAir = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PlayerInputState")
	bool bIsBlocking = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PlayerInputState")
	bool bIsAttacking = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PlayerInputState")
	bool bCanAttack = true;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PlayerInputState")
	bool bIsJustLanded = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PlayerInputState")
	float ComboCount = 0.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PlayerInputState")
	float CurrentSpeed = 0.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PlayerInputState")
	float FutureSpeed = 0.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PlayerInputState")
	float FallingSpeed = 0.0f;
	
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PlayerInputState")
	ERollDirection RollDirection = ERollDirection::Neutral;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PlayerInputState")
	ECharacterClass CharacterClass = ECharacterClass::Paladin;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PlayerInputState")
	EGaitState T3GaitState = EGaitState::Run;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PlayerInputState")
	EMovementState MovementState = EMovementState::Moving;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PlayerInputState")
	float InputYawOffset = 0.0f;
	
};
