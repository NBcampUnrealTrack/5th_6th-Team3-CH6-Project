// T3MidBossMonster_Pattern.cpp — 패턴 실행/취소/검색, 노티파이 핸들러, 체인 몽타주, 쿨다운

#include "Monster/T3MidBossMonster.h"
#include "Monster/T3BossWeaponComponent.h"
#include "Desecration.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"
#include "Components/CapsuleComponent.h"
#include "DrawDebugHelpers.h"

// ============================================================
// 패턴 실행
// ============================================================

bool AT3MidBossMonster::ExecutePattern(FName PatternName, bool bAsReaction)
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
	// 리액션 모드 플래그 — PlaySectionComboFirstEntry 등 후속 단계에서 ReactionSectionNameOverride 적용 판단에 사용
	bIsCurrentPatternReaction = bAsReaction;
	// StartSectionIndex 적용 — 막기 리액션 등에서 앞쪽 N개 엔트리를 스킵하고 빠르게 진입
	// 리액션 모드(bAsReaction=true) + bAllowAsReaction + ReactionStartSectionOverride>=0 조합이면 오버라이드 우선
	// 범위 밖 값은 클램프 (마지막 엔트리만 남기는 케이스 허용)
	const int32 RawStartIndex = (bAsReaction && PatternData->bAllowAsReaction && PatternData->ReactionStartSectionOverride >= 0)
		? PatternData->ReactionStartSectionOverride
		: PatternData->StartSectionIndex;
	CurrentChainIndex = FMath::Clamp(RawStartIndex, 0, PatternData->MontageChain.Num() - 1);
	ConsecutiveDisengageCount = 0;
	AddStateTag(TAG_Boss_State_ExecutingPattern);

	// 보정기에 패턴 사용 횟수 기록
	if (NotifyModifier)
	{
		NotifyModifier->RecordPatternUsage(PatternName);
	}

	const TCHAR* SourceLabel =
		PendingReactionSource == EBossReactionSource::FromBlock ? TEXT("FromBlock") :
		PendingReactionSource == EBossReactionSource::FromRoll  ? TEXT("FromRoll")  : TEXT("None");
	UE_LOG(LogDesecration, Log,
		TEXT("T3_MidBoss: 패턴 실행 시작 — '%s' [%s] (체인 수: %d, Source:%s)"),
		*PatternName.ToString(),
		bAsReaction ? TEXT("리액션") : TEXT("일반"),
		PatternData->MontageChain.Num(),
		SourceLabel);

	// 리액션 트리거 1회 소모 — 다음 공격까지 영향 없게 즉시 None으로 클리어
	PendingReactionSource = EBossReactionSource::None;

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

	if (WeaponComponent)
	{
		WeaponComponent->SetAttackCollisionEnabled(false);
		WeaponComponent->SetWideCollisionEnabled(false);
	}
	bIsMovingToTarget = false;
	if (IsParryWindowActive()) { CloseParryWindow(); }
	if (MotionWarpingComponent) { MotionWarpingComponent->RemoveWarpTarget(MotionWarpTargetName); MotionWarpingComponent->RemoveWarpTarget(MotionWarpTargetRotationName); }
	ResetPatternState();
}

// ============================================================
// 패턴 검색
// ============================================================

TArray<FName> AT3MidBossMonster::CollectAvailablePatterns(TFunctionRef<bool(const FMidBossAttackPattern&)> ExtraFilter) const
{
	TArray<FName> Result;
	for (const FMidBossAttackPattern& Pattern : AttackPatterns)
	{
		// 공통: Stage 충족 + 쿨다운 OFF + 호출자별 추가 필터
		if (BossStage >= Pattern.RequiredStage
			&& IsPatternOffCooldown(Pattern.PatternName)
			&& ExtraFilter(Pattern))
		{
			Result.Add(Pattern.PatternName);
		}
	}
	return Result;
}

