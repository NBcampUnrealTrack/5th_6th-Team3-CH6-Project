#include "Monster/T3MidBossMonster.h"
#include "Monster/T3BossWeaponComponent.h"
#include "Monster/T3MidBossHPBarWidget.h"
#include "Desecration.h"
#include "AIController.h"
#include "Engine/DamageEvents.h"
#include "Blueprint/UserWidget.h"
#include "Components/AudioComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/TimelineComponent.h"
#include "Components/WidgetComponent.h"
#include "Curves/CurveFloat.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "NativeGameplayTags.h"

// Gameplay Tag 네이티브 정의 — 상태 태그 (State + Event 겸용: SendStateTreeEvent에도 사용)
UE_DEFINE_GAMEPLAY_TAG(TAG_Boss_State_Dead, "Boss.State.Dead");
UE_DEFINE_GAMEPLAY_TAG(TAG_Boss_State_Stunned, "Boss.State.Stunned");
UE_DEFINE_GAMEPLAY_TAG(TAG_Boss_State_ExecutingPattern, "Boss.State.ExecutingPattern");
UE_DEFINE_GAMEPLAY_TAG(TAG_Boss_State_SuperArmor, "Boss.State.SuperArmor");

// Gameplay Tag 네이티브 정의 — StateTree 전용 이벤트 태그 (대응 State 없음)
UE_DEFINE_GAMEPLAY_TAG(TAG_Boss_Event_StunRecovered, "Boss.Event.StunRecovered");
UE_DEFINE_GAMEPLAY_TAG(TAG_Boss_Event_ActionCountDepleted, "Boss.Event.ActionCountDepleted");

