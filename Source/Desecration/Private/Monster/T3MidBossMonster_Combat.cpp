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
#include "GameFramework/CharacterMovementComponent.h"
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

	// 데미지 타입 체크 — Undodgable이면 모든 방어 우회
	const UClass* DmgTypeClass = DamageEvent.DamageTypeClass;
	const bool bUndodgable = DmgTypeClass && DmgTypeClass->IsChildOf(UT3DamageType_Undodgable::StaticClass());
	const bool bUnparryable = DmgTypeClass && DmgTypeClass->IsChildOf(UT3DamageType_Unparryable::StaticClass());
	const bool bUnblockable = DmgTypeClass && DmgTypeClass->IsChildOf(UT3DamageType_Unblockable::StaticClass());

	float IncomingDamage = DamageAmount;

	if (!bUndodgable)
	{
		// 회피 i-frame — 데미지 0
		if (IsInvulnerable())
		{
			UE_LOG(LogDesecration, Log, TEXT("T3_MidBoss: i-frame 회피 성공 (피해 무효, 공격자: %s)"),
				DamageCauser ? *DamageCauser->GetName() : TEXT("nullptr"));
			return 0.f;
		}

		// 막기 — Unparryable / Unblockable이면 가드 뚫림 (풀데미지), 아니면 정면/후방 배율 적용
		if (IsBlocking())
		{
			// 막기 시퀀스 동안 받은 모든 피격 카운트 (막든/뚫리든 무관) — STT_Block이 BlockReaction 임계치 폴링용
			++BlockHitsCount;

			if (!bUnparryable && !bUnblockable)
			{
				const bool bFront = IsHitFromFront(DamageCauser);
				const float Mult = bFront ? BlockDamageMultiplier_Front : BlockDamageMultiplier_Back;
				IncomingDamage = DamageAmount * Mult;

				// 경직치 누적 — 데스몬드처럼 StaggerOnBlockedHit>0인 보스만 동작
				// "가드는 HP 대신 게이지를 깎는다"는 트레이드 — TakeDamage StunAmount와 별도 추가 누적
				// 막아낸 비용 의미라 가드 뚫림 분기에서는 누적 X (어차피 풀데미지 받음)
				AddStunGauge(StaggerOnBlockedHit);

				UE_LOG(LogDesecration, Log,
					TEXT("T3_MidBoss: 막기 성공 (방향:%s, 배율:%.2f, 원본:%.0f → %.0f, 누적:%d회)"),
					bFront ? TEXT("정면") : TEXT("후방"), Mult, DamageAmount, IncomingDamage, BlockHitsCount);

				// BlockReaction 이벤트 송신은 FT3STT_Block::Tick이 ResolvedHitThreshold 도달 시 책임짐.
				// (Combat 측 매 hit FRand 굴림 모델 제거 — 진입 시 1회 RandRange 추첨 모델로 단일화)
				// BlockReactionChance 변수는 BP 데이터 보존 차원에서 헤더에 유지 (정리 라운드에서 제거 예정)
			}
			else
			{
				UE_LOG(LogDesecration, Log,
					TEXT("T3_MidBoss: 가드 뚫림 (Unparry:%d Unblock:%d, 풀데미지:%.0f, 누적:%d회)"),
					bUnparryable ? 1 : 0, bUnblockable ? 1 : 0, DamageAmount, BlockHitsCount);
			}
		}
	}

	const float ActualDamage = Super::TakeDamage(IncomingDamage, DamageEvent, EventInstigator, DamageCauser);

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
	UE_LOG(LogDesecration, Log, TEXT("T3_MidBoss: %s 피격 (데미지: %.0f, 남은HP: %.0f, 스턴게이지: %.0f/%.0f)"),
		*BossDisplayName.ToString(), DamageAmount, MidBossStats.CurrentHP, MidBossStats.CurrentStunGauge, MidBossStats.StunThreshold);

	PlayHitFeedback(GetActorLocation(), DamageCauser);

	if (!IsStunned())
	{
		MidBossStats.CurrentStunGauge += StunAmount;
	}

	if (MidBossStats.CurrentHP <= 0.f)
	{
		EnterDeathState();
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
		// 헬퍼 통과 — 슬로우 존 안에서 맞을 때 히트리액션도 실시간 슬로우 적용
		PlayMoveMontageWithSlow(Montage, 1.f);
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

	// 리액션 트리거 클리어 — Stun 3초 텀 후 "회피/막기 직후 반격"은 시맨틱 깨짐
	// (Roll/Block 정상 종료 직후 Stun이 끼면, 회복 후 첫 공격이 리액션 가속으로 나오는 부조리 방지)
	PendingReactionSource = EBossReactionSource::None;

	// 스턴 몽타주 재생 — 헬퍼 통과로 슬로우 존 실시간 갱신 적용
	// (막기 진행 중이었다면 ST의 Block ExitState → StopBlockSequence가 활성 막기 몽타주만 외과적으로 정지하므로 충돌 없음)
	if (StunMontage)
	{
		PlayMoveMontageWithSlow(StunMontage, 1.f);
	}

	// 스턴 사운드 재생
	PlayBossSoundAt(StunSound, GetActorLocation(), StunVolumeMultiplier);

	OnMidBossStun.Broadcast();

	// StateTree 이벤트 전송 — 즉시 Stunned 상태로 전환 (State 태그를 이벤트로 겸용)
	SendStateTreeStateEvent(TAG_Boss_State_Stunned);

	// 스턴 자동 해제 타이머 시작
	GetWorldTimerManager().ClearTimer(StunTimerHandle);
	GetWorldTimerManager().SetTimer(
		StunTimerHandle, this, &AT3MidBossMonster::RecoverFromStun,
		StunDuration, false);

	UE_LOG(LogDesecration, Log,
		TEXT("T3_MidBoss: %s 스턴 상태 진입 (%.1f초 후 자동 해제, StateTree 이벤트 전송)"), *BossDisplayName.ToString(), StunDuration);
}