TArray<FName> AT3MidBossMonster::GetAvailablePatterns(EMidBossPatternCategory Category) const
{
	return CollectAvailablePatterns([Category](const FMidBossAttackPattern& Pattern)
	{
		return Pattern.Category == Category;
	});
}

TArray<FName> AT3MidBossMonster::GetAllAvailablePatterns() const
{
	return CollectAvailablePatterns([](const FMidBossAttackPattern&) { return true; });
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
		HandleDropWeaponNotify();
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
	if (Name.Equals(TEXT("Slow")))                   { HandleSlowNotify(AnimInstance, CurrentMontage); }
	else if (Name.Equals(TEXT("Fast")))              { HandleFastNotify(AnimInstance, CurrentMontage); }
	else if (Name.Equals(TEXT("Normal")))            { HandleNormalNotify(AnimInstance, CurrentMontage); }
	// --- 이동 + 속도 복구 (확률 적용) ---
	else if (Name.StartsWith(TEXT("Step")))          { HandleStepNotify(AnimInstance, CurrentMontage); }
	// --- 무기 판정 ON/OFF (항상 실행) ---
	else if (Name.Equals(TEXT("AttackStart")))       { SetWeaponCollisionByNotify(NotifyName, true); }
	else if (Name.Equals(TEXT("AttackEnd")))         { SetWeaponCollisionByNotify(NotifyName, false); }
	else if (Name.Equals(TEXT("WideAttackStart")))   { SetWeaponCollisionByNotify(NotifyName, true); }
	else if (Name.Equals(TEXT("WideAttackEnd")))     { SetWeaponCollisionByNotify(NotifyName, false); }
	else if (Name.Equals(TEXT("BodyAttackStart")))   { SetWeaponCollisionByNotify(NotifyName, true); }
	else if (Name.Equals(TEXT("BodyAttackEnd")))     { SetWeaponCollisionByNotify(NotifyName, false); }
	// --- 투사체/AoE/패링/워프/콤보 ---
	else if (Name.Equals(TEXT("SpawnProjectile")))   { HandleSpawnProjectileNotify(); }
	else if (Name.Equals(TEXT("GroundSlamPreview"))) { HandleGroundSlamPreviewNotify(); }
	else if (Name.Equals(TEXT("GroundSlam")))        { HandleGroundSlamNotify(); }
	else if (Name.Equals(TEXT("ParryWindowStart")))  { HandleParryWindowNotify(true); }
	else if (Name.Equals(TEXT("ParryWindowEnd")))    { HandleParryWindowNotify(false); }
	else if (Name.Equals(TEXT("WarpTarget")))        { HandleWarpTargetNotify(); }
	else if (Name.Equals(TEXT("NextCombo")))         { HandleNextComboNotify(AnimInstance, CurrentMontage); }
}

// ============================================================
// 노티파이 공통 헬퍼
// ============================================================

void AT3MidBossMonster::ApplyCurrentChainPlayRate(UAnimInstance* AnimInst, UAnimMontage* Montage, float ExtraMultiplier) const
{
	if (!AnimInst || !Montage)
	{
		return;
	}

	const FMidBossAttackPattern* PatternData = FindPatternData(CurrentPatternName);
	const float BaseRate = (PatternData && PatternData->MontageChain.IsValidIndex(CurrentChainIndex))
		? PatternData->MontageChain[CurrentChainIndex].PlayRate : 1.0f;

	// 천사 장신구 슬로우 누적 곱
	AnimInst->Montage_SetPlayRate(Montage, BaseRate * ExtraMultiplier * CurrentAttackAnimRate);
}

void AT3MidBossMonster::ProcessProbabilisticNotify(FName NotifyName, TFunctionRef<void()> OnTriggered)
{
	const bool bTriggered = ShouldTriggerNotify(NotifyName);
	if (bTriggered)
	{
		OnTriggered();
	}
	// Pity 결과 기록
	if (NotifyModifier)
	{
		NotifyModifier->RecordNotifyResult(NotifyName, bTriggered);
	}
}

