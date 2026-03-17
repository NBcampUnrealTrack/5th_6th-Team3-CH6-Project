// T3MidBossMonster_Combat.cpp — 데미지 처리, 히트 리액션, 스턴, 무기 히트

#include "Monster/T3MidBossMonster.h"
#include "Monster/T3BossWeaponComponent.h"
#include "Desecration.h"
#include "Engine/DamageEvents.h"
#include "Kismet/GameplayStatics.h"
#include "Monster/T3BossProjectile.h"
#include "Kismet/KismetSystemLibrary.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"
#include "Components/CapsuleComponent.h"
#include "DrawDebugHelpers.h"
#include "Player/T3DamageTypes.h"

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

	// 패링 윈도우 활성 시 — 데미지 무효화 + 반격
	if (IsParryWindowActive())
	{
		// ExecuteParryCounter가 먼저 — bParrySucceeded 설정 후 CloseParryWindow
		ExecuteParryCounter(DamageCauser);
		CloseParryWindow();

		UE_LOG(LogDesecration, Log, TEXT("T3_MidBoss: 패링 성공! (공격자: %s)"),
			DamageCauser ? *DamageCauser->GetName() : TEXT("nullptr"));
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
	
	UE_LOG(LogItem, Log, TEXT("중간보스의 남은 체력 : %.1f"), MidBossStats.CurrentHP);
	
	// 피격 사운드 재생 (최소 간격 제한 — 연속 히트 시 씹힘 방지)
	if (HitSound)
	{
		const double CurrentTime = GetWorld()->GetTimeSeconds();
		if (CurrentTime - LastHitSoundTime >= HitSoundMinInterval)
		{
			UGameplayStatics::PlaySoundAtLocation(
				this, HitSound, GetActorLocation(),
				SoundVolume * HitVolumeMultiplier);
			LastHitSoundTime = CurrentTime;
		}
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
// 투사체 스폰 (검기)
// ============================================================

void AT3MidBossMonster::SpawnBossProjectile()
{
	if (!ProjectileClass)
	{
		UE_LOG(LogDesecration, Warning, TEXT("T3_MidBoss: ProjectileClass가 설정되지 않음"));
		return;
	}

	// 스폰 위치: 보스 전방 오프셋
	const FVector SpawnLocation = GetActorLocation() + GetActorForwardVector() * ProjectileSpawnOffset;
	const FRotator SpawnRotation = GetActorRotation();

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;
	SpawnParams.Instigator = this;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	AT3BossProjectile* Projectile = GetWorld()->SpawnActor<AT3BossProjectile>(
		ProjectileClass, SpawnLocation, SpawnRotation, SpawnParams);

	if (!Projectile)
	{
		UE_LOG(LogDesecration, Warning, TEXT("T3_MidBoss: 투사체 스폰 실패"));
		return;
	}

	// 현재 서브히트 데미지 데이터 조회
	float ProjectileDamage = 30.f;
	EHitIntensity Intensity = EHitIntensity::Light;
	TSubclassOf<UT3DamageType_Base> DmgType = nullptr;
	GetCurrentHitData(ProjectileDamage, Intensity, DmgType);

	Projectile->InitializeProjectile(ProjectileDamage, ProjectileSpeed, Intensity, DmgType);

	UE_LOG(LogDesecration, Log, TEXT("T3_MidBoss: 투사체 스폰 (데미지:%.0f, 속도:%.0f)"),
		ProjectileDamage, ProjectileSpeed);
}

// ============================================================
// AoE 장판기 데미지
// ============================================================

void AT3MidBossMonster::ExecuteAoEDamage(float Radius, float DamageAmount, EHitIntensity Intensity)
{
	// AoE 중심 = 보스 발밑 (캡슐 하단)
	const FVector AoECenter = GetActorLocation() - FVector(0.0, 0.0, static_cast<double>(GetCapsuleComponent()->GetScaledCapsuleHalfHeight()));

	// 범위 내 Pawn 오버랩 (SphereOverlapActors — Pawn 오브젝트 타입)
	TArray<AActor*> OverlappedActors;
	TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
	ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_Pawn));

	UKismetSystemLibrary::SphereOverlapActors(
		this, AoECenter, Radius, ObjectTypes, nullptr, TArray<AActor*>{this}, OverlappedActors);

	// 현재 패턴의 DamageType 조회
	TSubclassOf<UDamageType> DamageTypeClass = UT3DamageType_Base::StaticClass();
	const FMidBossAttackPattern* PatternData = FindPatternData(CurrentPatternName);
	if (PatternData && PatternData->MontageChain.IsValidIndex(CurrentChainIndex))
	{
		if (PatternData->MontageChain[CurrentChainIndex].DamageTypeClass)
		{
			DamageTypeClass = PatternData->MontageChain[CurrentChainIndex].DamageTypeClass;
		}
	}

	for (AActor* HitActor : OverlappedActors)
	{
		if (!HitActor)
		{
			continue;
		}

		FT3DamageEvent DamageEvent(DamageTypeClass);
		DamageEvent.HitIntensity = Intensity;
		DamageEvent.HitDamageMultiplier = 1.0f;

		HitActor->TakeDamage(DamageAmount, DamageEvent, GetController(), this);

		UE_LOG(LogDesecration, Log, TEXT("T3_MidBoss: AoE 데미지 — %s에게 %.0f (반경:%.0f)"),
			*HitActor->GetName(), DamageAmount, Radius);
	}

	// Niagara 이펙트 스폰 — AoEEffectScale로 크기 조절
	if (AoEEffect)
	{
		const FVector ScaleVec = FVector(AoEEffectScale);
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			this, AoEEffect, AoECenter, GetActorRotation(), ScaleVec);
	}
	else
	{
		// Niagara 없을 때 임시 범위 표시
		DrawDebugSphere(GetWorld(), AoECenter, Radius, 24,
			FColor::Red, false, 1.5f, 0, 3.f);
	}

	// AoE 사운드
	if (AoESound)
	{
		UGameplayStatics::PlaySoundAtLocation(
			this, AoESound, AoECenter, SoundVolume * AoEVolumeMultiplier);
	}

	// 카메라 쉐이크
	if (AoECameraShakeClass)
	{
		UGameplayStatics::PlayWorldCameraShake(
			this, AoECameraShakeClass, AoECenter, 0.f, AoEShakeOuterRadius);
	}
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

	// 현재 서브히트 데미지 데이터 조회
	float HitDamage = 20.f;
	EHitIntensity Intensity = EHitIntensity::Light;
	TSubclassOf<UT3DamageType_Base> DmgType = nullptr;
	GetCurrentHitData(HitDamage, Intensity, DmgType);

	TSubclassOf<UDamageType> FinalDamageType = DmgType ? DmgType.Get() : UT3DamageType_Base::StaticClass();

	FT3DamageEvent DamageEvent(FinalDamageType);
	DamageEvent.HitIntensity = Intensity;
	DamageEvent.HitDamageMultiplier = 1.0f;

	// 플레이어의 TakeDamage 직접 호출
	// → AT3CharacterBase::TakeDamage → CombatComponent::ExecuteHitLogic
	// → 블록/패링/회피 판정 → HP 차감
	HitActor->TakeDamage(
		HitDamage,
		DamageEvent,
		GetController(),    // 보스의 AIController
		this                // DamageCauser = 보스
	);

	UE_LOG(LogDesecration, Log, TEXT("T3_MidBoss: %s에게 TakeDamage (데미지:%.0f, 강도:%s, 패턴:'%s'[%d])"),
		*HitActor->GetName(), HitDamage,
		*UEnum::GetValueAsString(Intensity),
		*CurrentPatternName.ToString(), CurrentChainIndex);
}