void AT3MidBossMonster::RecoverFromStun()
{
	GetWorldTimerManager().ClearTimer(StunTimerHandle);
	RemoveStateTag(TAG_Boss_State_Stunned);
	MidBossStats.CurrentStunGauge = 0.f;

	// StateTree 이벤트 전송 — Stunned 상태 종료, Approach로 복귀
	SendStateTreeStateEvent(TAG_Boss_Event_StunRecovered);

	UE_LOG(LogDesecration, Log, TEXT("T3_MidBoss: %s 스턴 해제 (StateTree 이벤트 전송)"), *BossDisplayName.ToString());
}

// ============================================================
// 경직치 게이지 누적 헬퍼 — Roll/Block 경로에서 호출
// (TakeDamage 경로의 StunAmount 누적은 ApplyDamageToMidBoss에서 그대로 처리)
// ============================================================

void AT3MidBossMonster::AddStunGauge(float Amount)
{
	if (Amount <= 0.f || IsStunned() || IsDead())
	{
		return;
	}

	MidBossStats.CurrentStunGauge += Amount;
	LastStaggerEventTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;

	UE_LOG(LogDesecration, Log,
		TEXT("T3_MidBoss: 경직치 누적 (+%.1f → %.1f/%.1f)"),
		Amount, MidBossStats.CurrentStunGauge, MidBossStats.StunThreshold);

	if (MidBossStats.CurrentStunGauge >= MidBossStats.StunThreshold)
	{
		ApplyStun();
	}
}

// ============================================================
// 막기 시퀀스 (In → Loop → Out)
// 단계 전환은 BlendingOut 시점 PlayAnimMontage 자연 크로스페이드로 처리
// (체인 패턴의 OnChainBlendingOut 매커니즘과 동일 원리 — 섹션 콤보 토독 방지)
// ============================================================