void AT3MidBossMonster::SetWeaponCollisionByNotify(FName NotifyName, bool bEnabled)
{
	const FString Name = NotifyName.ToString();

	if (Name.Equals(TEXT("AttackStart")))
	{
		UE_LOG(LogDesecration, Log, TEXT("T3_MidBoss: AttackStart 노티파이 수신 (WeaponComponent: %s)"),
			WeaponComponent ? TEXT("유효") : TEXT("nullptr"));
		if (WeaponComponent) { WeaponComponent->SetAttackCollisionEnabled(bEnabled); }
	}
	else if (Name.Equals(TEXT("AttackEnd")))
	{
		UE_LOG(LogDesecration, Log, TEXT("T3_MidBoss: AttackEnd 노티파이 수신 — 전체 판정 OFF"));
		if (WeaponComponent)
		{
			WeaponComponent->SetAttackCollisionEnabled(bEnabled);
			WeaponComponent->SetWideCollisionEnabled(false);
			WeaponComponent->SetBodyAttackCollisionEnabled(false);
		}
	}
	else if (Name.Equals(TEXT("WideAttackStart")) || Name.Equals(TEXT("WideAttackEnd")))
	{
		UE_LOG(LogDesecration, Log, TEXT("T3_MidBoss: %s 노티파이 수신"), *Name);
		if (WeaponComponent) { WeaponComponent->SetWideCollisionEnabled(bEnabled); }
	}
	else if (Name.Equals(TEXT("BodyAttackStart")) || Name.Equals(TEXT("BodyAttackEnd")))
	{
		UE_LOG(LogDesecration, Log, TEXT("T3_MidBoss: %s 노티파이 수신"), *Name);
		if (WeaponComponent) { WeaponComponent->SetBodyAttackCollisionEnabled(bEnabled); }
	}
}

// ============================================================
// 노티파이 분기별 핸들러
// ============================================================

void AT3MidBossMonster::HandleDropWeaponNotify()
{
	if (WeaponComponent)
	{
		WeaponComponent->DropWeapon();
		UE_LOG(LogDesecration, Log, TEXT("T3_MidBoss: 노티파이로 무기 드롭"));
	}
}

void AT3MidBossMonster::HandleSlowNotify(UAnimInstance* AnimInst, UAnimMontage* Montage)
{
	ProcessProbabilisticNotify(FName(TEXT("Slow")), [&]()
	{
		// 보정기에서 범위 내 랜덤 배율 조회 → 몽타주 기본 PlayRate에 곱
		const float Multiplier = NotifyModifier ? NotifyModifier->GetSlowRate(CurrentPatternName) : 0.1f;
		ApplyCurrentChainPlayRate(AnimInst, Montage, Multiplier);
	});
}

void AT3MidBossMonster::HandleFastNotify(UAnimInstance* AnimInst, UAnimMontage* Montage)
{
	ProcessProbabilisticNotify(FName(TEXT("Fast")), [&]()
	{
		// 보정기에서 범위 내 랜덤 배율 조회 → 몽타주 기본 PlayRate에 곱
		const float Multiplier = NotifyModifier ? NotifyModifier->GetFastRate(CurrentPatternName) : 2.0f;
		ApplyCurrentChainPlayRate(AnimInst, Montage, Multiplier);
	});
}

void AT3MidBossMonster::HandleNormalNotify(UAnimInstance* AnimInst, UAnimMontage* Montage)
{
	// 속도 복구 — 몽타주의 원래 PlayRate로 복원 (몽타주 에디터 Rate Scale 유지)
	ApplyCurrentChainPlayRate(AnimInst, Montage);
}

void AT3MidBossMonster::HandleStepNotify(UAnimInstance* AnimInst, UAnimMontage* Montage)
{
	ProcessProbabilisticNotify(FName(TEXT("Step")), [&]()
	{
		// 속도 복구 — 몽타주의 원래 PlayRate로 복원 (몽타주 에디터 Rate Scale 유지)
		ApplyCurrentChainPlayRate(AnimInst, Montage);
		// 보정기에서 범위 내 랜덤 거리/시간 조회
		const float StepDist = NotifyModifier ? NotifyModifier->GetStepDistance(CurrentPatternName) : 200.f;
		const float StepDur = NotifyModifier ? NotifyModifier->GetStepDuration(CurrentPatternName) : 0.1f;
		MoveToTarget(StepDur, StepDist);
	});
}

