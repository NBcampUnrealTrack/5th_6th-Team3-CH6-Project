#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "T3Monster.generated.h"

UENUM(BlueprintType)
enum class ET3MonsterType : uint8
{
	None,
	Normal,
	MiddleBoss,
	Boss
};

UINTERFACE(MinimalAPI)
class UT3Monster : public UInterface
{
	GENERATED_BODY()
};

class DESECRATION_API IT3Monster
{
	GENERATED_BODY()

public:
	virtual ET3MonsterType GetMonsterType() const = 0;
	
	virtual float GetHPPercent() const = 0;
	
	virtual void ApplyBonusDamage(float BonusDamage) = 0;
	
	virtual void SetAnimationSpeedMultiplier(float MoveAnimMultiplier, float AttackAnimMultiplier) = 0;
};