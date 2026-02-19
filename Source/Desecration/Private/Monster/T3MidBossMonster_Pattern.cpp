// T3MidBossMonster_Pattern.cpp — 패턴 실행/취소/검색, 노티파이 핸들러, 체인 몽타주, 쿨다운

#include "Monster/T3MidBossMonster.h"
#include "Monster/T3BossWeaponComponent.h"
#include "Desecration.h"

// ============================================================
// 패턴 실행
// ============================================================

bool AT3MidBossMonster::ExecutePattern(FName PatternName)
{
	// 블로킹 태그 일괄 체크 (Dead, Stunned, ExecutingPattern 중 하나라도 있으면 실행 불가)
	FGameplayTagContainer BlockingTags;
	BlockingTags.AddTag(TAG_Boss_State_Dead);
	BlockingTags.AddTag(TAG_Boss_State_Stunned);
	BlockingTags.AddTag(TAG_Boss_State_ExecutingPattern);

	if (ActiveGameplayTags.HasAny(BlockingTags))
	{
		UE_LOG(LogDesecration, Warning,
			TEXT("T3_MidBoss: ExecutePattern(%s) 실패 (Dead:%d, Stun:%d, Executing:%d)"),
			*PatternName.ToString(), IsDead(), IsStunned(), IsExecutingPattern());
		return false;
	}

	const FMidBossAttackPattern* PatternData = FindPatternData(PatternName);
	if (!PatternData)
	{
		UE_LOG(LogDesecration, Warning,
			TEXT("T3_MidBoss: 패턴 '%s' 데이터를 찾을 수 없음"), *PatternName.ToString());
		return false;
	}

	if (BossStage < PatternData->RequiredStage)
	{
		UE_LOG(LogDesecration, Warning,
			TEXT("T3_MidBoss: 패턴 '%s' 스테이지 미달 (현재:%d, 필요:%d)"),
			*PatternName.ToString(), BossStage, PatternData->RequiredStage);
		return false;
	}

	if (!IsPatternOffCooldown(PatternName))
	{
		UE_LOG(LogDesecration, Warning,
			TEXT("T3_MidBoss: 패턴 '%s' 쿨다운 중"), *PatternName.ToString());
		return false;
	}

	if (PatternData->MontageChain.Num() == 0)
	{
		UE_LOG(LogDesecration, Warning,
			TEXT("T3_MidBoss: 패턴 '%s' MontageChain이 비어 있음"), *PatternName.ToString());
		return false;
	}

	// 패턴 실행 시작
	CurrentPatternName = PatternName;
	CurrentChainIndex = 0;
	AddStateTag(TAG_Boss_State_ExecutingPattern);

	if (PatternData->Cooldown > 0.f)
	{
		RegisterCooldown(PatternName, PatternData->Cooldown);
	}

	// 보정기에 패턴 사용 횟수 기록
	if (NotifyModifier)
	{
		NotifyModifier->RecordPatternUsage(PatternName);
	}

	UE_LOG(LogDesecration, Log,
		TEXT("T3_MidBoss: 패턴 실행 시작 — '%s' (체인 수: %d)"),
		*PatternName.ToString(), PatternData->MontageChain.Num());

	PlayCurrentChainMontage();
	return true;
}

void AT3MidBossMonster::CancelCurrentPattern()
{
	if (!IsExecutingPattern())
	{
		return;
	}

	UE_LOG(LogDesecration, Log,
		TEXT("T3_MidBoss: 패턴 강제 중단 — '%s'"), *CurrentPatternName.ToString());

	// 현재 패턴 몽타주를 명시적으로 중단 (히트 리액션과 혼동 방지)
	const FMidBossAttackPattern* PatternData = FindPatternData(CurrentPatternName);
	if (PatternData && PatternData->MontageChain.IsValidIndex(CurrentChainIndex))
	{
		StopAnimMontage(PatternData->MontageChain[CurrentChainIndex].Montage);
	}
	else
	{
		StopAnimMontage();
	}

	if (WeaponComponent) { WeaponComponent->SetAttackCollisionEnabled(false); }
	bIsMovingToTarget = false;
	if (MotionWarpingComponent) { MotionWarpingComponent->RemoveWarpTarget(MotionWarpTargetName); MotionWarpingComponent->RemoveWarpTarget(MotionWarpTargetRotationName); }
	ResetPatternState();
}