void AT3MidBossMonster::HandleSpawnProjectileNotify()
{
	const FMidBossAttackPattern* PatternData = FindPatternData(CurrentPatternName);
	const bool bIsCounterPattern = PatternData && PatternData->bHasParryWindow;

	if (bIsCounterPattern)
	{
		if (bParrySucceeded)
		{
			// 플레이어가 때림 → 반격 휘두르기 (검기 안 나감, 근접 판정으로 대체)
			UE_LOG(LogDesecration, Log, TEXT("T3_MidBoss: 카운터 발동 — 검기 스킵, 근접 반격"));
		}
		else
		{
			// 안 때림 → 기 모으다 그냥 종료 (공격 안 함)
			UE_LOG(LogDesecration, Log, TEXT("T3_MidBoss: 카운터 패턴 — 반격 미발동, 검기 스킵"));
		}
	}
	else
	{
		// 일반 검기 패턴 — 투사체 발사
		SpawnBossProjectile();
	}
}

void AT3MidBossMonster::HandleGroundSlamPreviewNotify()
{
	const FVector AoECenter = GetActorLocation() - FVector(0.0, 0.0, static_cast<double>(GetCapsuleComponent()->GetScaledCapsuleHalfHeight()));

	if (AoEPreviewEffect)
	{
		const FVector ScaleVec = FVector(AoEEffectScale);
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			this, AoEPreviewEffect, AoECenter, GetActorRotation(), ScaleVec);
	}

#if WITH_EDITOR
	// 디버그 범위 표시 (노란색) — 이펙트 유무와 무관하게 항상 표시
	DrawDebugSphere(GetWorld(), AoECenter, AoERadius, 24,
		FColor::Yellow, false, 1.5f, 0, 3.f);
#endif

	// 프리뷰 경고 사운드
	PlayBossSoundAt(AoEPreviewSound, AoECenter, AoEPreviewVolumeMultiplier);

	UE_LOG(LogDesecration, Log, TEXT("T3_MidBoss: GroundSlamPreview — 범위 표시 (반경:%.0f)"), AoERadius);
}

void AT3MidBossMonster::HandleGroundSlamNotify()
{
	float AoEDamage = 20.f;
	EHitIntensity AoEIntensity = EHitIntensity::Heavy;
	TSubclassOf<UT3DamageType_Base> AoEDmgType = nullptr;
	GetCurrentHitData(AoEDamage, AoEIntensity, AoEDmgType);
	ExecuteAoEDamage(AoERadius, AoEDamage, AoEIntensity, AoEDmgType);
	UE_LOG(LogDesecration, Log, TEXT("T3_MidBoss: GroundSlam 노티파이 — AoE 발동 (반경:%.0f, 데미지:%.0f)"),
		AoERadius, AoEDamage);
}

void AT3MidBossMonster::HandleParryWindowNotify(bool bOpen)
{
	if (bOpen)
	{
		// bHasParryWindow 체크 — 카운터 패턴만 윈도우 오픈
		const FMidBossAttackPattern* PatternData = FindPatternData(CurrentPatternName);
		if (PatternData && PatternData->bHasParryWindow)
		{
			OpenParryWindow();
		}
	}
	else
	{
		CloseParryWindow();
	}
}

void AT3MidBossMonster::HandleWarpTargetNotify()
{
	UpdateMotionWarpTarget();
	UE_LOG(LogDesecration, Log, TEXT("T3_MidBoss: WarpTarget 노티파이 — 워프 위치 스냅샷"));
}