AT3MidBossMonster::AT3MidBossMonster()
{
	PrimaryActorTick.bCanEverTick = true;

	// 회전 설정 — 컨트롤러 회전 직접 적용 비활성화, MovementComponent가 부드럽게 보간
	bUseControllerRotationYaw = false;
	bUseControllerRotationPitch = false;
	bUseControllerRotationRoll = false;

	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		MoveComp->bOrientRotationToMovement = false;       // 이동 방향 회전 OFF (스트레이프 시 타겟 바라보기 위해)
		MoveComp->bUseControllerDesiredRotation = true;    // 컨트롤러 SetFocus 방향으로 부드럽게 보간
		MoveComp->RotationRate = FRotator(0.f, 360.f, 0.f); // 초당 360도 (에디터에서 조절 가능)
	}

	// StateTree 컴포넌트 (standalone 스키마 — AI Controller 없이 동작)
	StateTreeComponent = CreateDefaultSubobject<UStateTreeComponent>(TEXT("StateTreeComponent"));
	StateTreeComponent->SetStartLogicAutomatically(false);

	// MotionWarping 컴포넌트
	MotionWarpingComponent = CreateDefaultSubobject<UMotionWarpingComponent>(TEXT("MotionWarpingComponent"));

	// 무기 컴포넌트
	WeaponComponent = CreateDefaultSubobject<UT3BossWeaponComponent>(TEXT("WeaponComponent"));

	// 디졸브 타임라인 컴포넌트
	DissolveTimeline = CreateDefaultSubobject<UTimelineComponent>(TEXT("DissolveTimeline"));

	// 록온 위젯 컴포넌트
	LockOnWidgetComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("LockOnWidget"));
	LockOnWidgetComponent->SetupAttachment(GetMesh(), TEXT("LockOn_Socket"));
	LockOnWidgetComponent->SetWidgetSpace(EWidgetSpace::Screen);
	LockOnWidgetComponent->SetDrawSize(FVector2D(30.f, 15.f));
	LockOnWidgetComponent->SetVisibility(false);
	LockOnWidgetComponent->SetRelativeLocation(FVector::ZeroVector);
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

	// 록온 위젯 소켓 재부착 (SetupAttachment는 런타임 보장 안 됨)
	if (LockOnWidgetComponent && GetMesh())
	{
		LockOnWidgetComponent->AttachToComponent(
			GetMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, TEXT("LockOn_Socket"));
		LockOnWidgetComponent->SetRelativeLocation(FVector::ZeroVector);

		UE_LOG(LogDesecration, Log, TEXT("T3_MidBoss: LockOnWidget 소켓 부착 완료 (%s)"),
			*LockOnWidgetComponent->GetAttachSocketName().ToString());
	}

	// 무기 소켓 부착 + 히트 델리게이트 바인딩
	if (WeaponComponent)
	{
		WeaponComponent->AttachToSocket(GetMesh());
		WeaponComponent->OnWeaponHitActor.AddDynamic(this, &AT3MidBossMonster::OnWeaponHit);
	}

	MidBossStats.CurrentHP = MidBossStats.MaxHP;
	MidBossStats.CurrentStunGauge = 0.f;

	// 초기 상태 태그 설정 (기존 bSuperArmor = true 대체)
	AddStateTag(TAG_Boss_State_SuperArmor);

	// 노티파이 보정기 생성 및 초기화
	NotifyModifier = NewObject<UMidBossNotifyModifier>(this);
	NotifyModifier->Initialize(ModifierDataAsset);

	// 디졸브용 Dynamic Material 생성 + Timeline 셋업
	if (bEnableDissolve)
	{
		CreateDynamicMaterials();

		// 커브 결정 — 에디터에서 할당한 커브 우선, 없으면 선형 자동 생성
		UCurveFloat* CurveToUse = DissolveCurve;
		if (!CurveToUse)
		{
			DefaultDissolveCurve = NewObject<UCurveFloat>(this);
			DefaultDissolveCurve->FloatCurve.AddKey(0.f, 0.f);
			DefaultDissolveCurve->FloatCurve.AddKey(DissolveDuration, 1.f);
			CurveToUse = DefaultDissolveCurve;
		}

		// Timeline 바인딩
		if (DissolveTimeline && CurveToUse)
		{
			FOnTimelineFloat UpdateDelegate;
			UpdateDelegate.BindUFunction(this, FName("OnDissolveUpdate"));

			FOnTimelineEvent FinishedDelegate;
			FinishedDelegate.BindUFunction(this, FName("OnDissolveFinished"));

			DissolveTimeline->AddInterpFloat(CurveToUse, UpdateDelegate, FName("DissolveTrack"));
			DissolveTimeline->SetTimelineFinishedFunc(FinishedDelegate);
			DissolveTimeline->SetTimelineLength(DissolveDuration);
			DissolveTimeline->SetLooping(false);
		}
	}

	OnMidBossSpawned.Broadcast();

	UE_LOG(LogDesecration, Log, TEXT("T3_MidBoss: %s 스폰 완료 (HP: %.0f, Stage: %d, 등록 패턴: %d개, Modifier: %s, Dissolve: %s)"),
		*BossName, MidBossStats.MaxHP, BossStage, AttackPatterns.Num(),
		ModifierDataAsset ? TEXT("O") : TEXT("X"),
		bEnableDissolve ? TEXT("O") : TEXT("X"));
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

	// AIController SetFocus — MovementComponent가 RotationRate로 부드럽게 보간
	if (AAIController* AIC = Cast<AAIController>(GetController()))
	{
		if (CombatTarget && !IsStunned() && !IsDead())
		{
			AIC->SetFocus(CombatTarget);

			// 공격 중: 느린 트래킹 (다음 공격 시 스냅 방지) / 비공격: 빠른 트래킹
			if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
			{
				const float Rate = IsExecutingPattern() ? AttackRotationRate : IdleRotationRate;
				MoveComp->RotationRate = FRotator(0.f, Rate, 0.f);
			}
		}
		else
		{
			AIC->ClearFocus(EAIFocusPriority::Gameplay);
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

void AT3MidBossMonster::UpdateMotionWarpTarget()
{
	if (!MotionWarpingComponent || !CombatTarget)
	{
		return;
	}

	// 회전용 — 오프셋 없이 플레이어 정확한 위치 (근거리 회전 뒤집힘 방지)
	MotionWarpingComponent->AddOrUpdateWarpTargetFromComponent(
		MotionWarpTargetRotationName,
		CombatTarget->GetRootComponent(),
		NAME_None,
		true,  // bFollowComponent
		EWarpTargetLocationOffsetDirection::VectorFromTargetToOwner,
		FVector::ZeroVector
	);

	// 이동용 — 거리 기반 동적 오프셋 (근거리 후진 방지)
	const float Distance = FVector::Dist(GetActorLocation(), CombatTarget->GetActorLocation());
	const float ClampedOffset = FMath::Clamp(WarpTargetOffset, 0.f, Distance - MinWarpDistance);

	MotionWarpingComponent->AddOrUpdateWarpTargetFromComponent(
		MotionWarpTargetName,
		CombatTarget->GetRootComponent(),
		NAME_None,
		true,  // bFollowComponent
		EWarpTargetLocationOffsetDirection::VectorFromTargetToOwner,
		FVector(ClampedOffset, 0.f, 0.f)
	);
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
	if (MotionWarpingComponent) { MotionWarpingComponent->RemoveWarpTarget(MotionWarpTargetName); MotionWarpingComponent->RemoveWarpTarget(MotionWarpTargetRotationName); }
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
// 입장 (Entry) — 외부 트리거에서 호출
// ============================================================

void AT3MidBossMonster::ActivateBoss(AActor* Activator)
{
	if (bIsActivated || IsDead())
	{
		return;
	}

	if (!Activator)
	{
		UE_LOG(LogDesecration, Warning, TEXT("T3_MidBoss: ActivateBoss — Activator가 nullptr"));
		return;
	}

	bIsActivated = true;
	CombatTarget = Activator;

	// StateTree 시작 (생성자에서 자동 시작 비활성화)
	if (StateTreeComponent)
	{
		StateTreeComponent->StartLogic();
	}

	// BGM 재생 (2D — 공간 감쇠 없이 음악처럼 재생)
	if (BossBGM)
	{
		BGMAudioComponent = UGameplayStatics::SpawnSound2D(this, BossBGM, BGMVolume);
	}

	// 보스 HP바 위젯 생성 (PlayerController 기반)
	if (BossHPBarWidgetClass)
	{
		if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
		{
			BossHPBarWidget = CreateWidget<UT3MidBossHPBarWidget>(PC, BossHPBarWidgetClass);
			if (BossHPBarWidget)
			{
				BossHPBarWidget->TargetBoss = this;
				BossHPBarWidget->AddToViewport();
			}
		}
	}

	OnMidBossActivated.Broadcast();

	UE_LOG(LogDesecration, Log, TEXT("T3_MidBoss: %s 활성화 (타겟: %s, StateTree 시작, BGM: %s, HPBar: %s)"),
		*BossName, *Activator->GetName(),
		BossBGM ? TEXT("O") : TEXT("X"),
		BossHPBarWidget ? TEXT("O") : TEXT("X"));
}

// ============================================================
// 사망 연출 (Death Sequence)
// ============================================================

void AT3MidBossMonster::BeginDeathSequence()
{
	// 사망 사운드 재생
	if (DeathSound)
	{
		UGameplayStatics::PlaySoundAtLocation(
			this, DeathSound, GetActorLocation(),
			SoundVolume * DeathVolumeMultiplier);
	}

	// BGM 페이드아웃
	if (BGMAudioComponent && BGMAudioComponent->IsPlaying())
	{
		BGMAudioComponent->FadeOut(BGMFadeOutDuration, 0.f);
	}

	// 사망 몽타주 재생
	if (DeathMontage)
	{
		const float Duration = PlayAnimMontage(DeathMontage);
		if (Duration > 0.f)
		{
			UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
			if (AnimInstance)
			{
				// BlendingOut 시점에 콜백 — 블렌드아웃 전에 포즈 고정
				FOnMontageBlendingOutStarted BlendOutDelegate;
				BlendOutDelegate.BindUObject(this, &AT3MidBossMonster::OnDeathMontageEnded);
				AnimInstance->Montage_SetBlendingOutDelegate(BlendOutDelegate, DeathMontage);
			}

			UE_LOG(LogDesecration, Log, TEXT("T3_MidBoss: 사망 몽타주 재생 (%.1f초)"), Duration);
			return;
		}
	}

	// 몽타주 없거나 재생 실패 시 즉시 완료
	FinishDeathSequence();
}

void AT3MidBossMonster::OnDeathMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	// 사망 포즈 고정 — ABP가 Idle로 복귀하지 않도록 애니메이션 정지
	GetMesh()->bPauseAnims = true;

	FinishDeathSequence();
}

void AT3MidBossMonster::FinishDeathSequence()
{
	// 무기 드롭 (bAutoDropWeapon=false면 AnimNotify 등에서 수동 호출)
	if (bAutoDropWeapon && WeaponComponent)
	{
		WeaponComponent->DropWeapon();
	}

	// 캡슐 충돌 해제 — 플레이어 통과 가능
	if (UCapsuleComponent* Capsule = GetCapsuleComponent())
	{
		Capsule->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	// 이동 비활성화
	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		MoveComp->DisableMovement();
	}

	// AI 회전 정지
	if (AAIController* AIC = Cast<AAIController>(GetController()))
	{
		AIC->ClearFocus(EAIFocusPriority::Gameplay);
	}

	// StateTree 정지
	if (StateTreeComponent)
	{
		StateTreeComponent->StopLogic(TEXT("Death"));
	}

	// 사망 연출 완료 델리게이트 — Level BP에서 안개벽 해제, 보상 등
	OnMidBossDeathFinished.Broadcast();

	// 디졸브 시작 (활성화 시) — 디졸브 완료 후 SetLifeSpan
	if (bEnableDissolve && DynamicMaterials.Num() > 0)
	{
		StartDissolve();
		return;
	}

	// 디졸브 비활성화 시 기존 방식
	SetLifeSpan(DeathCleanupDelay);

	UE_LOG(LogDesecration, Log, TEXT("T3_MidBoss: %s 사망 연출 완료 (%.0f초 후 제거)"),
		*BossName, DeathCleanupDelay);
}

// ============================================================
// 디졸브 연출
// ============================================================

void AT3MidBossMonster::CreateDynamicMaterials()
{
	USkeletalMeshComponent* MeshComp = GetMesh();
	if (!MeshComp) return;

	const int32 NumMaterials = MeshComp->GetNumMaterials();
	DynamicMaterials.Reserve(NumMaterials);

	for (int32 i = 0; i < NumMaterials; ++i)
	{
		UMaterialInstanceDynamic* DynMat = MeshComp->CreateAndSetMaterialInstanceDynamic(i);
		if (DynMat)
		{
			DynamicMaterials.Add(DynMat);
		}
	}

	UE_LOG(LogDesecration, Verbose, TEXT("T3_MidBoss: DynamicMaterial %d개 생성"), DynamicMaterials.Num());
}

void AT3MidBossMonster::StartDissolve()
{
	if (!DissolveTimeline)
	{
		UE_LOG(LogDesecration, Warning, TEXT("T3_MidBoss: DissolveTimeline이 nullptr — 디졸브 스킵"));
		SetLifeSpan(DeathCleanupDelay);
		return;
	}

	// 디졸브 사운드 재생
	if (DissolveSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, DissolveSound, GetActorLocation());
	}

	DissolveTimeline->PlayFromStart();

	UE_LOG(LogDesecration, Log, TEXT("T3_MidBoss: %s 디졸브 시작 (%.1f초, 머티리얼 %d개)"),
		*BossName, DissolveDuration, DynamicMaterials.Num());
}

void AT3MidBossMonster::OnDissolveUpdate(float Value)
{
	for (UMaterialInstanceDynamic* DynMat : DynamicMaterials)
	{
		if (DynMat)
		{
			DynMat->SetScalarParameterValue(DissolveParameterName, Value);
		}
	}
}

void AT3MidBossMonster::OnDissolveFinished()
{
	UE_LOG(LogDesecration, Log, TEXT("T3_MidBoss: %s 디졸브 완료 — 액터 제거 예정"),
		*BossName);

	// 디졸브 완료 후 짧은 딜레이로 제거
	SetLifeSpan(0.5f);
}

// ============================================================
// 록온 위젯 표시
// ============================================================

void AT3MidBossMonster::SetLockOnWidgetVisible(bool bVisible)
{
	if (LockOnWidgetComponent)
	{
		LockOnWidgetComponent->SetVisibility(bVisible);
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