void AT3MidBossMonster::PlayBlockEntry(const FBlockMontageEntry& Entry, EBlockPhase NewPhase)
{
	if (!Entry.Montage)
	{
		UE_LOG(LogDesecration, Warning,
			TEXT("T3_MidBoss: 막기 Entry 몽타주 누락 — Phase=%d, 시퀀스 중단"), (int32)NewPhase);
		StopBlockSequence();
		return;
	}

	const FName StartSection = Entry.SectionName.IsNone() ? NAME_None : Entry.SectionName;
	PlayAnimMontage(Entry.Montage, Entry.PlayRate, StartSection);

	if (UAnimInstance* AnimInst = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr)
	{
		// 섹션 지정 시 해당 섹션만 재생 — 기본 섹션 순서 진행 방지 (체인 패턴과 동일)
		if (!StartSection.IsNone())
		{
			AnimInst->Montage_SetNextSection(StartSection, NAME_None, Entry.Montage);
		}

		// BlendingOut 시점에 다음 단계로 자연 크로스페이드
		FOnMontageBlendingOutStarted BlendOutDelegate;
		BlendOutDelegate.BindUObject(this, &AT3MidBossMonster::OnBlockMontageBlendingOut);
		AnimInst->Montage_SetBlendingOutDelegate(BlendOutDelegate, Entry.Montage);
	}

	// 활성 막기 몽타주 캐싱 — StopBlockSequence가 외과적으로 이 몽타주만 정지
	CurrentBlockMontage = Entry.Montage;

	CurrentBlockPhase = NewPhase;

	UE_LOG(LogDesecration, Log,
		TEXT("T3_MidBoss: 막기 Entry 재생 — Phase=%d, 몽타주='%s', 섹션='%s', PlayRate=%.2f"),
		(int32)NewPhase,
		*Entry.Montage->GetName(),
		StartSection.IsNone() ? TEXT("None") : *StartSection.ToString(),
		Entry.PlayRate);
}

void AT3MidBossMonster::StartBlockSequence()
{
	if (CurrentBlockPhase != EBlockPhase::Idle)
	{
		UE_LOG(LogDesecration, Verbose,
			TEXT("T3_MidBoss: 막기 시퀀스 이미 진행 중 (Phase=%d) — 시작 무시"), (int32)CurrentBlockPhase);
		return;
	}

	BlockHitsCount = 0;
	bBlockEndRequested = false;
	AddStateTag(TAG_Boss_State_Blocking);

	PlayBlockEntry(BlockMontageData.InEntry, EBlockPhase::In);
}

void AT3MidBossMonster::RequestEndBlockSequence()
{
	// 이미 종료 단계거나 비활성이면 무시
	if (CurrentBlockPhase == EBlockPhase::Idle || CurrentBlockPhase == EBlockPhase::Out)
	{
		return;
	}

	bBlockEndRequested = true;

	// Loop 단계는 자기루프(Section bLoop=true)거나 길 수 있어서 자연 BlendingOut을 영원히 못 받을 수 있음
	// → 능동적으로 Out 즉시 진입 (PlayAnimMontage가 Loop 인스턴스를 자연 크로스페이드로 끊음)
	// In 단계는 짧고 곧 자연 종료되니 BlendingOut 콜백에서 Out 직행 처리 (Loop 거치지 않음)
	if (CurrentBlockPhase == EBlockPhase::Loop)
	{
		UE_LOG(LogDesecration, Verbose,
			TEXT("T3_MidBoss: 막기 종료 요청 — Loop에서 즉시 Out 진입"));
		PlayBlockEntry(BlockMontageData.OutEntry, EBlockPhase::Out);
	}
	else
	{
		UE_LOG(LogDesecration, Verbose,
			TEXT("T3_MidBoss: 막기 종료 요청 — In 단계, 자연 종료 후 Loop 건너뛰고 Out 직행 예정"));
	}
}

void AT3MidBossMonster::StopBlockSequence()
{
	if (CurrentBlockPhase == EBlockPhase::Idle)
	{
		return;
	}

	// 활성 막기 몽타주만 외과적으로 정지 — 동시에 재생된 다른 몽타주(예: 스턴 진입 직후)는 보존
	// (StopAllMontages 사용 시 ApplyStun이 방금 시작한 StunMontage까지 같이 죽는 부작용 있어서 캐시 기반으로 전환)
	if (CurrentBlockMontage)
	{
		if (UAnimInstance* AnimInst = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr)
		{
			AnimInst->Montage_Stop(0.2f, CurrentBlockMontage);
		}
		CurrentBlockMontage = nullptr;
	}

	RemoveStateTag(TAG_Boss_State_Blocking);
	CurrentBlockPhase = EBlockPhase::Idle;
	bBlockEndRequested = false;

	UE_LOG(LogDesecration, Log,
		TEXT("T3_MidBoss: 막기 시퀀스 강제 종료 — 누적 피격수 %d"), BlockHitsCount);
}