void AT3MidBossMonster::HandleNextComboNotify(UAnimInstance* AnimInst, UAnimMontage* Montage)
{
	const FMidBossAttackPattern* PatternData = FindPatternData(CurrentPatternName);
	if (!PatternData || !PatternData->bUseSectionCombo)
	{
		return;
	}

	CurrentChainIndex++;

	if (PatternData->MontageChain.IsValidIndex(CurrentChainIndex))
	{
		// 새 섹션의 PlayRate 적용 — SetNextSection은 배속을 변경하지 않으므로 명시적 갱신
		// 천사 장신구 슬로우 누적 곱
		const FPatternMontageData& CurrentEntry = PatternData->MontageChain[CurrentChainIndex];
		AnimInst->Montage_SetPlayRate(Montage, CurrentEntry.PlayRate * CurrentAttackAnimRate);

		// 동적 섹션 전환: 현재 섹션 → 다음 섹션 설정
		// 같은 섹션 이름이 반복되어도 매 NextCombo마다 갱신하므로 정확한 다음 섹션 지정
		SetupDynamicNextSection(AnimInst, PatternData->MontageChain[0].Montage, *PatternData, CurrentChainIndex);
	}

	UE_LOG(LogDesecration, Log,
		TEXT("T3_MidBoss: NextCombo — 섹션 진행 체인[%d] (패턴:'%s', 배속:%.1f)"),
		CurrentChainIndex, *CurrentPatternName.ToString(),
		PatternData->MontageChain.IsValidIndex(CurrentChainIndex) ? PatternData->MontageChain[CurrentChainIndex].PlayRate : -1.f);
}

bool AT3MidBossMonster::ShouldTriggerNotify(FName NotifyName) const
{
	const FMidBossAttackPattern* PatternData = FindPatternData(CurrentPatternName);
	if (!PatternData)
	{
		return true;
	}

	// 1. 노티파이 타입별 기본 확률 조회
	const float BaseChance = GetBaseNotifyChance(NotifyName, *PatternData);

	// 2. 보정기 적용 (거리/Pity/Usage/HP)
	float ModifiedChance = BaseChance;
	if (NotifyModifier)
	{
		const FNotifyModifierContext Context = BuildNotifyModifierContext(NotifyName);
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

float AT3MidBossMonster::GetBaseNotifyChance(FName NotifyName, const FMidBossAttackPattern& PatternData) const
{
	const FString Name = NotifyName.ToString();

	if (Name.Equals(TEXT("Slow")))
	{
		return PatternData.NotifyChances.SlowChance;
	}
	if (Name.Equals(TEXT("Fast")))
	{
		return PatternData.NotifyChances.FastChance;
	}
	if (Name.Equals(TEXT("Step")))
	{
		return PatternData.NotifyChances.StepChance;
	}

	// 그 외 노티파이는 항상 발동 (기본 확률 1.0)
	return 1.0f;
}

FNotifyModifierContext AT3MidBossMonster::BuildNotifyModifierContext(FName NotifyName) const
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

	return Context;
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
	const double CurrentTime = GetWorld()->GetTimeSeconds();

	// 1. 개별 쿨다운 체크
	const double* PatternExpire = PatternCooldownExpireMap.Find(PatternName);
	if (PatternExpire && CurrentTime < *PatternExpire)
	{
		return false;
	}

	// 2. 그룹 쿨다운 체크
	const FMidBossAttackPattern* PatternData = FindPatternData(PatternName);
	if (PatternData && !PatternData->CooldownGroup.IsNone())
	{
		const double* GroupExpire = PatternCooldownExpireMap.Find(PatternData->CooldownGroup);
		if (GroupExpire && CurrentTime < *GroupExpire)
		{
			return false;
		}
	}

	return true;
}

void AT3MidBossMonster::RegisterCooldown(FName PatternName, float CooldownSeconds)
{
	const double CurrentTime = GetWorld()->GetTimeSeconds();

	// 1. 개별 쿨다운 등록
	if (CooldownSeconds > 0.f)
	{
		PatternCooldownExpireMap.Add(PatternName, CurrentTime + static_cast<double>(CooldownSeconds));
	}

	// 2. 그룹 쿨다운 등록
	const FMidBossAttackPattern* PatternData = FindPatternData(PatternName);
	if (PatternData && !PatternData->CooldownGroup.IsNone() && PatternData->GroupCooldown > 0.f)
	{
		PatternCooldownExpireMap.Add(PatternData->CooldownGroup, CurrentTime + static_cast<double>(PatternData->GroupCooldown));
	}
}

// ============================================================
// 내부 함수 (패턴 체인)
// ============================================================

void AT3MidBossMonster::GetCurrentHitData(float& OutDamage, EHitIntensity& OutIntensity, TSubclassOf<UT3DamageType_Base>& OutDamageType) const
{
	const FMidBossAttackPattern* PatternData = FindPatternData(CurrentPatternName);
	if (!PatternData || !PatternData->MontageChain.IsValidIndex(CurrentChainIndex))
	{
		OutDamage = 20.f;
		OutIntensity = EHitIntensity::Light;
		OutDamageType = nullptr;
		return;
	}

	const FPatternMontageData& MontageData = PatternData->MontageChain[CurrentChainIndex];
	OutDamage = MontageData.Damage;
	OutIntensity = MontageData.HitIntensity;
	OutDamageType = MontageData.DamageTypeClass;
}

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

	// MotionWarping — 몽타주 시작 시 자동 호출 안 함
	// 몽타주에 WarpTarget 노티파이를 배치하여 원하는 타이밍에 스냅샷

	// 섹션 콤보 첫 진입 vs 일반 체인 모드 (섹션 콤보 후속의 다른 몽타주 포함)
	if (PatternData->bUseSectionCombo && CurrentChainIndex == 0)
	{
		PlaySectionComboFirstEntry(*PatternData, MontageData);
	}
	else
	{
		// 리액션 배율은 패턴 단위로 전달 — 모든 체인 엔트리가 동일 가속을 적용받도록
		PlayChainMontageEntry(MontageData, PatternData->ReactionPlayRateMultiplier);
	}
}

