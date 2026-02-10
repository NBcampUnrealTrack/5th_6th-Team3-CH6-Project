// T3MidBossNotifyModifier.h

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "T3MidBossNotifyModifier.generated.h"

// ============================================================
// 범위 구조체 (배속/거리 등 Min~Max 범위)
// ============================================================

USTRUCT(BlueprintType)
struct FModifierFloatRange
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Min = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Max = 1.f;

	// 범위 내 랜덤 값 반환
	float GetRandom() const { return FMath::FRandRange(Min, Max); }
};

// ============================================================
// 개별 보정기 구조체
// ============================================================

// 거리 구간별 확률 배율
USTRUCT(BlueprintType)
struct FDistanceModifierEntry
{
	GENERATED_BODY()

	// 거리 하한 (이상)
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float MinDistance = 0.f;

	// 거리 상한 (이하)
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float MaxDistance = 500.f;

	// 이 거리 구간에서 확률에 곱해지는 배율
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0.0"))
	float Multiplier = 1.0f;
};

// Pity 시스템 (LoL 치명타식 — 미발동 시 확률 누적 증가)
USTRUCT(BlueprintType)
struct FPityModifierConfig
{
	GENERATED_BODY()

	// 활성화 여부
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bEnabled = false;

	// 미발동 1회당 확률 증가량 (가산)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0.0", ClampMax = "1.0", EditCondition = "bEnabled"))
	float IncrementPerMiss = 0.1f;

	// 최대 누적 보정치
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0.0", ClampMax = "1.0", EditCondition = "bEnabled"))
	float MaxAccumulation = 0.5f;
};

// 패턴 사용 횟수 기반 확률 배율
USTRUCT(BlueprintType)
struct FUsageModifierEntry
{
	GENERATED_BODY()

	// 이 횟수 이상 사용 시 적용
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0"))
	int32 MinUsageCount = 0;

	// 확률 배율
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0.0"))
	float Multiplier = 1.0f;
};

// HP% 구간별 확률 배율
USTRUCT(BlueprintType)
struct FHPModifierEntry
{
	GENERATED_BODY()

	// HP% 하한 (0~1, 이상)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float MinHPPercent = 0.f;

	// HP% 상한 (0~1, 이하)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float MaxHPPercent = 1.0f;

	// 이 HP 구간에서 확률에 곱해지는 배율
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0.0"))
	float Multiplier = 1.0f;
};

// ============================================================
// 노티파이 타입별 보정기 묶음
// ============================================================

USTRUCT(BlueprintType)
struct FNotifyTypeModifiers
{
	GENERATED_BODY()

	// 거리 기반 배율 (배열 순서대로 검색, 첫 매칭 구간 적용)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Distance")
	TArray<FDistanceModifierEntry> DistanceModifiers;

	// Pity 시스템 (연속 미발동 → 확률 증가)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pity")
	FPityModifierConfig PityConfig;

	// 패턴 사용 횟수 기반 (가장 높은 매칭 구간 적용)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Usage")
	TArray<FUsageModifierEntry> UsageModifiers;

	// HP% 기반 배율 (배열 순서대로 검색, 첫 매칭 구간 적용)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HP")
	TArray<FHPModifierEntry> HPModifiers;
};

// ============================================================
// 패턴별 보정기 프로필 (배속 범위 + 노티파이별 보정기)
// ============================================================

USTRUCT(BlueprintType)
struct FPatternModifierProfile
{
	GENERATED_BODY()

	// Slow 노티파이 발동 시 배속 범위 (기본 0.05 ~ 0.3)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PlayRate")
	FModifierFloatRange SlowRate = {0.05f, 0.3f};

	// Fast 노티파이 발동 시 배속 범위 (기본 1.5 ~ 3.0)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PlayRate")
	FModifierFloatRange FastRate = {1.5f, 3.0f};

	// Step 이동 거리 범위 (기본 100 ~ 300)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Step")
	FModifierFloatRange StepDistance = {100.f, 300.f};

	// Step 이동 시간 범위 (기본 0.08 ~ 0.15)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Step")
	FModifierFloatRange StepDuration = {0.08f, 0.15f};

	// 노티파이 타입별 보정기 (Key: "Slow", "Fast", "Step")
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modifiers")
	TMap<FName, FNotifyTypeModifiers> NotifyModifiers;
};

