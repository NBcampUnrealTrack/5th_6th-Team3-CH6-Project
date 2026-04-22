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
	ConsecutiveDisengageCount = 0;
	AddStateTag(TAG_Boss_State_ExecutingPattern);

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
			// 보정기에서 범위 내 랜덤 배율 조회 → 몽타주 기본 PlayRate에 곱
			const float Multiplier = NotifyModifier ? NotifyModifier->GetSlowRate(CurrentPatternName) : 0.1f;
			const FMidBossAttackPattern* SlowPatternData = FindPatternData(CurrentPatternName);
			const float BaseRate = (SlowPatternData && SlowPatternData->MontageChain.IsValidIndex(CurrentChainIndex))
				? SlowPatternData->MontageChain[CurrentChainIndex].PlayRate : 1.0f;
			// 천사 장신구 슬로우 누적 곱
			AnimInstance->Montage_SetPlayRate(CurrentMontage, BaseRate * Multiplier * CurrentAttackAnimRate);
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
			// 보정기에서 범위 내 랜덤 배율 조회 → 몽타주 기본 PlayRate에 곱
			const float FastMultiplier = NotifyModifier ? NotifyModifier->GetFastRate(CurrentPatternName) : 2.0f;
			const FMidBossAttackPattern* FastPatternData = FindPatternData(CurrentPatternName);
			const float FastBaseRate = (FastPatternData && FastPatternData->MontageChain.IsValidIndex(CurrentChainIndex))
				? FastPatternData->MontageChain[CurrentChainIndex].PlayRate : 1.0f;
			// 천사 장신구 슬로우 누적 곱
			AnimInstance->Montage_SetPlayRate(CurrentMontage, FastBaseRate * FastMultiplier * CurrentAttackAnimRate);
		}
		if (NotifyModifier)
		{
			NotifyModifier->RecordNotifyResult(NotifyName, bTriggered);
		}
	}
	else if (Name.Equals(TEXT("Normal")))
	{
		// 속도 복구 — 몽타주의 원래 PlayRate로 복원 (몽타주 에디터 Rate Scale 유지)
		const FMidBossAttackPattern* PatternData = FindPatternData(CurrentPatternName);
		if (PatternData && PatternData->MontageChain.IsValidIndex(CurrentChainIndex))
		{
			// 천사 장신구 슬로우 누적 곱
			AnimInstance->Montage_SetPlayRate(CurrentMontage, PatternData->MontageChain[CurrentChainIndex].PlayRate * CurrentAttackAnimRate);
		}
	}
	// --- 이동 + 속도 복구 (확률 적용) ---
	else if (Name.StartsWith(TEXT("Step")))
	{
		const bool bTriggered = ShouldTriggerNotify(FName(TEXT("Step")));
		if (bTriggered)
		{
			// 속도 복구 — 몽타주의 원래 PlayRate로 복원 (몽타주 에디터 Rate Scale 유지)
			const FMidBossAttackPattern* StepPatternData = FindPatternData(CurrentPatternName);
			if (StepPatternData && StepPatternData->MontageChain.IsValidIndex(CurrentChainIndex))
			{
				// 천사 장신구 슬로우 누적 곱
				AnimInstance->Montage_SetPlayRate(CurrentMontage, StepPatternData->MontageChain[CurrentChainIndex].PlayRate * CurrentAttackAnimRate);
			}
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
		UE_LOG(LogDesecration, Log, TEXT("T3_MidBoss: AttackEnd 노티파이 수신 — 전체 판정 OFF"));
		if (WeaponComponent)
		{
			WeaponComponent->SetAttackCollisionEnabled(false);
			WeaponComponent->SetWideCollisionEnabled(false);
			WeaponComponent->SetBodyAttackCollisionEnabled(false);
		}
	}
	// --- 투사체 스폰 (검기) — 카운터 패턴이면 스킵 ---
	else if (Name.Equals(TEXT("SpawnProjectile")))
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
	// --- AoE 프리뷰 (범위 표시만, 데미지 없음) ---
	else if (Name.Equals(TEXT("GroundSlamPreview")))
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
		if (AoEPreviewSound)
		{
			UGameplayStatics::PlaySoundAtLocation(
				this, AoEPreviewSound, AoECenter, FRotator::ZeroRotator,
				SoundVolume * AoEPreviewVolumeMultiplier, 1.f, 0.f, SoundAttenuationSettings);
		}

		UE_LOG(LogDesecration, Log, TEXT("T3_MidBoss: GroundSlamPreview — 범위 표시 (반경:%.0f)"), AoERadius);
	}
	// --- AoE 장판기 발동 ---
	else if (Name.Equals(TEXT("GroundSlam")))
	{
		float AoEDamage = 20.f;
		EHitIntensity AoEIntensity = EHitIntensity::Heavy;
		TSubclassOf<UT3DamageType_Base> AoEDmgType = nullptr;
		GetCurrentHitData(AoEDamage, AoEIntensity, AoEDmgType);
		ExecuteAoEDamage(AoERadius, AoEDamage, AoEIntensity);
		UE_LOG(LogDesecration, Log, TEXT("T3_MidBoss: GroundSlam 노티파이 — AoE 발동 (반경:%.0f, 데미지:%.0f)"),
			AoERadius, AoEDamage);
	}
	// --- 패링 윈도우 ON/OFF — bHasParryWindow 체크 ---
	else if (Name.Equals(TEXT("ParryWindowStart")))
	{
		const FMidBossAttackPattern* PatternData = FindPatternData(CurrentPatternName);
		if (PatternData && PatternData->bHasParryWindow)
		{
			OpenParryWindow();
		}
	}
	else if (Name.Equals(TEXT("ParryWindowEnd")))
	{
		CloseParryWindow();
	}
	// --- 모션 워프 스냅샷 (ANS_MotionWarping 시작 프레임에 배치) ---
	else if (Name.Equals(TEXT("WarpTarget")))
	{
		UpdateMotionWarpTarget();
		UE_LOG(LogDesecration, Log, TEXT("T3_MidBoss: WarpTarget 노티파이 — 워프 위치 스냅샷"));
	}
	// --- 섹션 콤보: 다음 섹션 진입 시 데미지 데이터 인덱스 갱신 + 동적 섹션 전환 ---
	else if (Name.Equals(TEXT("NextCombo")))
	{
		const FMidBossAttackPattern* PatternData = FindPatternData(CurrentPatternName);
		if (PatternData && PatternData->bUseSectionCombo)
		{
			CurrentChainIndex++;

			if (PatternData->MontageChain.IsValidIndex(CurrentChainIndex))
			{
				// 새 섹션의 PlayRate 적용 — SetNextSection은 배속을 변경하지 않으므로 명시적 갱신
				// 천사 장신구 슬로우 누적 곱
				const FPatternMontageData& CurrentEntry = PatternData->MontageChain[CurrentChainIndex];
				AnimInstance->Montage_SetPlayRate(CurrentMontage, CurrentEntry.PlayRate * CurrentAttackAnimRate);

				// 동적 섹션 전환: 현재 섹션 → 다음 섹션 설정
				// 같은 섹션 이름이 반복되어도 매 NextCombo마다 갱신하므로 정확한 다음 섹션 지정
				UAnimMontage* SectionMontage = PatternData->MontageChain[0].Montage;
				const int32 NextIndex = CurrentChainIndex + 1;

				if (PatternData->MontageChain.IsValidIndex(NextIndex))
				{
					const FPatternMontageData& NextEntry = PatternData->MontageChain[NextIndex];
					const bool bNextIsSameMontage = (NextEntry.Montage == nullptr || NextEntry.Montage == SectionMontage);

					if (bNextIsSameMontage && !CurrentEntry.SectionName.IsNone() && !NextEntry.SectionName.IsNone())
					{
						AnimInstance->Montage_SetNextSection(CurrentEntry.SectionName, NextEntry.SectionName, SectionMontage);
					}
					else
					{
						// 다른 몽타주 경계 — 현재 섹션에서 종료 → OnMontageEnded에서 체인 전환
						if (!CurrentEntry.SectionName.IsNone())
						{
							AnimInstance->Montage_SetNextSection(CurrentEntry.SectionName, NAME_None, SectionMontage);
						}
					}
				}
				else
				{
					// 마지막 섹션 — 기본 연결 끊기
					if (!CurrentEntry.SectionName.IsNone())
					{
						AnimInstance->Montage_SetNextSection(CurrentEntry.SectionName, NAME_None, SectionMontage);
					}
				}
			}

			UE_LOG(LogDesecration, Log,
				TEXT("T3_MidBoss: NextCombo — 섹션 진행 체인[%d] (패턴:'%s', 배속:%.1f)"),
				CurrentChainIndex, *CurrentPatternName.ToString(),
				PatternData->MontageChain.IsValidIndex(CurrentChainIndex) ? PatternData->MontageChain[CurrentChainIndex].PlayRate : -1.f);
		}
	}
	// --- 넓은 판정 ON/OFF (대쉬 내려찍기 등) ---
	else if (Name.Equals(TEXT("WideAttackStart")))
	{
		UE_LOG(LogDesecration, Log, TEXT("T3_MidBoss: WideAttackStart 노티파이 수신"));
		if (WeaponComponent) { WeaponComponent->SetWideCollisionEnabled(true); }
	}
	else if (Name.Equals(TEXT("WideAttackEnd")))
	{
		UE_LOG(LogDesecration, Log, TEXT("T3_MidBoss: WideAttackEnd 노티파이 수신"));
		if (WeaponComponent) { WeaponComponent->SetWideCollisionEnabled(false); }
	}
	// --- 팔 공격 판정 ON/OFF (맨손 타격 등) ---
	else if (Name.Equals(TEXT("BodyAttackStart")))
	{
		UE_LOG(LogDesecration, Log, TEXT("T3_MidBoss: BodyAttackStart 노티파이 수신"));
		if (WeaponComponent) { WeaponComponent->SetBodyAttackCollisionEnabled(true); }
	}
	else if (Name.Equals(TEXT("BodyAttackEnd")))
	{
		UE_LOG(LogDesecration, Log, TEXT("T3_MidBoss: BodyAttackEnd 노티파이 수신"));
		if (WeaponComponent) { WeaponComponent->SetBodyAttackCollisionEnabled(false); }
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

	// === 섹션 콤보 모드: 첫 진입 시 섹션 체인 설정 ===
	if (PatternData->bUseSectionCombo && CurrentChainIndex == 0)
	{
		// 시작 섹션 지정하여 재생 — 천사 장신구 슬로우 누적 곱
		const FName StartSection = MontageData.SectionName.IsNone() ? NAME_None : MontageData.SectionName;
		PlayAnimMontage(MontageData.Montage, MontageData.PlayRate * CurrentAttackAnimRate, StartSection);

		UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
		if (AnimInstance)
		{
			UAnimMontage* SectionMontage = MontageData.Montage;

			// 동적 섹션 전환: 첫 번째 전환만 설정 (나머지는 NextCombo에서 동적 설정)
			// 같은 섹션 이름이 반복될 때 SetNextSection 덮어쓰기 문제 방지
			if (PatternData->MontageChain.Num() > 1)
			{
				const FPatternMontageData& Next = PatternData->MontageChain[1];
				const bool bNextIsSameMontage = (Next.Montage == nullptr || Next.Montage == SectionMontage);
				if (bNextIsSameMontage && !MontageData.SectionName.IsNone() && !Next.SectionName.IsNone())
				{
					AnimInstance->Montage_SetNextSection(MontageData.SectionName, Next.SectionName, SectionMontage);
				}
				else
				{
					// 다른 몽타주 경계 — 현재 섹션에서 종료
					if (!MontageData.SectionName.IsNone())
					{
						AnimInstance->Montage_SetNextSection(MontageData.SectionName, NAME_None, SectionMontage);
					}
				}
			}
			else
			{
				// 섹션 1개뿐 — 기본 연결 끊기
				if (!MontageData.SectionName.IsNone())
				{
					AnimInstance->Montage_SetNextSection(MontageData.SectionName, NAME_None, SectionMontage);
				}
			}

			FOnMontageEnded EndDelegate;
			EndDelegate.BindUObject(this, &AT3MidBossMonster::OnMontageEnded);
			AnimInstance->Montage_SetEndDelegate(EndDelegate, SectionMontage);
		}

		UE_LOG(LogDesecration, Log,
			TEXT("T3_MidBoss: 섹션 콤보 시작 — 패턴:'%s' 섹션:'%s' (배속:%.1f, 섹션 수:%d)"),
			*CurrentPatternName.ToString(),
			StartSection.IsNone() ? TEXT("Default") : *StartSection.ToString(),
			MontageData.PlayRate, PatternData->MontageChain.Num());
		return;
	}

	// === 기존 체인 모드 (또는 섹션 콤보 후속의 다른 몽타주) === 천사 장신구 슬로우 누적 곱
	const FName StartSection = MontageData.SectionName.IsNone() ? NAME_None : MontageData.SectionName;
	PlayAnimMontage(MontageData.Montage, MontageData.PlayRate * CurrentAttackAnimRate, StartSection);

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
		TEXT("T3_MidBoss: 몽타주 재생 — 패턴:'%s' 체인[%d] (배속:%.1f, 데미지:%.0f, 섹션:'%s')"),
		*CurrentPatternName.ToString(), CurrentChainIndex,
		MontageData.PlayRate, MontageData.Damage,
		StartSection.IsNone() ? TEXT("None") : *StartSection.ToString());
}

void AT3MidBossMonster::OnChainBlendingOut(UAnimMontage* Montage, bool bInterrupted)
{
	if (!IsExecutingPattern())
	{
		return;
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
			TEXT("T3_MidBoss: 체인 블렌드아웃 인터럽트 — 패턴:'%s' 체인[%d]"),
			*CurrentPatternName.ToString(), CurrentChainIndex);
		ResetPatternState();
		return;
	}

	// 블렌드아웃 시작 시점에 바로 다음 체인 진행 → Blend Out + Blend In 겹침
	AdvanceChainOrComplete();
}

void AT3MidBossMonster::OnMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	if (!IsExecutingPattern())
	{
		return;
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
			TEXT("T3_MidBoss: 몽타주 인터럽트 — 패턴:'%s' 체인[%d]"),
			*CurrentPatternName.ToString(), CurrentChainIndex);
		ResetPatternState();
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