// ============================================================
// 패턴 검색
// ============================================================

TArray<FName> AT3MidBossMonster::GetAvailablePatterns(EMidBossPatternCategory Category) const
{
	TArray<FName> Result;
	for (const FMidBossAttackPattern& Pattern : AttackPatterns)
	{
		if (Pattern.Category == Category
			&& BossStage >= Pattern.RequiredStage
			&& IsPatternOffCooldown(Pattern.PatternName))
		{
			Result.Add(Pattern.PatternName);
		}
	}
	return Result;
}

TArray<FName> AT3MidBossMonster::GetAllAvailablePatterns() const
{
	TArray<FName> Result;
	for (const FMidBossAttackPattern& Pattern : AttackPatterns)
	{
		if (BossStage >= Pattern.RequiredStage
			&& IsPatternOffCooldown(Pattern.PatternName))
		{
			Result.Add(Pattern.PatternName);
		}
	}
	return Result;
}

bool AT3MidBossMonster::GetPatternData(FName PatternName, FMidBossAttackPattern& OutData) const
{
	const FMidBossAttackPattern* Found = FindPatternData(PatternName);
	if (Found)
	{
		OutData = *Found;
		return true;
	}
	return false;
}

// ============================================================
// 노티파이 핸들러
// ============================================================

void AT3MidBossMonster::HandlePatternNotify(FName NotifyName)
{
	const FString Name = NotifyName.ToString();

	// --- 상태 무관 노티파이 (사망 몽타주 등에서도 동작) ---
	if (Name.Equals(TEXT("DropWeapon")))
	{
		if (WeaponComponent)
		{
			WeaponComponent->DropWeapon();
			UE_LOG(LogDesecration, Log, TEXT("T3_MidBoss: 노티파이로 무기 드롭"));
		}
		return;
	}

	if (!IsExecutingPattern())
	{
		return;
	}

	UE_LOG(LogDesecration, Verbose,
		TEXT("T3_MidBoss: 노티파이 수신 — %s (패턴:'%s', 체인:%d)"),
		*Name, *CurrentPatternName.ToString(), CurrentChainIndex);

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (!AnimInstance)
	{
		return;
	}

	UAnimMontage* CurrentMontage = AnimInstance->GetCurrentActiveMontage();

	// --- 배속 제어 (확률 적용 대상) ---
	if (Name.Equals(TEXT("Slow")))
	{
		const bool bTriggered = ShouldTriggerNotify(NotifyName);
		if (bTriggered)
		{
			// 보정기에서 범위 내 랜덤 배속 조회
			const float Rate = NotifyModifier ? NotifyModifier->GetSlowRate(CurrentPatternName) : 0.1f;
			AnimInstance->Montage_SetPlayRate(CurrentMontage, Rate);
		}
		// Pity 결과 기록
		if (NotifyModifier)
		{
			NotifyModifier->RecordNotifyResult(NotifyName, bTriggered);
		}
	}
	else if (Name.Equals(TEXT("Fast")))
	{
		const bool bTriggered = ShouldTriggerNotify(NotifyName);
		if (bTriggered)
		{
			const float Rate = NotifyModifier ? NotifyModifier->GetFastRate(CurrentPatternName) : 2.0f;
			AnimInstance->Montage_SetPlayRate(CurrentMontage, Rate);
		}
		if (NotifyModifier)
		{
			NotifyModifier->RecordNotifyResult(NotifyName, bTriggered);
		}
	}
	else if (Name.Equals(TEXT("Normal")))
	{
		// 속도 복구는 항상 실행
		AnimInstance->Montage_SetPlayRate(CurrentMontage, 1.0f);
	}
	// --- 이동 + 속도 복구 (확률 적용) ---
	else if (Name.StartsWith(TEXT("Step")))
	{
		const bool bTriggered = ShouldTriggerNotify(FName(TEXT("Step")));
		if (bTriggered)
		{
			AnimInstance->Montage_SetPlayRate(CurrentMontage, 1.0f);
			// 보정기에서 범위 내 랜덤 거리/시간 조회
			const float StepDist = NotifyModifier ? NotifyModifier->GetStepDistance(CurrentPatternName) : 200.f;
			const float StepDur = NotifyModifier ? NotifyModifier->GetStepDuration(CurrentPatternName) : 0.1f;
			MoveToTarget(StepDur, StepDist);
		}
		if (NotifyModifier)
		{
			NotifyModifier->RecordNotifyResult(FName(TEXT("Step")), bTriggered);
		}
	}
	// --- 판정 ON/OFF (항상 실행) ---
	else if (Name.Equals(TEXT("AttackStart")))
	{
		UE_LOG(LogDesecration, Log, TEXT("T3_MidBoss: AttackStart 노티파이 수신 (WeaponComponent: %s)"),
			WeaponComponent ? TEXT("유효") : TEXT("nullptr"));
		if (WeaponComponent) { WeaponComponent->SetAttackCollisionEnabled(true); }
	}
	else if (Name.Equals(TEXT("AttackEnd")))
	{
		UE_LOG(LogDesecration, Log, TEXT("T3_MidBoss: AttackEnd 노티파이 수신"));
		if (WeaponComponent) { WeaponComponent->SetAttackCollisionEnabled(false); }
	}
}

