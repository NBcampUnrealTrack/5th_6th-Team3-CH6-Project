// T3MidBossMonster_Combat.cpp — 데미지 처리, 히트 리액션, 스턴, 무기 히트

#include "Monster/T3MidBossMonster.h"
#include "Monster/T3BossWeaponComponent.h"
#include "Desecration.h"
#include "Engine/DamageEvents.h"
#include "Kismet/GameplayStatics.h"

// ============================================================
// 데미지 처리
// ============================================================

float AT3MidBossMonster::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent,
	AController* EventInstigator, AActor* DamageCauser)
{
	if (IsDead())
	{
		return 0.f;
	}

	const float ActualDamage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);

	float StunAmount = 0.f;
	if (DamageEvent.GetTypeID() == FT3DamageEvent::ClassID)
	{
		const FT3DamageEvent* T3Event = static_cast<const FT3DamageEvent*>(&DamageEvent);
		if (T3Event)
		{
			StunAmount = T3Event->StunAmount;
		}
	}

	ApplyDamageToMidBoss(ActualDamage, StunAmount, DamageCauser);
	return ActualDamage;
}

void AT3MidBossMonster::ApplyDamageToMidBoss(float DamageAmount, float StunAmount, AActor* DamageCauser)
{
	if (IsDead() || DamageAmount <= 0.f)
	{
		return;
	}

	MidBossStats.CurrentHP -= DamageAmount;
	OnMidBossDamaged.Broadcast();

	// 피격 사운드 재생
	if (HitSound)
	{
		UGameplayStatics::PlaySoundAtLocation(
			this, HitSound, GetActorLocation(),
			SoundVolume * HitVolumeMultiplier);
	}

	UE_LOG(LogDesecration, Log, TEXT("T3_MidBoss: %s 피격 (데미지: %.0f, 남은HP: %.0f, 스턴게이지: %.0f/%.0f)"),
		*BossName, DamageAmount, MidBossStats.CurrentHP, MidBossStats.CurrentStunGauge, MidBossStats.StunThreshold);

	// 카메라 쉐이크 — 상태 무관하게 항상 재생 (피격 피드백)
	if (HitCameraShakeClass)
	{
		UGameplayStatics::PlayWorldCameraShake(
			this, HitCameraShakeClass, GetActorLocation(),
			0.f, HitShakeOuterRadius, HitShakeFalloff);

		UE_LOG(LogDesecration, Log, TEXT("T3_MidBoss: 카메라 쉐이크 재생 (Class: %s, Radius: %.0f)"),
			*HitCameraShakeClass->GetName(), HitShakeOuterRadius);
	}
	else
	{
		UE_LOG(LogDesecration, Warning, TEXT("T3_MidBoss: HitCameraShakeClass가 할당되지 않음!"));
	}

	// 히트 리액션 — 슈퍼아머 + 비기절 + 비공격 + 생존 시에만 재생
	if (HasSuperArmor() && !IsStunned() && !IsExecutingPattern() && MidBossStats.CurrentHP > 0.f)
	{
		PlayAdditiveHitReaction(DamageCauser);
	}

	if (!IsStunned())
	{
		MidBossStats.CurrentStunGauge += StunAmount;
	}

	if (MidBossStats.CurrentHP <= 0.f)
	{
		AddStateTag(TAG_Boss_State_Dead);
		MidBossStats.CurrentHP = 0.f;
		CancelCurrentPattern();
		OnMidBossDeath.Broadcast();

		// StateTree 이벤트 전송 — 즉시 Dead 상태로 전환 (State 태그를 이벤트로 겸용)
		if (StateTreeComponent)
		{
			StateTreeComponent->SendStateTreeEvent(TAG_Boss_State_Dead);
		}

		BeginDeathSequence();

		UE_LOG(LogDesecration, Log, TEXT("T3_MidBoss: %s 사망 (StateTree 이벤트 전송)"), *BossName);
		return;
	}

	if (MidBossStats.CurrentStunGauge >= MidBossStats.StunThreshold && !IsStunned())
	{
		ApplyStun();
		return;
	}

	OnMidBossHit.Broadcast();
}

// ============================================================
// 히트 리액션 (방향별)
// ============================================================

void AT3MidBossMonster::PlayAdditiveHitReaction(AActor* DamageCauser)
{
	UAnimMontage* Montage = GetDirectionalHitReactMontage(DamageCauser);
	if (Montage)
	{
		PlayAnimMontage(Montage);
	}
}