// ============================================================
// 체인 몽타주 재생 헬퍼
// ============================================================

void AT3MidBossMonster::SetupDynamicNextSection(UAnimInstance* AnimInst, UAnimMontage* SectionMontage,
	const FMidBossAttackPattern& PatternData, int32 CurrentIdx) const
{
	if (!AnimInst || !SectionMontage || !PatternData.MontageChain.IsValidIndex(CurrentIdx))
	{
		return;
	}

	const FPatternMontageData& CurrentEntry = PatternData.MontageChain[CurrentIdx];
	if (CurrentEntry.SectionName.IsNone())
	{
		// 현재 섹션 이름이 없으면 SetNextSection 호출 자체가 무의미
		return;
	}

	const int32 NextIndex = CurrentIdx + 1;

	// 다음 인덱스가 있고 같은 몽타주(또는 nullptr)이며 섹션 이름이 있으면 → 체인
	// 그 외 → 현재 섹션에서 종료 (NAME_None)
	if (PatternData.MontageChain.IsValidIndex(NextIndex))
	{
		const FPatternMontageData& NextEntry = PatternData.MontageChain[NextIndex];
		const bool bNextIsSameMontage = (NextEntry.Montage == nullptr || NextEntry.Montage == SectionMontage);

		if (bNextIsSameMontage && !NextEntry.SectionName.IsNone())
		{
			AnimInst->Montage_SetNextSection(CurrentEntry.SectionName, NextEntry.SectionName, SectionMontage);
			return;
		}
	}

	// 다른 몽타주 경계 / 마지막 섹션 — 현재 섹션에서 종료
	AnimInst->Montage_SetNextSection(CurrentEntry.SectionName, NAME_None, SectionMontage);
}

