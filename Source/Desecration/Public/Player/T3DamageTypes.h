// T3DamageTypes.h

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/DamageType.h"
#include "Engine/DamageEvents.h"
#include "T3DamageTypes.generated.h"

UENUM(BlueprintType)
enum class EHitIntensity : uint8
{
	Light    UMETA(DisplayName = "Flinch (움찔)"),
	Medium   UMETA(DisplayName = "Stagger (경직)"),
	Heavy    UMETA(DisplayName = "Knockback (날아감)")
};

// 커스텀 데미지 이벤트 구조체
USTRUCT()
struct FT3DamageEvent : public FDamageEvent
{
	GENERATED_BODY()

	UPROPERTY()
	EHitIntensity HitIntensity = EHitIntensity::Light;

	// 기본 생성자 및 클래스 전달 생성자
	FT3DamageEvent() : FDamageEvent() {}
	FT3DamageEvent(TSubclassOf<UDamageType> InDamageTypeClass) : FDamageEvent(InDamageTypeClass) {}

	// 기본 FDamageEvent와 구분하기 위한 ID (임의의 고유값)
	static const int32 ClassID = 777;
	virtual int32 GetTypeID() const override { return ClassID; }
};

//  1. 기본 데미지 (막기/패링 가능)
UCLASS()
class DESECRATION_API UT3DamageType_Base : public UDamageType
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat")
	EHitIntensity HitIntensity = EHitIntensity::Light;
};

// 2. 패링 불가 (막기/ 패링 불가)
UCLASS()
class DESECRATION_API UT3DamageType_Unparryable : public UT3DamageType_Base
{
	GENERATED_BODY()

};

//  3. 가드 불가 (패링 가능)
UCLASS()
class DESECRATION_API UT3DamageType_Unblockable : public UT3DamageType_Base
{
	GENERATED_BODY()

};

// 4. 회피 불가 (회피/막기/패링 불가)
UCLASS()
class DESECRATION_API UT3DamageType_Undodgable : public UT3DamageType_Base
{
	GENERATED_BODY()


};