UAnimMontage* AT3MidBossMonster::GetDirectionalHitReactMontage(AActor* DamageCauser) const
{
	if (!DamageCauser)
	{
		return HitReactMontage_Default;
	}

	// 보스 기준 공격자 방향 계산
	const FVector ToAttacker = (DamageCauser->GetActorLocation() - GetActorLocation()).GetSafeNormal2D();
	const FVector Forward = GetActorForwardVector().GetSafeNormal2D();

	const float Dot = FVector::DotProduct(Forward, ToAttacker);
	const float Cross = FVector::CrossProduct(Forward, ToAttacker).Z;

	// 전후좌우 판별 (45도 기준)
	UAnimMontage* Selected = nullptr;

	if (Dot > 0.707f)
	{
		// 전방에서 맞음 (±45도)
		Selected = HitReactMontage_F;
	}
	else if (Dot < -0.707f)
	{
		// 후방에서 맞음 (±45도)
		Selected = HitReactMontage_B;
	}
	else if (Cross > 0.f)
	{
		// 우측에서 맞음
		Selected = HitReactMontage_R;
	}
	else
	{
		// 좌측에서 맞음
		Selected = HitReactMontage_L;
	}

	// 방향별 몽타주가 없으면 폴백
	return Selected ? Selected : HitReactMontage_Default.Get();
}

// ============================================================
// 스턴 / 회복
// ============================================================

void AT3MidBossMonster::ApplyStun()
{
	AddStateTag(TAG_Boss_State_Stunned);
	MidBossStats.CurrentStunGauge = 0.f;
	CancelCurrentPattern();

	// 스턴 몽타주 재생
	if (StunMontage)
	{
		PlayAnimMontage(StunMontage);
	}

	// 스턴 사운드 재생
	if (StunSound)
	{
		UGameplayStatics::PlaySoundAtLocation(
			this, StunSound, GetActorLocation(),
			SoundVolume * StunVolumeMultiplier);
	}

	OnMidBossStun.Broadcast();

	// StateTree 이벤트 전송 — 즉시 Stunned 상태로 전환 (State 태그를 이벤트로 겸용)
	if (StateTreeComponent)
	{
		StateTreeComponent->SendStateTreeEvent(TAG_Boss_State_Stunned);
	}

	// 스턴 자동 해제 타이머 시작
	GetWorldTimerManager().ClearTimer(StunTimerHandle);
	GetWorldTimerManager().SetTimer(
		StunTimerHandle, this, &AT3MidBossMonster::RecoverFromStun,
		StunDuration, false);

	UE_LOG(LogDesecration, Log,
		TEXT("T3_MidBoss: %s 스턴 상태 진입 (%.1f초 후 자동 해제, StateTree 이벤트 전송)"), *BossName, StunDuration);
}

void AT3MidBossMonster::RecoverFromStun()
{
	GetWorldTimerManager().ClearTimer(StunTimerHandle);
	RemoveStateTag(TAG_Boss_State_Stunned);
	MidBossStats.CurrentStunGauge = 0.f;

	// StateTree 이벤트 전송 — Stunned 상태 종료, Approach로 복귀
	if (StateTreeComponent)
	{
		StateTreeComponent->SendStateTreeEvent(TAG_Boss_Event_StunRecovered);
	}

	UE_LOG(LogDesecration, Log, TEXT("T3_MidBoss: %s 스턴 해제 (StateTree 이벤트 전송)"), *BossName);
}

// ============================================================
// 무기 히트 → 데미지 적용
// ============================================================

void AT3MidBossMonster::OnWeaponHit(AActor* HitActor)
{
	if (!HitActor || IsDead())
	{
		return;
	}

	// 현재 패턴의 데미지 데이터 조회
	const FMidBossAttackPattern* PatternData = FindPatternData(CurrentPatternName);
	if (!PatternData || !PatternData->MontageChain.IsValidIndex(CurrentChainIndex))
	{
		return;
	}

	const FPatternMontageData& MontageData = PatternData->MontageChain[CurrentChainIndex];

	// FT3DamageEvent 생성 — HitIntensity + DamageType 전달
	TSubclassOf<UDamageType> DamageTypeClass = MontageData.DamageTypeClass;
	if (!DamageTypeClass)
	{
		DamageTypeClass = UT3DamageType_Base::StaticClass();
	}

	FT3DamageEvent DamageEvent(DamageTypeClass);
	DamageEvent.HitIntensity = MontageData.HitIntensity;
	DamageEvent.HitDamageMultiplier = 1.0f;

	// 플레이어의 TakeDamage 직접 호출
	// → AT3CharacterBase::TakeDamage → CombatComponent::ExecuteHitLogic
	// → 블록/패링/회피 판정 → HP 차감
	HitActor->TakeDamage(
		MontageData.Damage,
		DamageEvent,
		GetController(),    // 보스의 AIController
		this                // DamageCauser = 보스
	);

	UE_LOG(LogDesecration, Log, TEXT("T3_MidBoss: %s에게 TakeDamage (데미지:%.0f, 강도:%s, 패턴:'%s'[%d])"),
		*HitActor->GetName(), MontageData.Damage,
		*UEnum::GetValueAsString(MontageData.HitIntensity),
		*CurrentPatternName.ToString(), CurrentChainIndex);
}