void AT3MidBossMonster::OnBlockMontageBlendingOut(UAnimMontage* Montage, bool bInterrupted)
{
	// 인터럽트(StopBlockSequence/외부 강제 정지)면 단계 전환 없이 종료 — Idle은 이미 StopBlockSequence가 set
	if (bInterrupted)
	{
		return;
	}

	switch (CurrentBlockPhase)
	{
	case EBlockPhase::In:
		// In 진행 중 RequestEnd가 들어왔으면 Loop 거치지 말고 Out 직행 (반응성 보장)
		if (bBlockEndRequested)
		{
			PlayBlockEntry(BlockMontageData.OutEntry, EBlockPhase::Out);
		}
		else
		{
			PlayBlockEntry(BlockMontageData.LoopEntry, EBlockPhase::Loop);
		}
		break;

	case EBlockPhase::Loop:
		// 종료 요청 들어왔으면 Out 진입, 아니면 Loop 자기루프 (BlendingOut 자연 크로스페이드)
		if (bBlockEndRequested)
		{
			PlayBlockEntry(BlockMontageData.OutEntry, EBlockPhase::Out);
		}
		else
		{
			PlayBlockEntry(BlockMontageData.LoopEntry, EBlockPhase::Loop);
		}
		break;

	case EBlockPhase::Out:
		// Out 종료 → Idle 복귀 (STT_Block이 폴링하여 Succeeded 처리)
		RemoveStateTag(TAG_Boss_State_Blocking);
		CurrentBlockPhase = EBlockPhase::Idle;
		bBlockEndRequested = false;
		CurrentBlockMontage = nullptr;
		UE_LOG(LogDesecration, Log,
			TEXT("T3_MidBoss: 막기 시퀀스 정상 종료 — 누적 피격수 %d"), BlockHitsCount);
		break;

	default:
		break;
	}
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

void AT3MidBossMonster::ExecuteAoEDamage(float Radius, float DamageAmount, EHitIntensity Intensity,
	TSubclassOf<UT3DamageType_Base> DamageType)
{
	// AoE 중심 = 보스 발밑 (캡슐 하단)
	const FVector AoECenter = GetActorLocation() - FVector(0.0, 0.0, static_cast<double>(GetCapsuleComponent()->GetScaledCapsuleHalfHeight()));

	// 범위 내 Pawn 오버랩 (SphereOverlapActors — Pawn 오브젝트 타입)
	TArray<AActor*> OverlappedActors;
	TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
	ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_Pawn));

	UKismetSystemLibrary::SphereOverlapActors(
		this, AoECenter, Radius, ObjectTypes, nullptr, TArray<AActor*>{this}, OverlappedActors);

	// DamageType nullptr → UT3DamageType_Base fallback (BP 하위호환 + 호출자 미지정 방어)
	TSubclassOf<UDamageType> FinalDamageType = DamageType ? DamageType.Get() : UT3DamageType_Base::StaticClass();

	for (AActor* HitActor : OverlappedActors)
	{
		if (!HitActor)
		{
			continue;
		}

		FT3DamageEvent DamageEvent(FinalDamageType);
		DamageEvent.HitIntensity = Intensity;
		DamageEvent.HitDamageMultiplier = MidBossStats.AttackMultiplier;

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
	// AoE 사운드
	PlayBossSoundAt(AoESound, AoECenter, AoEVolumeMultiplier);

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

// ============================================================
// 플레이어 패링 반응 (플레이어가 보스 공격을 패링 성공 시)
// ============================================================

void AT3MidBossMonster::NotifyParriedByPlayer()
{
	// DamageType에서 이미 패링 가능 여부를 판정 완료
	// → 이 함수가 호출됐다 = 패링 성공 확정
	UE_LOG(LogDesecration, Log, TEXT("T3_MidBoss: 플레이어 패링 성공! 히트리액션 재생"));

	// 현재 패턴 캔슬
	CancelCurrentPattern();

	// 정면 히트리액션 재생 + 종료 델리게이트 바인딩 — 헬퍼 통과로 슬로우 존 실시간 갱신 적용
	UAnimMontage* HitReactMontage = GetDirectionalHitReactMontage(CombatTarget);
	if (HitReactMontage)
	{
		PlayMoveMontageWithSlow(HitReactMontage, 1.f);
		bParryHitReactionPlaying = true;

		// 몽타주 종료 시 콜백 바인딩
		if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
		{
			FOnMontageEnded EndDelegate;
			EndDelegate.BindUObject(this, &AT3MidBossMonster::OnParryHitReactionEnded);
			AnimInstance->Montage_SetEndDelegate(EndDelegate, HitReactMontage);
		}
	}

	// 외부 알림 (플레이어팀 등 바인딩 가능)
	OnParriedByPlayer.Broadcast();
}

void AT3MidBossMonster::OnParryHitReactionEnded(UAnimMontage* Montage, bool bInterrupted)
{
	if (!bParryHitReactionPlaying)
	{
		return;
	}
	bParryHitReactionPlaying = false;

	UE_LOG(LogDesecration, Log, TEXT("T3_MidBoss: 패링 히트리액션 종료 (중단:%s) → StateTree 이벤트 전송"),
		bInterrupted ? TEXT("Y") : TEXT("N"));

	// StateTree 이벤트 전송 — AC 잔량에 따라 분기
	if (ActionCount <= 0)
	{
		SendStateTreeStateEvent(TAG_Boss_Event_ActionCountDepleted);
		UE_LOG(LogDesecration, Log, TEXT("T3_MidBoss: 패링 + AC 소진 → ActionCountDepleted 이벤트 전송"));
	}
	else
	{
		SendStateTreeStateEvent(TAG_Boss_Event_ParriedByPlayer);
		UE_LOG(LogDesecration, Log, TEXT("T3_MidBoss: ParriedByPlayer 이벤트 전송 (AC:%d)"), ActionCount);
	}
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
			// 천사 장신구 슬로우는 별개 효과이므로 패링 카운터 풀스피드 복귀에도 곱해서 유지
			AnimInstance->Montage_SetPlayRate(
				PatternData->MontageChain[CurrentChainIndex].Montage, 1.0f * CurrentAttackAnimRate);
		}
	}

	// 패링 사운드
	PlayBossSoundAt(ParrySound, GetActorLocation(), ParryVolumeMultiplier);

	UE_LOG(LogDesecration, Log, TEXT("T3_MidBoss: 패링 반격 — Slow 해제, 몽타주 이어서 재생"));
}

