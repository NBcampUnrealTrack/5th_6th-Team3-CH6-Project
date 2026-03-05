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
	Evasion		UMETA(DisplayName = "Evasion"),
	GapCloser	UMETA(DisplayName = "GapCloser")
};

// ============================================================
// Struct: 공격 패턴 데이터
// ============================================================

// 체인 내 개별 몽타주 데이터 (한 섹션 = 한 공격)
USTRUCT(BlueprintType)
struct FPatternMontageData
{
	GENERATED_BODY()

	// 재생할 몽타주
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<UAnimMontage> Montage = nullptr;

	// 섹션 콤보 모드에서 재생할 섹션 이름 (bUseSectionCombo=true일 때만 사용)
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName SectionName = NAME_None;

	// 시작 배속
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float PlayRate = 1.0f;

	// 데미지
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Damage = 20.f;

	// 경직 강도
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EHitIntensity HitIntensity = EHitIntensity::Light;

	// 데미지 타입
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSubclassOf<UT3DamageType_Base> DamageTypeClass;

	// 모션 워프 최대 거리 오버라이드 (0 이하 = 보스 기본값 사용)
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float MaxWarpDistanceOverride = 0.f;
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
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (TitleProperty = "[{SectionName}] Dmg:{Damage} Rate:{PlayRate} Warp:{MaxWarpDistanceOverride}"))
	TArray<FPatternMontageData> MontageChain;

	// 섹션 콤보 모드 — MontageChain[0]의 몽타주를 섹션으로 진행
	// true: 같은 몽타주 내 섹션 자동 연결 (idle 복귀 없음)
	// 마지막 엔트리에 다른 몽타주 지정 시 체인으로 연결 (자연스러운 idle 복귀용)
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bUseSectionCombo = false;

	// 해금 스테이지 (1 = 항상 사용 가능)
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 RequiredStage = 1;

	// 쿨다운 초 (0이면 쿨다운 없음)
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Cooldown = 0.f;

	// 공용 쿨다운 그룹 (같은 그룹 패턴은 쿨다운 공유 — 예: "DK_Combo2" 그룹이면 Short/Mid/Full 동시 쿨다운)
	// NAME_None이면 개별 쿨다운만 사용
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName CooldownGroup = NAME_None;

	// 그룹 쿨다운 초 (이 패턴 사용 시 그룹 전체에 걸리는 쿨다운)
	// Cooldown = 개별, GroupCooldown = 그룹 — 둘 다 적용됨
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = "CooldownGroup != NAME_None"))
	float GroupCooldown = 0.f;

	// ActionCount 소모량 (0 = 소모 안 함, 예: Evasion)
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 ActionCountCost = 1;

	// 패링 윈도우 사용 여부 (ParryWindowStart 노티파이가 이 패턴에서만 동작)
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bHasParryWindow = false;

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