// ============================================================
// 패링 카운터
// ============================================================

bool AT3MidBossMonster::IsParryWindowActive() const
{
	return HasStateTag(TAG_Boss_State_ParryWindow);
}

void AT3MidBossMonster::OpenParryWindow()
{
	AddStateTag(TAG_Boss_State_ParryWindow);

	// 타임아웃 타이머 — 윈도우 지속시간 후 자동 닫힘
	GetWorldTimerManager().ClearTimer(ParryWindowTimerHandle);
	GetWorldTimerManager().SetTimer(
		ParryWindowTimerHandle, this, &AT3MidBossMonster::CloseParryWindow,
		ParryWindowDuration, false);

	UE_LOG(LogDesecration, Log, TEXT("T3_MidBoss: 패링 윈도우 오픈 (%.1f초)"), ParryWindowDuration);
}

void AT3MidBossMonster::CloseParryWindow()
{
	GetWorldTimerManager().ClearTimer(ParryWindowTimerHandle);
	RemoveStateTag(TAG_Boss_State_ParryWindow);

	// 반격 미발동 (타임아웃) → 기 모으다 끝, 패턴 종료
	if (!bParrySucceeded)
	{
		UE_LOG(LogDesecration, Log, TEXT("T3_MidBoss: 카운터 타임아웃 — 패턴 캔슬 (공격 안 함)"));
		CancelCurrentPattern();
		return;
	}

	UE_LOG(LogDesecration, Log, TEXT("T3_MidBoss: 패링 윈도우 닫힘 (반격 발동됨)"));
}

void AT3MidBossMonster::ExecuteParryCounter(AActor* ParriedAttacker)
{
	// 패링 성공 플래그 — SpawnProjectile에서 검기 스킵
	bParrySucceeded = true;

	// Slow 상태 해제 → 정상 속도로 몽타주 이어서 재생 (검 휘두르기로 이어짐)
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance)
	{
		const FMidBossAttackPattern* PatternData = FindPatternData(CurrentPatternName);
		if (PatternData && PatternData->MontageChain.IsValidIndex(CurrentChainIndex))
		{
			AnimInstance->Montage_SetPlayRate(
				PatternData->MontageChain[CurrentChainIndex].Montage, 1.0f);
		}
	}

	// 패링 사운드
	if (ParrySound)
	{
		UGameplayStatics::PlaySoundAtLocation(
			this, ParrySound, GetActorLocation(), SoundVolume * ParryVolumeMultiplier);
	}

	UE_LOG(LogDesecration, Log, TEXT("T3_MidBoss: 패링 반격 — Slow 해제, 몽타주 이어서 재생"));
}

float AT3MidBossMonster::GetHPPercent() const
{
	return MidBossStats.CurrentHP / MidBossStats.MaxHP;
}

ET3MonsterType AT3MidBossMonster::GetMonsterType() const
{
	return MonsterType;
}

void AT3MidBossMonster::ApplyBonusDamage(float BonusDamage)
{
	FT3DamageEvent DamageEvent(UT3DamageType_Base::StaticClass());
	
	TakeDamage(BonusDamage, DamageEvent, nullptr, nullptr);
}