void AT3MidBossMonster::PlaySectionComboFirstEntry(const FMidBossAttackPattern& PatternData,
	const FPatternMontageData& MontageData)
{
	// 시작 섹션 결정 — 리액션 모드 + bAllowAsReaction + ReactionSectionNameOverride 지정이면 오버라이드 우선
	// 그 외엔 MontageChain[0].SectionName 그대로 (없으면 NAME_None)
	FName StartSection = MontageData.SectionName.IsNone() ? NAME_None : MontageData.SectionName;
	if (bIsCurrentPatternReaction && PatternData.bAllowAsReaction
		&& !PatternData.ReactionSectionNameOverride.IsNone())
	{
		StartSection = PatternData.ReactionSectionNameOverride;
	}

	// 천사 장신구 슬로우 누적 곱 + 리액션 가속 배율
	const float FinalPlayRate = MontageData.PlayRate * CurrentAttackAnimRate * PatternData.ReactionPlayRateMultiplier;
	PlayAnimMontage(MontageData.Montage, FinalPlayRate, StartSection);

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance)
	{
		UAnimMontage* SectionMontage = MontageData.Montage;

		// 동적 섹션 전환: 첫 번째 전환만 설정 (나머지는 NextCombo에서 동적 설정)
		// 같은 섹션 이름이 반복될 때 SetNextSection 덮어쓰기 문제 방지
		SetupDynamicNextSection(AnimInstance, SectionMontage, PatternData, 0);

		FOnMontageEnded EndDelegate;
		EndDelegate.BindUObject(this, &AT3MidBossMonster::OnMontageEnded);
		AnimInstance->Montage_SetEndDelegate(EndDelegate, SectionMontage);
	}

	UE_LOG(LogDesecration, Log,
		TEXT("T3_MidBoss: 섹션 콤보 시작 — 패턴:'%s' 섹션:'%s' (최종배속:%.2f, 리액션×%.2f, 섹션 수:%d)"),
		*CurrentPatternName.ToString(),
		StartSection.IsNone() ? TEXT("Default") : *StartSection.ToString(),
		FinalPlayRate, PatternData.ReactionPlayRateMultiplier, PatternData.MontageChain.Num());
}

void AT3MidBossMonster::PlayChainMontageEntry(const FPatternMontageData& MontageData, float ReactionPlayRateMultiplier)
{
	// 천사 장신구 슬로우 누적 곱 + 리액션 가속 배율
	const FName StartSection = MontageData.SectionName.IsNone() ? NAME_None : MontageData.SectionName;
	const float FinalPlayRate = MontageData.PlayRate * CurrentAttackAnimRate * ReactionPlayRateMultiplier;
	PlayAnimMontage(MontageData.Montage, FinalPlayRate, StartSection);

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance)
	{
		// 섹션 지정 시 해당 섹션만 재생하고 종료 — 기본 섹션 순서 진행 방지
		if (!StartSection.IsNone())
		{
			AnimInstance->Montage_SetNextSection(StartSection, NAME_None, MontageData.Montage);
		}

		// 체인 모드: BlendingOut 시작 시점에 다음 몽타주 겹쳐 재생 (idle 깜박임 방지)
		FOnMontageBlendingOutStarted BlendOutDelegate;
		BlendOutDelegate.BindUObject(this, &AT3MidBossMonster::OnChainBlendingOut);
		AnimInstance->Montage_SetBlendingOutDelegate(BlendOutDelegate, MontageData.Montage);
	}

	UE_LOG(LogDesecration, Log,
		TEXT("T3_MidBoss: 몽타주 재생 — 패턴:'%s' 체인[%d] (최종배속:%.2f, 리액션×%.2f, 데미지:%.0f, 섹션:'%s')"),
		*CurrentPatternName.ToString(), CurrentChainIndex,
		FinalPlayRate, ReactionPlayRateMultiplier, MontageData.Damage,
		StartSection.IsNone() ? TEXT("None") : *StartSection.ToString());
}