bool AT3MidBossMonster::ShouldTriggerNotify(FName NotifyName) const
{
	const FMidBossAttackPattern* PatternData = FindPatternData(CurrentPatternName);
	if (!PatternData)
	{
		return true;
	}

	// 1. 노티파이 타입별 기본 확률 조회
	const FString Name = NotifyName.ToString();
	float BaseChance = 1.0f;

	if (Name.Equals(TEXT("Slow")))
	{
		BaseChance = PatternData->NotifyChances.SlowChance;
	}
	else if (Name.Equals(TEXT("Fast")))
	{
		BaseChance = PatternData->NotifyChances.FastChance;
	}
	else if (Name.Equals(TEXT("Step")))
	{
		BaseChance = PatternData->NotifyChances.StepChance;
	}

	// 2. 보정기 적용 (거리/Pity/Usage/HP)
	float ModifiedChance = BaseChance;
	if (NotifyModifier)
	{
		FNotifyModifierContext Context;
		Context.PatternName = CurrentPatternName;
		Context.NotifyName = NotifyName;

		// 거리 계산
		if (CombatTarget)
		{
			Context.DistanceToTarget = FVector::Dist(GetActorLocation(), CombatTarget->GetActorLocation());
		}

		// HP 비율 계산
		if (MidBossStats.MaxHP > 0.f)
		{
			Context.HPPercent = MidBossStats.CurrentHP / MidBossStats.MaxHP;
		}

		ModifiedChance = NotifyModifier->CalculateFinalChance(BaseChance, Context);
	}

	// 3. BP 오버라이드로 최종 보정 (추가적인 상황별 조절)
	const float FinalChance = ModifyNotifyChance(NotifyName, ModifiedChance);

	if (FinalChance >= 1.0f)
	{
		return true;
	}
	if (FinalChance <= 0.f)
	{
		return false;
	}

	return FMath::FRand() < FinalChance;
}

float AT3MidBossMonster::ModifyNotifyChance_Implementation(FName NotifyName, float BaseChance) const
{
	// 기본 구현: 보정 없이 그대로 반환
	// BP에서 오버라이드하여 거리/HP/Stage 등에 따라 확률 조절
	return BaseChance;
}

// ============================================================
// 쿨다운
// ============================================================

bool AT3MidBossMonster::IsPatternOffCooldown(FName PatternName) const
{
	const double* ExpireTime = PatternCooldownExpireMap.Find(PatternName);
	if (!ExpireTime)
	{
		return true;
	}

	const double CurrentTime = GetWorld()->GetTimeSeconds();
	return CurrentTime >= *ExpireTime;
}