// ============================================================
// DataAsset — 에디터에서 보정기 데이터 관리
// ============================================================

UCLASS(BlueprintType)
class DESECRATION_API UMidBossModifierDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	// 패턴별 보정기 프로필 (Key: PatternName, 예: "Melee_1")
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modifier")
	TMap<FName, FPatternModifierProfile> PatternProfiles;

	// 기본 프로필 (패턴별 설정이 없을 때 폴백으로 사용)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modifier")
	FPatternModifierProfile DefaultProfile;
};

// ============================================================
// 런타임 컨텍스트 (보정기 계산에 필요한 상황 정보)
// ============================================================

USTRUCT(BlueprintType)
struct FNotifyModifierContext
{
	GENERATED_BODY()

	// 타겟까지 거리
	UPROPERTY(BlueprintReadWrite)
	float DistanceToTarget = 0.f;

	// 현재 HP 비율 (0~1)
	UPROPERTY(BlueprintReadWrite)
	float HPPercent = 1.0f;

	// 현재 실행 중인 패턴 이름
	UPROPERTY(BlueprintReadWrite)
	FName PatternName = NAME_None;

	// 현재 처리 중인 노티파이 이름
	UPROPERTY(BlueprintReadWrite)
	FName NotifyName = NAME_None;
};

// ============================================================
// UMidBossNotifyModifier — 런타임 보정기 (상태 관리 + 계산)
// ============================================================

UCLASS(BlueprintType)
class DESECRATION_API UMidBossNotifyModifier : public UObject
{
	GENERATED_BODY()

public:
	// DataAsset 설정 및 초기화
	UFUNCTION(BlueprintCallable, Category = "MidBoss|Modifier")
	void Initialize(UMidBossModifierDataAsset* InDataAsset);

	// 최종 확률 계산 (BaseChance에 모든 보정기 적용)
	UFUNCTION(BlueprintCallable, Category = "MidBoss|Modifier")
	float CalculateFinalChance(float BaseChance, const FNotifyModifierContext& Context) const;

	// Pity 결과 기록 (발동 여부에 따라 카운터 조정)
	UFUNCTION(BlueprintCallable, Category = "MidBoss|Modifier")
	void RecordNotifyResult(FName NotifyName, bool bTriggered);

	// 패턴 사용 횟수 기록
	UFUNCTION(BlueprintCallable, Category = "MidBoss|Modifier")
	void RecordPatternUsage(FName PatternName);

	// 배속/거리 값 가져오기 (범위에서 랜덤)
	UFUNCTION(BlueprintCallable, Category = "MidBoss|Modifier")
	float GetSlowRate(FName PatternName) const;

	UFUNCTION(BlueprintCallable, Category = "MidBoss|Modifier")
	float GetFastRate(FName PatternName) const;

	UFUNCTION(BlueprintCallable, Category = "MidBoss|Modifier")
	float GetStepDistance(FName PatternName) const;

	UFUNCTION(BlueprintCallable, Category = "MidBoss|Modifier")
	float GetStepDuration(FName PatternName) const;

	// 전체 런타임 상태 초기화 (Pity 카운터, 사용 횟수 등)
	UFUNCTION(BlueprintCallable, Category = "MidBoss|Modifier")
	void ResetAllState();

private:
	UPROPERTY()
	TObjectPtr<UMidBossModifierDataAsset> ModifierDataAsset;

	// Pity 카운터 (Key: NotifyName, Value: 연속 미발동 횟수)
	TMap<FName, int32> PityCounters;

	// 패턴 사용 횟수 (Key: PatternName, Value: 누적 사용 횟수)
	TMap<FName, int32> PatternUsageCounters;

	// 프로필 조회 (패턴별 → 없으면 Default)
	const FPatternModifierProfile* GetProfile(FName PatternName) const;

	// 개별 보정기 적용 함수
	float ApplyDistanceModifier(const TArray<FDistanceModifierEntry>& Entries, float Distance, float CurrentChance) const;
	float ApplyPityModifier(const FPityModifierConfig& Config, FName NotifyName, float CurrentChance) const;
	float ApplyUsageModifier(const TArray<FUsageModifierEntry>& Entries, FName PatternName, float CurrentChance) const;
	float ApplyHPModifier(const TArray<FHPModifierEntry>& Entries, float HPPercent, float CurrentChance) const;
};