bool AT3MidBossMonster::HandleChainCallbackPrelude(bool bInterrupted, const TCHAR* InterruptLogPrefix)
{
	if (!IsExecutingPattern())
	{
		return true;
	}

	if (WeaponComponent)
	{
		WeaponComponent->SetAttackCollisionEnabled(false);
		WeaponComponent->SetWideCollisionEnabled(false);
	}
	bIsMovingToTarget = false;

	if (bInterrupted)
	{
		UE_LOG(LogDesecration, Log,
			TEXT("T3_MidBoss: %s 인터럽트 — 패턴:'%s' 체인[%d]"),
			InterruptLogPrefix, *CurrentPatternName.ToString(), CurrentChainIndex);
		ResetPatternState();
		return true;
	}

	return false;
}

void AT3MidBossMonster::OnChainBlendingOut(UAnimMontage* Montage, bool bInterrupted)
{
	if (HandleChainCallbackPrelude(bInterrupted, TEXT("체인 블렌드아웃")))
	{
		return;
	}

	// 블렌드아웃 시작 시점에 바로 다음 체인 진행 → Blend Out + Blend In 겹침
	AdvanceChainOrComplete();
}

void AT3MidBossMonster::OnMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	if (HandleChainCallbackPrelude(bInterrupted, TEXT("몽타주")))
	{
		return;
	}

	// === 섹션 콤보 모드: 섹션 몽타주 종료 후 다른 몽타주 체인 확인 ===
	const FMidBossAttackPattern* PatternData = FindPatternData(CurrentPatternName);
	if (PatternData && PatternData->bUseSectionCombo && PatternData->MontageChain.Num() > 0)
	{
		UAnimMontage* SectionMontage = PatternData->MontageChain[0].Montage;

		// 섹션 몽타주가 끝난 경우에만 후속 체인 확인
		if (Montage == SectionMontage)
		{
			// 다른 몽타주를 가진 첫 번째 엔트리 찾기
			for (int32 i = 1; i < PatternData->MontageChain.Num(); ++i)
			{
				UAnimMontage* EntryMontage = PatternData->MontageChain[i].Montage;
				if (EntryMontage != nullptr && EntryMontage != SectionMontage)
				{
					// 후속 체인 몽타주 발견 → 전환
					CurrentChainIndex = i;
					UE_LOG(LogDesecration, Log,
						TEXT("T3_MidBoss: 섹션 콤보 → 체인 전환 — 패턴:'%s' 체인[%d]"),
						*CurrentPatternName.ToString(), CurrentChainIndex);
					PlayCurrentChainMontage();
					return;
				}
			}
		}
		// 후속 몽타주 없거나, 후속 몽타주가 끝난 경우 → 패턴 완료
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
	// 쿨다운을 패턴 종료/중단 시점부터 등록 (몽타주 재생 시간이 쿨다운에 포함되지 않도록)
	if (!CurrentPatternName.IsNone())
	{
		const FMidBossAttackPattern* PatternData = FindPatternData(CurrentPatternName);
		if (PatternData && PatternData->Cooldown > 0.f)
		{
			RegisterCooldown(CurrentPatternName, PatternData->Cooldown);
		}
	}

	CurrentPatternName = NAME_None;
	CurrentChainIndex = 0;
	bParrySucceeded = false;

	// 패링 윈도우 타이머 정리 (패턴 종료 후 다음 패턴에 영향 방지)
	if (IsParryWindowActive())
	{
		GetWorldTimerManager().ClearTimer(ParryWindowTimerHandle);
		RemoveStateTag(TAG_Boss_State_ParryWindow);
	}

	RemoveStateTag(TAG_Boss_State_ExecutingPattern);
	if (MotionWarpingComponent) { MotionWarpingComponent->RemoveWarpTarget(MotionWarpTargetName); MotionWarpingComponent->RemoveWarpTarget(MotionWarpTargetRotationName); }

	// 소켓 스위칭 안전 복귀 — 패턴 중단 시 ANS NotifyEnd가 호출 안 될 수 있음
	if (WeaponComponent) { WeaponComponent->ResetToDefaultSocket(); }
}
