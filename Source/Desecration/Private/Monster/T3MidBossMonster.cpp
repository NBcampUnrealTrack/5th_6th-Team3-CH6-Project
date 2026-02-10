#include "Monster/T3MidBossMonster.h"
#include "Desecration.h"
#include "Engine/DamageEvents.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "NativeGameplayTags.h"

// Gameplay Tag 네이티브 정의 — 상태 태그
UE_DEFINE_GAMEPLAY_TAG(TAG_Boss_State_Dead, "Boss.State.Dead");
UE_DEFINE_GAMEPLAY_TAG(TAG_Boss_State_Stunned, "Boss.State.Stunned");
UE_DEFINE_GAMEPLAY_TAG(TAG_Boss_State_ExecutingPattern, "Boss.State.ExecutingPattern");
UE_DEFINE_GAMEPLAY_TAG(TAG_Boss_State_SuperArmor, "Boss.State.SuperArmor");

// Gameplay Tag 네이티브 정의 — StateTree 이벤트 태그
UE_DEFINE_GAMEPLAY_TAG(TAG_Boss_Event_Dead, "Boss.Event.Dead");
UE_DEFINE_GAMEPLAY_TAG(TAG_Boss_Event_Stunned, "Boss.Event.Stunned");
UE_DEFINE_GAMEPLAY_TAG(TAG_Boss_Event_StunRecovered, "Boss.Event.StunRecovered");
UE_DEFINE_GAMEPLAY_TAG(TAG_Boss_Event_ActionCountDepleted, "Boss.Event.ActionCountDepleted");

AT3MidBossMonster::AT3MidBossMonster()
{
	PrimaryActorTick.bCanEverTick = true;

	// StateTree 컴포넌트 (standalone 스키마 — AI Controller 없이 동작)
	StateTreeComponent = CreateDefaultSubobject<UStateTreeComponent>(TEXT("StateTreeComponent"));
}

// ============================================================
// 상태 태그 헬퍼
// ============================================================

void AT3MidBossMonster::AddStateTag(FGameplayTag Tag)
{
	ActiveGameplayTags.AddTag(Tag);
}

void AT3MidBossMonster::RemoveStateTag(FGameplayTag Tag)
{
	ActiveGameplayTags.RemoveTag(Tag);
}

bool AT3MidBossMonster::HasStateTag(FGameplayTag Tag) const
{
	return ActiveGameplayTags.HasTag(Tag);
}

bool AT3MidBossMonster::IsDead() const
{
	return ActiveGameplayTags.HasTag(TAG_Boss_State_Dead);
}

bool AT3MidBossMonster::IsStunned() const
{
	return ActiveGameplayTags.HasTag(TAG_Boss_State_Stunned);
}

bool AT3MidBossMonster::IsExecutingPattern() const
{
	return ActiveGameplayTags.HasTag(TAG_Boss_State_ExecutingPattern);
}

bool AT3MidBossMonster::HasSuperArmor() const
{
	return ActiveGameplayTags.HasTag(TAG_Boss_State_SuperArmor);
}

// ============================================================
// BeginPlay / Tick
// ============================================================

void AT3MidBossMonster::BeginPlay()
{
	Super::BeginPlay();

	MidBossStats.CurrentHP = MidBossStats.MaxHP;
	MidBossStats.CurrentStunGauge = 0.f;

	// 초기 상태 태그 설정 (기존 bSuperArmor = true 대체)
	AddStateTag(TAG_Boss_State_SuperArmor);

	// 노티파이 보정기 생성 및 초기화
	NotifyModifier = NewObject<UMidBossNotifyModifier>(this);
	NotifyModifier->Initialize(ModifierDataAsset);

	OnMidBossSpawned.Broadcast();

	UE_LOG(LogDesecration, Log, TEXT("T3_MidBoss: %s 스폰 완료 (HP: %.0f, Stage: %d, 등록 패턴: %d개, Modifier: %s)"),
		*BossName, MidBossStats.MaxHP, BossStage, AttackPatterns.Num(),
		ModifierDataAsset ? TEXT("O") : TEXT("X"));
}