// ============================================================
// 공통 사운드 헬퍼
// ============================================================

void AT3MidBossMonster::PlayBossSoundAt(USoundBase* Sound, const FVector& Loc, float VolumeMultiplier) const
{
	// nullptr 가드 + SoundVolume × Multiplier + SoundAttenuationSettings 일원화
	if (!Sound)
	{
		return;
	}

	UGameplayStatics::PlaySoundAtLocation(
		this, Sound, Loc, FRotator::ZeroRotator,
		SoundVolume * VolumeMultiplier, 1.f, 0.f, SoundAttenuationSettings);
}

void AT3MidBossMonster::SendStateTreeStateEvent(FGameplayTag Tag) const
{
	// StateTreeComponent 가드 — State/Event 태그를 겸용으로 전송
	if (StateTreeComponent)
	{
		StateTreeComponent->SendStateTreeEvent(Tag);
	}
}

void AT3MidBossMonster::PlayHitFeedback(const FVector& HitLoc, AActor* DamageCauser)
{
	// 피격 사운드 — 최소 간격 제한 (연속 히트 시 씹힘 방지)
	if (HitSound)
	{
		const double CurrentTime = GetWorld()->GetTimeSeconds();
		if (CurrentTime - LastHitSoundTime >= HitSoundMinInterval)
		{
			PlayBossSoundAt(HitSound, HitLoc, HitVolumeMultiplier);
			LastHitSoundTime = CurrentTime;
		}
	}

	// 카메라 쉐이크 — 상태 무관하게 항상 재생 (피격 피드백)
	if (HitCameraShakeClass)
	{
		UGameplayStatics::PlayWorldCameraShake(
			this, HitCameraShakeClass, HitLoc,
			0.f, HitShakeOuterRadius, HitShakeFalloff);

		UE_LOG(LogDesecration, Log, TEXT("T3_MidBoss: 카메라 쉐이크 재생 (Class: %s, Radius: %.0f)"),
			*HitCameraShakeClass->GetName(), HitShakeOuterRadius);
	}
	else
	{
		UE_LOG(LogDesecration, Warning, TEXT("T3_MidBoss: HitCameraShakeClass가 할당되지 않음!"));
	}

	// 히트 리액션 — 슈퍼아머 + 비기절 + 비공격 + 비이탈 + 비무적 + 비방어 + 생존 시에만 재생
	// 막기 중에는 막기 몽타주가 진행되어야 하므로 히트리액션이 끼어들면 안 됨 (블록 시퀀스 끊김 방지)
	if (HasSuperArmor() && !IsStunned() && !IsExecutingPattern() && !IsDisengaging() && !IsInvulnerable() && !IsBlocking() && MidBossStats.CurrentHP > 0.f)
	{
		PlayAdditiveHitReaction(DamageCauser);
	}
}

