// T3DamageTypes.h

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/DamageType.h"
#include "T3DamageTypes.generated.h"

//  1. 기본 데미지 (막기/패링 가능)
UCLASS()
class DESECRATION_API UT3DamageType_Base : public UDamageType
{
	GENERATED_BODY()
};

// 2. 패링 불가 (막기만 가능)
UCLASS()
class DESECRATION_API UT3DamageType_Unparryable : public UT3DamageType_Base
{
	GENERATED_BODY()
};

//  3. 가드 불가 (막기/패링 모두 무시)
UCLASS()
class DESECRATION_API UT3DamageType_Unblockable : public UT3DamageType_Base
{
	GENERATED_BODY()
};

// 4. 즉사 공격
UCLASS()
class DESECRATION_API UT3DamageType_InstantDeath : public UT3DamageType_Base
{
	GENERATED_BODY()
};