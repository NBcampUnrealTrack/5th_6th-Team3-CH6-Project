#include "Monster/T3MidBossNotifyModifier.h"
#include "Desecration.h"

// ============================================================
// 초기화
// ============================================================

void UMidBossNotifyModifier::Initialize(UMidBossModifierDataAsset* InDataAsset)
{
	ModifierDataAsset = InDataAsset;
	ResetAllState();

	UE_LOG(LogDesecration, Log, TEXT("T3_MidBoss: NotifyModifier 초기화 (DataAsset: %s)"),
		InDataAsset ? *InDataAsset->GetName() : TEXT("nullptr"));
}

// ============================================================
// 최종 확률 계산
// ============================================================

float UMidBossNotifyModifier::CalculateFinalChance(float BaseChance, const FNotifyModifierContext& Context) const
{
	if (!ModifierDataAsset)
	{
		return BaseChance;
	}

	const FPatternModifierProfile* Profile = GetProfile(Context.PatternName);
	if (!Profile)
	{
		return BaseChance;
	}

	const FNotifyTypeModifiers* TypeMod = Profile->NotifyModifiers.Find(Context.NotifyName);
	if (!TypeMod)
	{
		return BaseChance;
	}

	float Result = BaseChance;

	// 1. 거리 보정 (배율)
	Result = ApplyDistanceModifier(TypeMod->DistanceModifiers, Context.DistanceToTarget, Result);

	// 2. Pity 보정 (가산)
	Result = ApplyPityModifier(TypeMod->PityConfig, Context.NotifyName, Result);

	// 3. 사용 횟수 보정 (배율)
	Result = ApplyUsageModifier(TypeMod->UsageModifiers, Context.PatternName, Result);

	// 4. HP 보정 (배율)
	Result = ApplyHPModifier(TypeMod->HPModifiers, Context.HPPercent, Result);

	return FMath::Clamp(Result, 0.f, 1.f);
}

// ============================================================
// 상태 기록
// ============================================================

void UMidBossNotifyModifier::RecordNotifyResult(FName NotifyName, bool bTriggered)
{
	if (bTriggered)
	{
		// 발동 시 카운터 리셋
		PityCounters.FindOrAdd(NotifyName) = 0;
	}
	else
	{
		// 미발동 시 카운터 증가
		PityCounters.FindOrAdd(NotifyName)++;
	}
}

void UMidBossNotifyModifier::RecordPatternUsage(FName PatternName)
{
	PatternUsageCounters.FindOrAdd(PatternName)++;
}

// ============================================================
// 배속/거리 값 조회 (범위에서 랜덤)
// ============================================================

float UMidBossNotifyModifier::GetSlowRate(FName PatternName) const
{
	const FPatternModifierProfile* Profile = GetProfile(PatternName);
	if (!Profile)
	{
		return 0.1f;
	}
	return Profile->SlowRate.GetRandom();
}

float UMidBossNotifyModifier::GetFastRate(FName PatternName) const
{
	const FPatternModifierProfile* Profile = GetProfile(PatternName);
	if (!Profile)
	{
		return 2.0f;
	}
	return Profile->FastRate.GetRandom();
}

float UMidBossNotifyModifier::GetStepDistance(FName PatternName) const
{
	const FPatternModifierProfile* Profile = GetProfile(PatternName);
	if (!Profile)
	{
		return 200.f;
	}
	return Profile->StepDistance.GetRandom();
}

float UMidBossNotifyModifier::GetStepDuration(FName PatternName) const
{
	const FPatternModifierProfile* Profile = GetProfile(PatternName);
	if (!Profile)
	{
		return 0.1f;
	}
	return Profile->StepDuration.GetRandom();
}

// ============================================================
// 상태 초기화
// ============================================================

void UMidBossNotifyModifier::ResetAllState()
{
	PityCounters.Empty();
	PatternUsageCounters.Empty();
}

// ============================================================
// 내부 함수
// ============================================================

const FPatternModifierProfile* UMidBossNotifyModifier::GetProfile(FName PatternName) const
{
	if (!ModifierDataAsset)
	{
		return nullptr;
	}

	// 패턴별 프로필 우선 검색
	const FPatternModifierProfile* Found = ModifierDataAsset->PatternProfiles.Find(PatternName);
	if (Found)
	{
		return Found;
	}

	// 없으면 기본 프로필 반환
	return &ModifierDataAsset->DefaultProfile;
}

float UMidBossNotifyModifier::ApplyDistanceModifier(
	const TArray<FDistanceModifierEntry>& Entries, float Distance, float CurrentChance) const
{
	for (const FDistanceModifierEntry& Entry : Entries)
	{
		if (Distance >= Entry.MinDistance && Distance <= Entry.MaxDistance)
		{
			return CurrentChance * Entry.Multiplier;
		}
	}
	return CurrentChance;
}

float UMidBossNotifyModifier::ApplyPityModifier(
	const FPityModifierConfig& Config, FName NotifyName, float CurrentChance) const
{
	if (!Config.bEnabled)
	{
		return CurrentChance;
	}

	const int32* MissCount = PityCounters.Find(NotifyName);
	if (!MissCount || *MissCount == 0)
	{
		return CurrentChance;
	}

	// 미발동 횟수 × 증가량, 최대 누적치 제한
	const float PityBonus = FMath::Min(
		static_cast<float>(*MissCount) * Config.IncrementPerMiss,
		Config.MaxAccumulation);

	return CurrentChance + PityBonus;
}

float UMidBossNotifyModifier::ApplyUsageModifier(
	const TArray<FUsageModifierEntry>& Entries, FName PatternName, float CurrentChance) const
{
	const int32* UsageCount = PatternUsageCounters.Find(PatternName);
	if (!UsageCount)
	{
		return CurrentChance;
	}

	// 가장 높은 매칭 구간 적용 (역순이 아닌 전체 탐색)
	float BestMultiplier = 1.0f;
	int32 BestThreshold = -1;

	for (const FUsageModifierEntry& Entry : Entries)
	{
		if (*UsageCount >= Entry.MinUsageCount && Entry.MinUsageCount > BestThreshold)
		{
			BestMultiplier = Entry.Multiplier;
			BestThreshold = Entry.MinUsageCount;
		}
	}

	return CurrentChance * BestMultiplier;
}

float UMidBossNotifyModifier::ApplyHPModifier(
	const TArray<FHPModifierEntry>& Entries, float HPPercent, float CurrentChance) const
{
	for (const FHPModifierEntry& Entry : Entries)
	{
		if (HPPercent >= Entry.MinHPPercent && HPPercent <= Entry.MaxHPPercent)
		{
			return CurrentChance * Entry.Multiplier;
		}
	}
	return CurrentChance;
}