void AT3MidBossMonster::EnterDeathState()
{
	// 순서 보존: 태그 먼저 → HP clamp → 패턴 캔슬 → 델리게이트 → ST 이벤트 → 사망 시퀀스
	AddStateTag(TAG_Boss_State_Dead);
	MidBossStats.CurrentHP = 0.f;
	CancelCurrentPattern();

	// 리액션 트리거 클리어 — 현재는 ExecutePattern Dead 가드로 무영향이지만,
	// 향후 다른 곳에서 PendingReactionSource를 검사할 때를 대비한 방어적 정리
	PendingReactionSource = EBossReactionSource::None;
	OnMidBossDeath.Broadcast();

	// StateTree 이벤트 전송 — 즉시 Dead 상태로 전환 (State 태그를 이벤트로 겸용)
	SendStateTreeStateEvent(TAG_Boss_State_Dead);

	BeginDeathSequence();

	UE_LOG(LogDesecration, Log, TEXT("T3_MidBoss: %s 사망 (StateTree 이벤트 전송)"), *BossDisplayName.ToString());
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

void AT3MidBossMonster::SetAnimationSpeedMultiplier(
	float MoveSpeedMultiplier,
	float MoveAnimMultiplier,
	float AttackAnimMultiplier)
{
	// 천사 장신구가 sphere 진입/이탈 시 호출 → 절대 배율로 덮어써서 누적 방지 (멱등)
	CurrentMoveSpeedRate = FMath::Max(MoveSpeedMultiplier, 0.f);
	CurrentMoveAnimRate = FMath::Max(MoveAnimMultiplier, 0.f);
	CurrentAttackAnimRate = FMath::Max(AttackAnimMultiplier, 0.f);

	// 단일 진입점 — 현재 베이스(Default 또는 Strafe/Dash 푸시값)에 새 배율 적용
	ApplyCurrentWalkSpeed();

	// 진행 중 몽타주에 즉시 새 배율 반영 — sphere 진입/이탈 모두 실시간 적용
	// 어택 체인(full-body)과 헬퍼 추적 몽타주(HitReact additive 등)는 슬롯이 달라 동시 재생 가능
	// → GetCurrentActiveMontage(하나만)에 의존하지 않고 두 종류 모두 명시적으로 체크
	if (UAnimInstance* AnimInst = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr)
	{
		// 1) 어택 체인 — 현재 인덱스 몽타주가 재생 중이면 BaseRate × Attack배율로 갱신
		// 섹션 콤보는 [0]에만 몽타주 포인터가 있고 [1..]는 SectionName만 가짐 → [0]에서 조회
		// (PlayRate는 ApplyCurrentChainPlayRate가 [CurrentChainIndex] 기준으로 읽음 — 섹션별 배속 OK)
		const FMidBossAttackPattern* Pat = FindPatternData(CurrentPatternName);
		if (Pat && Pat->MontageChain.Num() > 0)
		{
			UAnimMontage* ChainM = nullptr;
			if (Pat->bUseSectionCombo)
			{
				ChainM = Pat->MontageChain[0].Montage;
			}
			else if (Pat->MontageChain.IsValidIndex(CurrentChainIndex))
			{
				ChainM = Pat->MontageChain[CurrentChainIndex].Montage;
			}

			if (ChainM && AnimInst->Montage_IsPlaying(ChainM))
			{
				ApplyCurrentChainPlayRate(AnimInst, ChainM, 1.f);
			}
		}

		// 2) 헬퍼 추적 몽타주 — 재생 중이면 BaseRate × Move배율로 갱신 (어택 체인과 별개 슬롯 가능)
		// Intro/Death raw 호출은 SlowManagedMontage에 안 잡혀서 자동 제외
		if (UAnimMontage* SlowM = SlowManagedMontage.Get())
		{
			if (AnimInst->Montage_IsPlaying(SlowM))
			{
				ApplyCurrentMoveMontagePlayRate(AnimInst, SlowM);
			}
		}
	}

	UE_LOG(LogDesecration, Warning, TEXT("T3_MidBoss: 애님 배율 변경 — MoveSpeed=%.2f, MoveAnim=%.2f, AttackAnim=%.2f"),
		CurrentMoveSpeedRate, CurrentMoveAnimRate, CurrentAttackAnimRate);
}

void AT3MidBossMonster::SetActiveBaseWalkSpeed(float NewBaseSpeed)
{
	// STNodes(Strafe/Dash 등)가 베이스 속도를 갱신 — 천사 슬로우 곱은 ApplyCurrentWalkSpeed에서 자동 적용
	ActiveBaseWalkSpeed = FMath::Max(NewBaseSpeed, 0.f);
	ApplyCurrentWalkSpeed();
}

void AT3MidBossMonster::ApplyCurrentWalkSpeed()
{
	// MaxWalkSpeed = 베이스 × 외부 이동 슬로우 배율 — 천사/STNodes 모든 호출이 이 한 곳을 거침
	// MoveSpeed와 MoveAnim는 별도 슬롯: 속도는 CurrentMoveSpeedRate로, 몽타주 PlayRate는 CurrentMoveAnimRate로 분리
	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		MoveComp->MaxWalkSpeed = ActiveBaseWalkSpeed * CurrentMoveSpeedRate;
	}
}

