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

USTRUCT(BlueprintType)
struct DESECRATION_API FT3PlayerInputState
{
	GENERATED_BODY()
	
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PlayerInputState")
	bool bWantsToMove = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PlayerInputState")
	bool bWantsToRoll = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PlayerInputState")
	bool bIsMoving = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PlayerInputState")
	bool bIsCombatState = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PlayerInputState")
	bool bIsLockOn = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PlayerInputState")
	bool bIsFalling = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PlayerInputState")
	bool bIsBlocking = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PlayerInputState")
	float CurrentSpeed = 0.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PlayerInputState")
	float FallingSpeed = 0.0f;
	
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PlayerInputState")
	ERollDirection RollDirection = ERollDirection::Neutral;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PlayerInputState")
	float InputYawOffset = 0.0f;
	
};
