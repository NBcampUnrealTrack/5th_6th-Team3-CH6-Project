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
	
	// 외부 슬로우/가속 적용 진입점 (천사 장신구 등).
	// MoveSpeedMultiplier  - CharacterMovement::MaxWalkSpeed에 곱
	// MoveAnimMultiplier   - 이동 계열 몽타주 PlayRate에 곱 (구현체 별 처리)
	// AttackAnimMultiplier - 공격 계열 몽타주 PlayRate에 곱 (AnimInstance/직접 곱셈)
	virtual void SetAnimationSpeedMultiplier(
		float MoveSpeedMultiplier,
		float MoveAnimMultiplier,
		float AttackAnimMultiplier) = 0;
};