float AT3MidBossMonster::PlayMoveMontageWithSlow(UAnimMontage* Montage, float BaseRate)
{
	// 이동 계열 몽타주 재생 단일 진입점 — BaseRate 캐시 후 슬로우 곱셈하여 재생
	// 호출 측은 BaseRate만 넘기면 됨 (슬로우 진행 중 호출되어도 자동 반영)
	LastMoveBaseRate = FMath::Max(BaseRate, 0.f);
	// 슬로우 실시간 갱신 대상으로 추적 — 캐시 기준 일치할 때만 SetAnimationSpeedMultiplier에서 갱신됨
	SlowManagedMontage = Montage;
	return PlayAnimMontage(Montage, LastMoveBaseRate * CurrentMoveAnimRate);
}

void AT3MidBossMonster::ApplyCurrentMoveMontagePlayRate(UAnimInstance* AnimInst, UAnimMontage* Montage) const
{
	// PlayRate = LastMoveBaseRate × CurrentMoveAnimRate — 단일 계산 진입점
	// SetAnimationSpeedMultiplier에서 슬로우 진입/이탈 시 진행 중 몽타주에 즉시 반영
	if (AnimInst && Montage)
	{
		AnimInst->Montage_SetPlayRate(Montage, LastMoveBaseRate * CurrentMoveAnimRate);
	}
}
