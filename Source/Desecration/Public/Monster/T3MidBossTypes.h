// T3MidBossTypes.h
// 중간보스 공용 타입 정의 (구조체, 열거형, 델리게이트)

#pragma once

#include "CoreMinimal.h"
#include "Player/T3DamageTypes.h"
#include "T3MidBossTypes.generated.h"

class UAnimMontage;

// ============================================================
// Enum: 패턴 분류 (고정 — 거리 기반 선택에 사용)
// ============================================================

UENUM(BlueprintType)
enum class EMidBossPatternCategory : uint8
{
	Melee		UMETA(DisplayName = "Melee"),
	Ranged		UMETA(DisplayName = "Ranged"),
	Skill		UMETA(DisplayName = "Skill"),
	Evasion		UMETA(DisplayName = "Evasion")
};

// ============================================================
// Struct: 공격 패턴 데이터
// ============================================================

// 체인 내 개별 몽타주 데이터
USTRUCT(BlueprintType)
struct FPatternMontageData
{
	GENERATED_BODY()

	// 재생할 몽타주
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<UAnimMontage> Montage = nullptr;

	// 시작 배속
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float PlayRate = 1.0f;

	// 이 구간 데미지
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Damage = 20.f;

	// 경직 강도
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EHitIntensity HitIntensity = EHitIntensity::Light;

	// 데미지 타입 (Base, Unparryable 등)
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSubclassOf<UT3DamageType_Base> DamageTypeClass;
};

// 노티파이별 기본 발동 확률
USTRUCT(BlueprintType)
struct FNotifyChanceConfig
{
	GENERATED_BODY()

	// Slow 노티파이 발동 확률 (엇박자 감속)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float SlowChance = 1.0f;

	// Fast 노티파이 발동 확률 (엇박자 가속)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float FastChance = 1.0f;

	// Step 노티파이 발동 확률 (추적 이동)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float StepChance = 1.0f;
};

// 공격 패턴 전체 데이터
USTRUCT(BlueprintType)
struct FMidBossAttackPattern
{
	GENERATED_BODY()

	// 패턴 이름 (자유 입력 — "Melee_1", "DK_HeavySlash" 등)
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName PatternName = NAME_None;

	// 분류 (Melee / Ranged / Skill / Evasion)
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EMidBossPatternCategory Category = EMidBossPatternCategory::Melee;

	// 체인 몽타주 배열 (순서대로 재생, 1개면 단타, 2개면 2타 체인)
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FPatternMontageData> MontageChain;

	// 해금 스테이지 (1 = 항상 사용 가능)
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 RequiredStage = 1;

	// 쿨다운 초 (0이면 쿨다운 없음)
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Cooldown = 0.f;

	// ActionCount 소모량 (0 = 소모 안 함, 예: Evasion)
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 ActionCountCost = 1;

	// 노티파이별 기본 발동 확률 (BP에서 ModifyNotifyChance로 상황별 보정 가능)
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FNotifyChanceConfig NotifyChances;
};

// ============================================================
// Delegate
// ============================================================

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnMidBossHit);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnMidBossStun);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnMidBossDeath);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnMidBossDamaged);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnMidBossSpawned);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnMidBossActivated);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnMidBossDeathFinished);

// 패턴 완료 델리게이트 — FName으로 어떤 패턴이 끝났는지 전달
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPatternCompleted, FName, CompletedPatternName);

// C++ 전용 Non-dynamic 델리게이트 (StateTree Task 바인딩용)
DECLARE_MULTICAST_DELEGATE(FOnPatternCompletedNative);

// ============================================================
// Struct: 스탯
// ============================================================

USTRUCT(BlueprintType)
struct FMidBossStats
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float MaxHP = 1500.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float CurrentHP = 1500.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float AttackPower = 20.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float StunThreshold = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float CurrentStunGauge = 0.f;
};