void AT3MidBossMonster::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// MoveToTarget 보간 처리
	if (bIsMovingToTarget)
	{
		MoveToTargetElapsed += DeltaTime;

		if (MoveToTargetElapsed >= MoveToTargetDuration)
		{
			bIsMovingToTarget = false;
		}
		else
		{
			const FVector Delta = MoveToTargetDirection * MoveToTargetSpeed * DeltaTime;
			AddActorWorldOffset(Delta, true);
		}
	}
}

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

	SetAttackCollisionEnabled(false);
	bIsMovingToTarget = false;
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
	if (!IsExecutingPattern())
	{
		return;
	}

	const FString Name = NotifyName.ToString();

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
		SetAttackCollisionEnabled(true);
	}
	else if (Name.Equals(TEXT("AttackEnd")))
	{
		SetAttackCollisionEnabled(false);
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
// 타겟 방향 회전
// ============================================================

void AT3MidBossMonster::FaceTarget(float InterpSpeed)
{
	if (!CombatTarget)
	{
		return;
	}

	const FVector Direction = (CombatTarget->GetActorLocation() - GetActorLocation()).GetSafeNormal2D();
	if (!Direction.IsNearlyZero())
	{
		const FRotator TargetRotation = Direction.Rotation();
		SetActorRotation(FMath::RInterpTo(
			GetActorRotation(), TargetRotation,
			GetWorld()->GetDeltaSeconds(), InterpSpeed));
	}
}

// ============================================================
// 유틸리티 함수
// ============================================================

void AT3MidBossMonster::MoveToTarget(float Duration, float Distance)
{
	if (!CombatTarget || Duration <= 0.f)
	{
		return;
	}

	const FVector ToTarget = CombatTarget->GetActorLocation() - GetActorLocation();
	const FVector Direction = ToTarget.GetSafeNormal2D();

	bIsMovingToTarget = true;
	MoveToTargetElapsed = 0.f;
	MoveToTargetDuration = Duration;
	MoveToTargetDirection = Direction;
	MoveToTargetSpeed = Distance / Duration;

	UE_LOG(LogDesecration, Verbose,
		TEXT("T3_MidBoss: MoveToTarget 시작 (Duration:%.2f, Distance:%.0f)"), Duration, Distance);
}

void AT3MidBossMonster::SetAttackCollisionEnabled(bool bEnable)
{
	if (WeaponCollisionComponent)
	{
		WeaponCollisionComponent->SetCollisionEnabled(
			bEnable ? ECollisionEnabled::QueryOnly : ECollisionEnabled::NoCollision);

		UE_LOG(LogDesecration, Verbose,
			TEXT("T3_MidBoss: 무기 콜리전 %s"), bEnable ? TEXT("ON") : TEXT("OFF"));
	}
}

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

// ============================================================
// 내부 함수
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

	SetAttackCollisionEnabled(false);
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

void AT3MidBossMonster::RegisterCooldown(FName PatternName, float CooldownSeconds)
{
	const double ExpireTime = GetWorld()->GetTimeSeconds() + static_cast<double>(CooldownSeconds);
	PatternCooldownExpireMap.Add(PatternName, ExpireTime);
}

void AT3MidBossMonster::ResetPatternState()
{
	CurrentPatternName = NAME_None;
	CurrentChainIndex = 0;
	RemoveStateTag(TAG_Boss_State_ExecutingPattern);
}

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
			switch (T3Event->HitIntensity)
			{
			case EHitIntensity::Light:
				StunAmount = 10.f;
				break;
			case EHitIntensity::Medium:
				StunAmount = 25.f;
				break;
			case EHitIntensity::Heavy:
				StunAmount = 50.f;
				break;
			}
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

	UE_LOG(LogDesecration, Log, TEXT("T3_MidBoss: %s 피격 (데미지: %.0f, 남은HP: %.0f, 스턴게이지: %.0f/%.0f)"),
		*BossName, DamageAmount, MidBossStats.CurrentHP, MidBossStats.CurrentStunGauge, MidBossStats.StunThreshold);

	if (HasSuperArmor())
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

		// StateTree 이벤트 전송 — 즉시 Dead 상태로 전환
		if (StateTreeComponent)
		{
			StateTreeComponent->SendStateTreeEvent(TAG_Boss_Event_Dead);
		}

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

void AT3MidBossMonster::ApplyStun()
{
	AddStateTag(TAG_Boss_State_Stunned);
	MidBossStats.CurrentStunGauge = 0.f;
	CancelCurrentPattern();
	OnMidBossStun.Broadcast();

	// StateTree 이벤트 전송 — 즉시 Stunned 상태로 전환
	if (StateTreeComponent)
	{
		StateTreeComponent->SendStateTreeEvent(TAG_Boss_Event_Stunned);
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

	// C++ 델리게이트 → StateTree Task에 완료 알림 (레거시, 제거 예정)
	OnStunRecoveredNative.Broadcast();

	// StateTree 이벤트 전송 — Stunned 상태 종료, Approach로 복귀
	if (StateTreeComponent)
	{
		StateTreeComponent->SendStateTreeEvent(TAG_Boss_Event_StunRecovered);
	}

	UE_LOG(LogDesecration, Log, TEXT("T3_MidBoss: %s 스턴 해제 (StateTree 이벤트 전송)"), *BossName);
}