void AT3MidBossMonster::RegisterCooldown(FName PatternName, float CooldownSeconds)
{
	const double ExpireTime = GetWorld()->GetTimeSeconds() + static_cast<double>(CooldownSeconds);
	PatternCooldownExpireMap.Add(PatternName, ExpireTime);
}

// ============================================================
// 내부 함수 (패턴 체인)
// ============================================================

const FMidBossAttackPattern* AT3MidBossMonster::FindPatternData(FName PatternName) const
{
	for (const FMidBossAttackPattern& Pattern : AttackPatterns)
	{
		if (Pattern.PatternName == PatternName)
		{
			return &Pattern;
		}
	}
	return nullptr;
}

void AT3MidBossMonster::PlayCurrentChainMontage()
{
	const FMidBossAttackPattern* PatternData = FindPatternData(CurrentPatternName);
	if (!PatternData || !PatternData->MontageChain.IsValidIndex(CurrentChainIndex))
	{
		AdvanceChainOrComplete();
		return;
	}

	const FPatternMontageData& MontageData = PatternData->MontageChain[CurrentChainIndex];

	if (!MontageData.Montage)
	{
		UE_LOG(LogDesecration, Warning,
			TEXT("T3_MidBoss: 패턴 '%s' 체인[%d] 몽타주가 nullptr"),
			*CurrentPatternName.ToString(), CurrentChainIndex);
		AdvanceChainOrComplete();
		return;
	}

	// MotionWarping 타겟 갱신 (ANS_MotionWarping 있는 몽타주에서만 실제 워프 발생)
	UpdateMotionWarpTarget();

	// 몽타주 재생 먼저 → 그 다음 EndDelegate 등록 (재생 중이어야 delegate가 걸림)
	PlayAnimMontage(MontageData.Montage, MontageData.PlayRate);

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance)
	{
		FOnMontageEnded EndDelegate;
		EndDelegate.BindUObject(this, &AT3MidBossMonster::OnMontageEnded);
		AnimInstance->Montage_SetEndDelegate(EndDelegate, MontageData.Montage);
	}

	UE_LOG(LogDesecration, Log,
		TEXT("T3_MidBoss: 몽타주 재생 — 패턴:'%s' 체인[%d] (배속:%.1f, 데미지:%.0f)"),
		*CurrentPatternName.ToString(), CurrentChainIndex,
		MontageData.PlayRate, MontageData.Damage);
}

void AT3MidBossMonster::OnMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	if (!IsExecutingPattern())
	{
		return;
	}

	if (WeaponComponent) { WeaponComponent->SetAttackCollisionEnabled(false); }
	bIsMovingToTarget = false;

	if (bInterrupted)
	{
		UE_LOG(LogDesecration, Log,
			TEXT("T3_MidBoss: 몽타주 인터럽트 — 패턴:'%s' 체인[%d]"),
			*CurrentPatternName.ToString(), CurrentChainIndex);
		ResetPatternState();
		return;
	}

	AdvanceChainOrComplete();
}

void AT3MidBossMonster::AdvanceChainOrComplete()
{
	const FMidBossAttackPattern* PatternData = FindPatternData(CurrentPatternName);

	CurrentChainIndex++;

	if (PatternData && PatternData->MontageChain.IsValidIndex(CurrentChainIndex))
	{
		PlayCurrentChainMontage();
		return;
	}

	const FName CompletedName = CurrentPatternName;

	UE_LOG(LogDesecration, Log,
		TEXT("T3_MidBoss: 패턴 완료 — '%s'"), *CompletedName.ToString());

	ResetPatternState();
	OnPatternCompleted.Broadcast(CompletedName);
	OnPatternCompletedNative.Broadcast();
}

void AT3MidBossMonster::ResetPatternState()
{
	CurrentPatternName = NAME_None;
	CurrentChainIndex = 0;
	RemoveStateTag(TAG_Boss_State_ExecutingPattern);
	if (MotionWarpingComponent) { MotionWarpingComponent->RemoveWarpTarget(MotionWarpTargetName); MotionWarpingComponent->RemoveWarpTarget(MotionWarpTargetRotationName); }
}
