#include "Monster/T3MidBossSTNodes.h"
#include "Desecration.h"
#include "Monster/T3MidBossMonster.h"
#include "AIController.h"
#include "Navigation/PathFollowingComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "StateTreeExecutionContext.h"
#include "Animation/AnimInstance.h"
#include "Components/SkeletalMeshComponent.h"

// ============================================================
// Evaluator: FT3STE_MidBossCombat
// Boss 포인터를 하위 노드에 전달하는 역할만 담당
// 모든 상태 전환은 SendStateTreeEvent()로 처리 (폴링 없음)
// ============================================================

void FT3STE_MidBossCombat::TreeStart(FStateTreeExecutionContext& Context) const
{
	const FT3STE_MidBossCombatInstanceData& Data = Context.GetInstanceData(*this);

	if (!Data.Boss)
	{
		UE_LOG(LogDesecration, Warning, TEXT("T3_ST: MidBossCombat Evaluator — Boss가 바인딩되지 않음"));
		return;
	}

	UE_LOG(LogDesecration, Log, TEXT("T3_ST: Evaluator 초기화 완료 (Boss 바인딩)"));
}

// ============================================================
// Task: FT3STT_ExecutePattern
// ============================================================

EStateTreeRunStatus FT3STT_ExecutePattern::EnterState(
	FStateTreeExecutionContext& Context,
	const FStateTreeTransitionResult& Transition) const
{
	FT3STT_ExecutePatternInstanceData& Data = Context.GetInstanceData(*this);

	Data.bPatternStarted = false;
	Data.bPatternCompleted = false;
	Data.CachedActionCountCost = 1;
	Data.CachedRequiredStage = 1;

	if (!Data.Boss)
	{
		UE_LOG(LogDesecration, Warning, TEXT("T3_ST: ExecutePattern — Boss가 바인딩되지 않음"));
		return EStateTreeRunStatus::Failed;
	}

	if (Data.PatternName.IsNone())
	{
		UE_LOG(LogDesecration, Warning, TEXT("T3_ST: ExecutePattern — PatternName이 비어 있음"));
		return EStateTreeRunStatus::Failed;
	}

	// ActionCountCost + RequiredStage 캐시
	FMidBossAttackPattern PatternData;
	if (Data.Boss->GetPatternData(Data.PatternName, PatternData))
	{
		Data.CachedActionCountCost = PatternData.ActionCountCost;
		Data.CachedRequiredStage = PatternData.RequiredStage;
	}

	// 리액션 모드 자동 감지 — bAsReaction 명시 OR PendingReactionSource 활성 시 리액션 진입.
	// (Why: BP의 모든 ExecutePattern 노드를 bAsReaction=true로 토글하지 않아도 Roll/Block 종료 후 1회 자동 적용. ExecutePattern 본체에서 None으로 소모됨)
	const bool bResolvedReaction = Data.bAsReaction
		|| Data.Boss->PendingReactionSource != EBossReactionSource::None;

	// 패턴 실행 시도 (리액션 모드면 ReactionStartSectionOverride / ReactionSectionNameOverride 적용)
	if (!Data.Boss->ExecutePattern(Data.PatternName, bResolvedReaction))
	{
		return EStateTreeRunStatus::Failed;
	}

	// 패턴 시작 성공 시 즉시 AC 감소 (Evaluator가 다음 틱에 올바른 값을 읽을 수 있도록)
	Data.Boss->ActionCount -= Data.CachedActionCountCost;
	UE_LOG(LogDesecration, Log,
		TEXT("T3_ST: ExecutePattern 시작 — '%s' (AC 소모:%d, 남은 AC:%d)"),
		*Data.PatternName.ToString(), Data.CachedActionCountCost, Data.Boss->ActionCount);

	// 델리게이트 바인딩 — 패턴 완료 시 플래그 세팅
	bool* CompletedFlagPtr = &Data.bPatternCompleted;
	Data.PatternCompletedHandle = Data.Boss->OnPatternCompletedNative.AddLambda(
		[CompletedFlagPtr]()
		{
			*CompletedFlagPtr = true;
		});

	Data.bPatternStarted = true;
	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FT3STT_ExecutePattern::Tick(
	FStateTreeExecutionContext& Context,
	const float DeltaTime) const
{
	FT3STT_ExecutePatternInstanceData& Data = Context.GetInstanceData(*this);

	if (!Data.Boss || !Data.bPatternStarted)
	{
		return EStateTreeRunStatus::Failed;
	}

	// 델리게이트로 완료 감지
	if (Data.bPatternCompleted)
	{
		const int32 StageIdx = FMath::Clamp(Data.CachedRequiredStage - 1, 0, 2);
		Data.Boss->StagePatternCounts[StageIdx]++;

		UE_LOG(LogDesecration, Log,
			TEXT("T3_ST: ExecutePattern 완료 — '%s' (AC:%d, Stage%d횟수:%d)"),
			*Data.PatternName.ToString(), Data.Boss->ActionCount,
			Data.CachedRequiredStage, Data.Boss->StagePatternCounts[StageIdx]);

		// ActionCount <= 0이면 이벤트 전송 → Disengage 트랜지션 트리거
		if (Data.Boss->ActionCount <= 0 && Data.Boss->StateTreeComponent)
		{
			Data.Boss->StateTreeComponent->SendStateTreeEvent(TAG_Boss_Event_ActionCountDepleted);
			UE_LOG(LogDesecration, Log, TEXT("T3_ST: ActionCountDepleted 이벤트 전송"));
		}

		return EStateTreeRunStatus::Succeeded;
	}

	return EStateTreeRunStatus::Running;
}

void FT3STT_ExecutePattern::ExitState(
	FStateTreeExecutionContext& Context,
	const FStateTreeTransitionResult& Transition) const
{
	FT3STT_ExecutePatternInstanceData& Data = Context.GetInstanceData(*this);

	// 델리게이트 해제
	if (Data.Boss && Data.PatternCompletedHandle.IsValid())
	{
		Data.Boss->OnPatternCompletedNative.Remove(Data.PatternCompletedHandle);
		Data.PatternCompletedHandle.Reset();
	}

	// 상태 전환 시 실행 중인 패턴 강제 중단
	if (Data.bPatternStarted && Data.Boss && Data.Boss->IsExecutingPattern())
	{
		Data.Boss->CancelCurrentPattern();
		UE_LOG(LogDesecration, Log,
			TEXT("T3_ST: ExecutePattern ExitState — '%s' 강제 중단"),
			*Data.PatternName.ToString());
	}

	Data.bPatternStarted = false;
	Data.bPatternCompleted = false;
}

// ============================================================
// 헬퍼: AIController 조회
// ============================================================

static AAIController* GetBossAIController(AT3MidBossMonster* Boss)
{
	if (!Boss)
	{
		return nullptr;
	}
	return Cast<AAIController>(Boss->GetController());
}

// ============================================================
// Task: FT3STT_ApproachTarget
// ============================================================

EStateTreeRunStatus FT3STT_ApproachTarget::EnterState(
	FStateTreeExecutionContext& Context,
	const FStateTreeTransitionResult& Transition) const
{
	FT3STT_ApproachTargetInstanceData& Data = Context.GetInstanceData(*this);

	Data.bArrived = false;
	Data.DelayElapsed = 0.f;
	Data.ElapsedTime = 0.f;

	if (!Data.Boss)
	{
		UE_LOG(LogDesecration, Warning, TEXT("T3_ST: ApproachTarget — Boss가 바인딩되지 않음"));
		return EStateTreeRunStatus::Failed;
	}

	if (!Data.Boss->CombatTarget)
	{
		UE_LOG(LogDesecration, Warning, TEXT("T3_ST: ApproachTarget — CombatTarget이 없음"));
		return EStateTreeRunStatus::Failed;
	}

	// AIController를 통한 NavMesh 이동 요청 (설정값보다 150 안쪽까지 접근 — 캡슐 반경 고려)
	const float MoveToRadius = FMath::Max(Data.AcceptableRadius - 150.f, 0.f);
	AAIController* AIC = GetBossAIController(Data.Boss);
	if (AIC)
	{
		AIC->MoveToActor(Data.Boss->CombatTarget, MoveToRadius);
	}
	else
	{
		UE_LOG(LogDesecration, Warning, TEXT("T3_ST: ApproachTarget — AIController가 없음"));
		return EStateTreeRunStatus::Failed;
	}

	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FT3STT_ApproachTarget::Tick(
	FStateTreeExecutionContext& Context,
	const float DeltaTime) const
{
	FT3STT_ApproachTargetInstanceData& Data = Context.GetInstanceData(*this);

	if (!Data.Boss || !Data.Boss->CombatTarget)
	{
		return EStateTreeRunStatus::Failed;
	}

	// 타임아웃 체크 — 도달 전에만 적용
	if (!Data.bArrived)
	{
		Data.ElapsedTime += DeltaTime;
		if (Data.ElapsedTime >= Data.Timeout)
		{
			UE_LOG(LogDesecration, Log,
				TEXT("T3_ST: ApproachTarget 타임아웃 (%.1f초) — 패턴 재선택"),
				Data.Timeout);
			return EStateTreeRunStatus::Failed;
		}
	}

	if (!Data.bArrived)
	{
		// Phase 1: NavMesh 이동 중 — 도달 체크 (회전은 MovementComponent가 SetFocus로 처리)
		const float Dist = FVector::Dist(
			Data.Boss->GetActorLocation(),
			Data.Boss->CombatTarget->GetActorLocation());

		if (Dist <= Data.AcceptableRadius)
		{
			Data.bArrived = true;
			Data.DelayElapsed = 0.f;

			// 이동 중지
			AAIController* AIC = GetBossAIController(Data.Boss);
			if (AIC)
			{
				AIC->StopMovement();
			}

			UE_LOG(LogDesecration, Log,
				TEXT("T3_ST: ApproachTarget — 도달 (거리:%.0f, 반경:%.0f)"),
				Dist, Data.AcceptableRadius);
		}
		else
		{
			// 플레이어가 이동하여 AI 이동이 완료됐지만 범위 밖일 때 재추적
			const float RetryRadius = FMath::Max(Data.AcceptableRadius - 150.f, 0.f);
			AAIController* AIC = GetBossAIController(Data.Boss);
			if (AIC && AIC->GetMoveStatus() != EPathFollowingStatus::Moving)
			{
				AIC->MoveToActor(Data.Boss->CombatTarget, RetryRadius);
			}
		}
	}
	else
	{
		// Phase 2: 도달 후 대기 (회전은 MovementComponent가 SetFocus로 처리)
		Data.DelayElapsed += DeltaTime;

		if (Data.DelayElapsed >= Data.PostArrivalDelay)
		{
			if (Data.bResetActionCount)
			{
				Data.Boss->ActionCount = Data.ActionCountReset;
				UE_LOG(LogDesecration, Log,
					TEXT("T3_ST: ApproachTarget — ActionCount 리셋 (%d)"),
					Data.ActionCountReset);
			}
			return EStateTreeRunStatus::Succeeded;
		}
	}

	return EStateTreeRunStatus::Running;
}

void FT3STT_ApproachTarget::ExitState(
	FStateTreeExecutionContext& Context,
	const FStateTreeTransitionResult& Transition) const
{
	FT3STT_ApproachTargetInstanceData& Data = Context.GetInstanceData(*this);

	if (Data.Boss)
	{
		AAIController* AIC = GetBossAIController(Data.Boss);
		if (AIC)
		{
			AIC->StopMovement();
		}

		// 타임아웃(Failed)으로 종료되어도 ActionCount 리셋
		if (Data.bResetActionCount && Transition.CurrentRunStatus == EStateTreeRunStatus::Failed)
		{
			Data.Boss->ActionCount = Data.ActionCountReset;
			UE_LOG(LogDesecration, Log,
				TEXT("T3_ST: ApproachTarget — 타임아웃, ActionCount 리셋 (%d)"),
				Data.ActionCountReset);
		}
	}
}

// ============================================================
// Task: FT3STT_WaitForStunEnd
// 이벤트 기반 — Boss.Event.StunRecovered 이벤트로 상태 전환
// Task는 스턴 상태에서의 동작(몽타주 재생 등)만 담당
// ============================================================

EStateTreeRunStatus FT3STT_WaitForStunEnd::EnterState(
	FStateTreeExecutionContext& Context,
	const FStateTreeTransitionResult& Transition) const
{
	FT3STT_WaitForStunEndInstanceData& Data = Context.GetInstanceData(*this);

	if (!Data.Boss)
	{
		UE_LOG(LogDesecration, Warning, TEXT("T3_ST: WaitForStunEnd — Boss가 바인딩되지 않음"));
		return EStateTreeRunStatus::Failed;
	}

	// TODO: 스턴 몽타주 재생 (추후 구현)
	// Data.Boss->PlayStunMontage();

	UE_LOG(LogDesecration, Log,
		TEXT("T3_ST: WaitForStunEnd — 스턴 대기 (%.1f초, 이벤트 기반 전환)"),
		Data.Boss->StunDuration);

	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FT3STT_WaitForStunEnd::Tick(
	FStateTreeExecutionContext& Context,
	const float DeltaTime) const
{
	// 이벤트 기반 전환 — Tick에서 완료 체크 불필요
	// Boss.Event.StunRecovered 이벤트가 StateTree 트랜지션을 트리거
	return EStateTreeRunStatus::Running;
}

void FT3STT_WaitForStunEnd::ExitState(
	FStateTreeExecutionContext& Context,
	const FStateTreeTransitionResult& Transition) const
{
	// 이벤트로 상태 전환 시 자동 호출됨
	UE_LOG(LogDesecration, Log, TEXT("T3_ST: WaitForStunEnd — ExitState (이벤트 전환)"));
}

// ============================================================
// Task: FT3STT_Disengage
// ============================================================

EStateTreeRunStatus FT3STT_Disengage::EnterState(
	FStateTreeExecutionContext& Context,
	const FStateTreeTransitionResult& Transition) const
{
	FT3STT_DisengageInstanceData& Data = Context.GetInstanceData(*this);

	Data.ElapsedTime = 0.f;
	Data.CachedDefaultSpeed = 0.f;

	if (!Data.Boss)
	{
		UE_LOG(LogDesecration, Warning, TEXT("T3_ST: Disengage — Boss가 바인딩되지 않음"));
		return EStateTreeRunStatus::Failed;
	}

	// Disengaging 상태 태그 설정 (히트리액션 무시용)
	Data.Boss->AddStateTag(TAG_Boss_State_Disengaging);

	// 연속 Disengage 카운터 증가
	Data.Boss->ConsecutiveDisengageCount++;

	// 스테이지별 패턴 카운트 리셋
	UE_LOG(LogDesecration, Log, TEXT("T3_ST: Disengage — StagePatternCounts 리셋 [%d,%d,%d → 0], 연속횟수:%d"),
		Data.Boss->StagePatternCounts[0], Data.Boss->StagePatternCounts[1], Data.Boss->StagePatternCounts[2],
		Data.Boss->ConsecutiveDisengageCount);
	Data.Boss->StagePatternCounts[0] = 0;
	Data.Boss->StagePatternCounts[1] = 0;
	Data.Boss->StagePatternCounts[2] = 0;

	// 횡이동 방향 랜덤 결정
	if (Data.bStrafe)
	{
		Data.StrafeDirection = FMath::RandBool() ? 1.f : -1.f;
	}

	// Strafe 속도 적용 — Boss의 단일 베이스 속도 진입점 사용 (천사 장신구 슬로우 곱 자동 적용)
	if (Data.StrafeSpeed > 0.f)
	{
		Data.CachedDefaultSpeed = Data.Boss->GetActiveBaseWalkSpeed();
		Data.Boss->SetActiveBaseWalkSpeed(Data.StrafeSpeed);
	}

	// 루트모션 거리 스케일 적용
	if (Data.RootMotionScale != 1.0f)
	{
		Data.Boss->SetAnimRootMotionTranslationScale(Data.RootMotionScale);
	}

	// 진입 전 잔여 몽타주 정리 (히트리액션 등이 남아있으면 백스텝과 겹침)
	Data.Boss->StopAnimMontage();

	// 백스텝 몽타주 재생 (보스별 몽타주는 AT3MidBossMonster::BackStepMontage에서 참조)
	// 천사 장신구 이동 슬로우 — 헬퍼 일원화 (BaseRate 캐시 + 실시간 슬로우 갱신)
	if (Data.bBackStep && Data.Boss->BackStepMontage)
	{
		Data.Boss->PlayMoveMontageWithSlow(Data.Boss->BackStepMontage, 1.f);
	}

	UE_LOG(LogDesecration, Log,
		TEXT("T3_ST: Disengage 시작 (Duration:%.1f, Strafe:%s, BackStep:%s)"),
		Data.Duration,
		Data.bStrafe ? TEXT("true") : TEXT("false"),
		Data.bBackStep ? TEXT("true") : TEXT("false"));

	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FT3STT_Disengage::Tick(
	FStateTreeExecutionContext& Context,
	const float DeltaTime) const
{
	FT3STT_DisengageInstanceData& Data = Context.GetInstanceData(*this);

	if (!Data.Boss)
	{
		return EStateTreeRunStatus::Failed;
	}

	Data.ElapsedTime += DeltaTime;

	if (Data.ElapsedTime >= Data.Duration)
	{
		return EStateTreeRunStatus::Succeeded;
	}

	// 타겟이 있을 때만 이동 입력
	if (Data.Boss->CombatTarget)
	{
		const FVector ToTarget = (Data.Boss->CombatTarget->GetActorLocation()
			- Data.Boss->GetActorLocation()).GetSafeNormal2D();

		if (Data.bStrafe)
		{
			// 타겟 기준 측면 방향으로 연속 이동
			const FVector StrafeDir = FVector::CrossProduct(FVector::UpVector, ToTarget) * Data.StrafeDirection;
			Data.Boss->AddMovementInput(StrafeDir, 1.0f);
		}

		if (Data.bBackStep)
		{
			// 타겟 반대 방향으로 후퇴
			Data.Boss->AddMovementInput(-ToTarget, 1.0f);
		}
	}

	return EStateTreeRunStatus::Running;
}

void FT3STT_Disengage::ExitState(
	FStateTreeExecutionContext& Context,
	const FStateTreeTransitionResult& Transition) const
{
	FT3STT_DisengageInstanceData& Data = Context.GetInstanceData(*this);

	if (Data.Boss)
	{
		// Disengaging 상태 태그 해제
		Data.Boss->RemoveStateTag(TAG_Boss_State_Disengaging);

		// 백스텝 몽타주 정지
		if (Data.bBackStep && Data.Boss->BackStepMontage)
		{
			Data.Boss->StopAnimMontage(Data.Boss->BackStepMontage);
		}

		// 루트모션 스케일 복원
		if (Data.RootMotionScale != 1.0f)
		{
			Data.Boss->SetAnimRootMotionTranslationScale(1.0f);
		}

		// Strafe 속도 복원 — Failed/Succeeded 모두 안전하게 이전 베이스로 복원
		if (Data.CachedDefaultSpeed > 0.f)
		{
			Data.Boss->SetActiveBaseWalkSpeed(Data.CachedDefaultSpeed);
			Data.CachedDefaultSpeed = 0.f;
		}
	}
}

// ============================================================
// Task: FT3STT_TestRoll
// 임시 데모용 — 8방향 회피 모션 시각 검증 (Desmond 프로토타입)
// ============================================================

EStateTreeRunStatus FT3STT_TestRoll::EnterState(
	FStateTreeExecutionContext& Context,
	const FStateTreeTransitionResult& Transition) const
{
	FT3STT_TestRollInstanceData& Data = Context.GetInstanceData(*this);

	Data.bRollEnded = false;
	Data.DelayElapsed = 0.f;
	Data.ActiveRoll = nullptr;
	Data.bAppliedScale = false;

	if (!Data.Boss)
	{
		UE_LOG(LogDesecration, Warning, TEXT("T3_ST: TestRoll — Boss가 바인딩되지 않음"));
		return EStateTreeRunStatus::Failed;
	}

	if (Data.Boss->RollMontages_8Dir.Num() != 8)
	{
		UE_LOG(LogDesecration, Warning,
			TEXT("T3_ST: TestRoll — Boss->RollMontages_8Dir가 8개여야 함 (현재 %d개)"),
			Data.Boss->RollMontages_8Dir.Num());
		return EStateTreeRunStatus::Failed;
	}

	// 방향 인덱스 결정 — 0°/45°/90°/135°/180°/225°/270°/315° 순
	int32 Idx = 0;
	if (Data.bRandomDirection)
	{
		Idx = FMath::RandRange(0, 7);
	}
	else
	{
		const int32 Raw = FMath::RoundToInt(Data.FixedDirectionYaw / 45.f);
		Idx = ((Raw % 8) + 8) % 8;  // 음수 안전
	}

	UAnimMontage* Roll = Data.Boss->RollMontages_8Dir[Idx];
	if (!Roll)
	{
		UE_LOG(LogDesecration, Warning,
			TEXT("T3_ST: TestRoll — Boss->RollMontages_8Dir[%d] 슬롯이 비어있음"), Idx);
		return EStateTreeRunStatus::Failed;
	}

	// i-frame — 몽타주 전체 구간에 Invulnerable 태그 적용 (TakeDamage가 데미지 0 처리)
	// 더 정밀한 5~45f 윈도우가 필요하면 이 플래그 false + 몽타주에 ANS_BossInvulnerable 배치
	if (Data.bUseInvulnerableTag)
	{
		Data.Boss->AddStateTag(TAG_Boss_State_Invulnerable);
	}

	// 방향별 배율 조회 — 배열이 비었거나 해당 인덱스가 없으면 1.0 (Boss->RollDirectionScales 참조)
	const float PerDirScale = Data.Boss->RollDirectionScales.IsValidIndex(Idx)
		? Data.Boss->RollDirectionScales[Idx]
		: 1.0f;
	const float FinalScale = Data.RootMotionScale * PerDirScale;

	// 루트모션 거리 배율 적용 (1.0과 다를 때만)
	if (!FMath::IsNearlyEqual(FinalScale, 1.0f))
	{
		Data.Boss->SetAnimRootMotionTranslationScale(FinalScale);
		Data.bAppliedScale = true;
	}

	// 잔여 몽타주 정리 후 회피 재생 — 헬퍼 일원화 (BaseRate 캐시 + 실시간 슬로우 갱신)
	Data.Boss->StopAnimMontage();
	const float Duration = Data.Boss->PlayMoveMontageWithSlow(Roll, Data.PlayRate);

	Data.ActiveRoll = Roll;

	// 경직치 누적 — 데스몬드처럼 StaggerOnRoll>0인 보스만 동작 (다크나이트는 0이라 헬퍼 내부에서 early-out)
	Data.Boss->AddStunGauge(Data.Boss->StaggerOnRoll);

	UE_LOG(LogDesecration, Log,
		TEXT("T3_ST: TestRoll 시작 (Idx:%d, Angle:%.0f°, Duration:%.2f, Rate:%.2f, DistScale:%.2f[Master:%.2f × Dir:%.2f])"),
		Idx, Idx * 45.f, Duration, Data.PlayRate,
		FinalScale, Data.RootMotionScale, PerDirScale);

	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FT3STT_TestRoll::Tick(
	FStateTreeExecutionContext& Context,
	const float DeltaTime) const
{
	FT3STT_TestRollInstanceData& Data = Context.GetInstanceData(*this);

	if (!Data.Boss)
	{
		return EStateTreeRunStatus::Failed;
	}

	// 몽타주 재생 종료 감지 — 폴링 (델리게이트 바인딩보다 단순)
	if (!Data.bRollEnded)
	{
		const UAnimInstance* AnimInst = Data.Boss->GetMesh()
			? Data.Boss->GetMesh()->GetAnimInstance()
			: nullptr;
		const bool bStillPlaying = AnimInst
			&& Data.ActiveRoll
			&& AnimInst->Montage_IsPlaying(Data.ActiveRoll);

		if (!bStillPlaying)
		{
			Data.bRollEnded = true;

			// 종료 시 i-frame 태그 제거 (Exit에서도 안전망 한 번 더)
			if (Data.bUseInvulnerableTag)
			{
				Data.Boss->RemoveStateTag(TAG_Boss_State_Invulnerable);
			}
		}
	}

	// 종료 후 PostRollDelay 만큼 대기 → Succeeded
	if (Data.bRollEnded)
	{
		Data.DelayElapsed += DeltaTime;
		if (Data.DelayElapsed >= Data.PostRollDelay)
		{
			return EStateTreeRunStatus::Succeeded;
		}
	}

	return EStateTreeRunStatus::Running;
}

void FT3STT_TestRoll::ExitState(
	FStateTreeExecutionContext& Context,
	const FStateTreeTransitionResult& Transition) const
{
	FT3STT_TestRollInstanceData& Data = Context.GetInstanceData(*this);

	if (Data.Boss)
	{
		// 안전 복원 — Failed/Succeeded 모두 태그 제거
		if (Data.bUseInvulnerableTag)
		{
			Data.Boss->RemoveStateTag(TAG_Boss_State_Invulnerable);
		}

		// 루트모션 스케일 복원 (EnterState에서 실제로 적용했을 때만)
		if (Data.bAppliedScale)
		{
			Data.Boss->SetAnimRootMotionTranslationScale(1.0f);
		}

		// 진행 중 몽타주가 있으면 정지 (조기 종료 대비)
		if (Data.ActiveRoll)
		{
			Data.Boss->StopAnimMontage(Data.ActiveRoll);
		}

		// 정상 Succeeded 시에만 리액션 트리거 세팅 — 인터럽트로 끊긴 경우 제외
		// (bRollEnded=true && DelayElapsed>=PostRollDelay 도달이 정상 종료 조건)
		const bool bNormalCompletion = Data.bRollEnded && (Data.DelayElapsed >= Data.PostRollDelay);
		if (bNormalCompletion)
		{
			Data.Boss->PendingReactionSource = EBossReactionSource::FromRoll;
			UE_LOG(LogDesecration, Log, TEXT("T3_ST: Roll 정상 종료 → PendingReactionSource=FromRoll 세팅"));
		}

		// [DEBUG:ReactionTest] Roll 정상 종료 시 강제 리액션 패턴 — 디버그 토글 ON일 때만
		if (bNormalCompletion
			&& Data.Boss->bDebugForceReactionAfterDefense
			&& !Data.Boss->DebugReactionPatternName.IsNone())
		{
			UE_LOG(LogDesecration, Warning,
				TEXT("T3_ST: [DEBUG:ReactionTest] Roll 종료 → 강제 리액션 패턴 '%s' 실행"),
				*Data.Boss->DebugReactionPatternName.ToString());
			Data.Boss->ExecutePattern(Data.Boss->DebugReactionPatternName, true);
		}
	}

	Data.ActiveRoll = nullptr;
	Data.bRollEnded = false;
	Data.DelayElapsed = 0.f;
	Data.bAppliedScale = false;
}

// ============================================================
// Task: FT3STT_Block
// 막기 시퀀스 트리거 — Boss가 In→Loop(자기루프)→Out 흐름을 직접 관리
// EnterState: StartBlockSequence (Phase=In 진입)
// Tick: 종료 조건(시간/횟수) 충족 시 RequestEndBlockSequence (한 번만) → Phase=Idle 도달 시 Succeeded
// ExitState: 외부 인터럽트면 StopBlockSequence로 강제 정리
// ============================================================

EStateTreeRunStatus FT3STT_Block::EnterState(
	FStateTreeExecutionContext& Context,
	const FStateTreeTransitionResult& Transition) const
{
	FT3STT_BlockInstanceData& Data = Context.GetInstanceData(*this);

	if (!Data.Boss)
	{
		UE_LOG(LogDesecration, Warning, TEXT("T3_ST: Block — Boss가 바인딩되지 않음"));
		return EStateTreeRunStatus::Failed;
	}

	// 진입 상태 리셋
	Data.ElapsedTime = 0.f;
	Data.bEndRequested = false;

	// Boss 측 시퀀스 시작 — 태그 ON + InEntry 재생 + BlendingOut 콜백 등록 + BlockHitsCount=0
	Data.Boss->StartBlockSequence();
	Data.InitialHitsCount = Data.Boss->BlockHitsCount; // (보통 0 — 델타 측정용)

	UE_LOG(LogDesecration, Log,
		TEXT("T3_ST: Block 진입 (MaxDuration:%.2f, MaxHits:%d)"),
		Data.MaxDuration, Data.MaxBlockHits);

	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FT3STT_Block::Tick(
	FStateTreeExecutionContext& Context,
	const float DeltaTime) const
{
	FT3STT_BlockInstanceData& Data = Context.GetInstanceData(*this);

	if (!Data.Boss)
	{
		return EStateTreeRunStatus::Failed;
	}

	// 시퀀스가 자연 종료(Out 끝)되어 Idle 복귀 — STT Succeeded
	if (Data.Boss->CurrentBlockPhase == EBlockPhase::Idle)
	{
		UE_LOG(LogDesecration, Log,
			TEXT("T3_ST: Block 정상 종료 (Phase=Idle 도달, 경과:%.2f초, 막힘:%d회)"),
			Data.ElapsedTime, Data.Boss->BlockHitsCount);
		return EStateTreeRunStatus::Succeeded;
	}

	Data.ElapsedTime += DeltaTime;

	// 종료 조건 평가 — 한 번만 RequestEndBlockSequence 호출 (이후엔 Idle 도달 대기)
	if (!Data.bEndRequested)
	{
		const bool bDurationOver = (Data.ElapsedTime >= Data.MaxDuration);
		const bool bHitsOver = (Data.MaxBlockHits > 0 && Data.Boss->BlockHitsCount >= Data.MaxBlockHits);

		if (bDurationOver || bHitsOver)
		{
			UE_LOG(LogDesecration, Log,
				TEXT("T3_ST: Block 종료 요청 (시간초과:%d, 횟수초과:%d, 경과:%.2f초, 막힘:%d/%d)"),
				bDurationOver ? 1 : 0, bHitsOver ? 1 : 0,
				Data.ElapsedTime, Data.Boss->BlockHitsCount, Data.MaxBlockHits);

			Data.Boss->RequestEndBlockSequence();
			Data.bEndRequested = true;
		}
	}

	return EStateTreeRunStatus::Running;
}

void FT3STT_Block::ExitState(
	FStateTreeExecutionContext& Context,
	const FStateTreeTransitionResult& Transition) const
{
	FT3STT_BlockInstanceData& Data = Context.GetInstanceData(*this);

	if (Data.Boss)
	{
		// 정상 Succeeded 경로에선 Phase=Idle 도달 — 인터럽트면 그 외 단계
		const bool bNormalCompletion = (Data.Boss->CurrentBlockPhase == EBlockPhase::Idle);

		// 외부 인터럽트(BlockReaction 이벤트, 사망 등 ST 트랜지션)로 끊긴 경우 — 시퀀스 강제 정리
		// 정상 Succeeded 경로에선 이미 Phase=Idle이라 StopBlockSequence가 early-return
		if (!bNormalCompletion)
		{
			Data.Boss->StopBlockSequence();
		}

		// 정상 종료 시에만 리액션 트리거 세팅 — 인터럽트(BlockReaction → 빠른 반격) 시 제외
		if (bNormalCompletion)
		{
			Data.Boss->PendingReactionSource = EBossReactionSource::FromBlock;
			UE_LOG(LogDesecration, Log, TEXT("T3_ST: Block 정상 종료 → PendingReactionSource=FromBlock 세팅"));
		}

		// [DEBUG:ReactionTest] Block 정상 종료 시 강제 리액션 패턴 — 디버그 토글 ON일 때만
		if (bNormalCompletion
			&& Data.Boss->bDebugForceReactionAfterDefense
			&& !Data.Boss->DebugReactionPatternName.IsNone())
		{
			UE_LOG(LogDesecration, Warning,
				TEXT("T3_ST: [DEBUG:ReactionTest] Block 종료 → 강제 리액션 패턴 '%s' 실행"),
				*Data.Boss->DebugReactionPatternName.ToString());
			Data.Boss->ExecutePattern(Data.Boss->DebugReactionPatternName, true);
		}
	}

	Data.ElapsedTime = 0.f;
	Data.InitialHitsCount = 0;
	Data.bEndRequested = false;
}

// ============================================================
// Task: FT3STT_HandleDeath
// 사망 상태 — StateTree 정지, 사망 델리게이트 브로드캐스트
// ============================================================

EStateTreeRunStatus FT3STT_HandleDeath::EnterState(
	FStateTreeExecutionContext& Context,
	const FStateTreeTransitionResult& Transition) const
{
	FT3STT_HandleDeathInstanceData& Data = Context.GetInstanceData(*this);

	if (!Data.Boss)
	{
		UE_LOG(LogDesecration, Warning, TEXT("T3_ST: HandleDeath — Boss가 바인딩되지 않음"));
		return EStateTreeRunStatus::Failed;
	}

	UE_LOG(LogDesecration, Log, TEXT("T3_ST: HandleDeath — 사망 처리 시작"));

	// StateTree 중지 — 더 이상 AI 행동 불필요
	if (Data.Boss->StateTreeComponent)
	{
		Data.Boss->StateTreeComponent->StopLogic(TEXT("Boss Dead"));
	}

	return EStateTreeRunStatus::Succeeded;
}

// ============================================================
// Task: FT3STT_RunToAttackRange
// AddMovementInput 기반 돌진 — ABP Run 블렌드 자연스러움
// ============================================================

EStateTreeRunStatus FT3STT_RunToAttackRange::EnterState(
	FStateTreeExecutionContext& Context,
	const FStateTreeTransitionResult& Transition) const
{
	FT3STT_RunToAttackRangeInstanceData& Data = Context.GetInstanceData(*this);

	Data.ElapsedTime = 0.f;
	Data.CachedDefaultSpeed = 0.f;

	if (!Data.Boss)
	{
		UE_LOG(LogDesecration, Warning, TEXT("T3_ST: RunToAttackRange — Boss가 바인딩되지 않음"));
		return EStateTreeRunStatus::Failed;
	}

	if (!Data.Boss->CombatTarget)
	{
		UE_LOG(LogDesecration, Warning, TEXT("T3_ST: RunToAttackRange — CombatTarget이 없음"));
		return EStateTreeRunStatus::Failed;
	}

	// 이전 베이스 속도 캐시 — 종료 시 복원용 (Boss의 단일 진입점 사용)
	Data.CachedDefaultSpeed = Data.Boss->GetActiveBaseWalkSpeed();

	// SetFocus — 딜레이 동안 타겟 방향으로 회전
	if (AAIController* AIC = GetBossAIController(Data.Boss))
	{
		AIC->SetFocus(Data.Boss->CombatTarget);
	}

	UE_LOG(LogDesecration, Log,
		TEXT("T3_ST: RunToAttackRange 시작 (PreDelay:%.2f, DashSpeed:%.0f, ApproachDist:%.0f)"),
		Data.PreDashDelay, Data.DashSpeed, Data.ApproachDistance);

	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FT3STT_RunToAttackRange::Tick(
	FStateTreeExecutionContext& Context,
	const float DeltaTime) const
{
	FT3STT_RunToAttackRangeInstanceData& Data = Context.GetInstanceData(*this);

	if (!Data.Boss || !Data.Boss->CombatTarget)
	{
		return EStateTreeRunStatus::Failed;
	}

	Data.ElapsedTime += DeltaTime;

	// 딜레이 중 — 회전만 하고 대기
	if (Data.ElapsedTime < Data.PreDashDelay)
	{
		return EStateTreeRunStatus::Running;
	}

	// 딜레이 끝난 직후 — 돌진 시작 (1회만)
	if (Data.ElapsedTime - DeltaTime < Data.PreDashDelay)
	{
		// Dash 베이스 속도 푸시 — 천사 슬로우 곱은 Boss 측에서 자동 적용
		Data.Boss->SetActiveBaseWalkSpeed(Data.DashSpeed);
		if (Data.RunMontage)
		{
			// Run 몽타주도 헬퍼 통과 — 슬로우 진입/이탈 실시간 갱신 가능
			Data.Boss->PlayMoveMontageWithSlow(Data.RunMontage, 1.f);
		}
		UE_LOG(LogDesecration, Log, TEXT("T3_ST: RunToAttackRange — 딜레이 완료, 돌진 시작"));
	}

	// 타임아웃 체크 (딜레이 제외)
	const float DashElapsed = Data.ElapsedTime - Data.PreDashDelay;
	if (DashElapsed >= Data.Timeout)
	{
		UE_LOG(LogDesecration, Log,
			TEXT("T3_ST: RunToAttackRange 타임아웃 (%.1f초)"), Data.Timeout);
		return EStateTreeRunStatus::Failed;
	}

	// 타겟 방향으로 이동 입력
	const FVector MyLocation = Data.Boss->GetActorLocation();
	const FVector TargetLocation = Data.Boss->CombatTarget->GetActorLocation();
	const FVector Direction = (TargetLocation - MyLocation).GetSafeNormal2D();

	Data.Boss->AddMovementInput(Direction, 1.0f);

	// 거리 도달 체크
	const float Dist = FVector::Dist2D(MyLocation, TargetLocation);
	if (Dist <= Data.ApproachDistance)
	{
		UE_LOG(LogDesecration, Log,
			TEXT("T3_ST: RunToAttackRange 도달 (거리:%.0f ≤ %.0f, 소요:%.1f초)"),
			Dist, Data.ApproachDistance, Data.ElapsedTime);
		return EStateTreeRunStatus::Succeeded;
	}

	return EStateTreeRunStatus::Running;
}

void FT3STT_RunToAttackRange::ExitState(
	FStateTreeExecutionContext& Context,
	const FStateTreeTransitionResult& Transition) const
{
	FT3STT_RunToAttackRangeInstanceData& Data = Context.GetInstanceData(*this);

	if (Data.Boss)
	{
		// 런 몽타주 정지 (블렌드아웃으로 자연스럽게 복귀)
		if (Data.RunMontage)
		{
			Data.Boss->StopAnimMontage(Data.RunMontage);
		}

		// 베이스 속도 복원 — Boss의 단일 진입점 사용 (천사 슬로우와 자동 동기)
		if (Data.CachedDefaultSpeed > 0.f)
		{
			Data.Boss->SetActiveBaseWalkSpeed(Data.CachedDefaultSpeed);
			Data.CachedDefaultSpeed = 0.f;
		}
	}
}

// ============================================================
// Condition: FT3STC_PatternOffCooldown
// 패턴 쿨다운 체크 — true면 사용 가능, false면 쿨다운 중
// ============================================================

bool FT3STC_PatternOffCooldown::TestCondition(FStateTreeExecutionContext& Context) const
{
	const FT3STC_PatternOffCooldownInstanceData& Data = Context.GetInstanceData(*this);

	if (!Data.Boss)
	{
		UE_LOG(LogDesecration, Warning, TEXT("T3_ST: PatternOffCooldown — Boss가 바인딩되지 않음"));
		return false;
	}

	if (Data.PatternName.IsNone())
	{
		// 패턴 이름 미설정 시 항상 사용 가능으로 간주
		return true;
	}

	const bool bOffCooldown = Data.Boss->IsPatternOffCooldown(Data.PatternName);

	UE_LOG(LogDesecration, Verbose,
		TEXT("T3_ST: PatternOffCooldown('%s') = %s"),
		*Data.PatternName.ToString(),
		bOffCooldown ? TEXT("true") : TEXT("false"));

	return bOffCooldown;
}

// ============================================================
// Condition: FT3STC_PatternAvailableAtStage
// 패턴의 RequiredStage ≤ BossStage이면 true
// ============================================================

bool FT3STC_PatternAvailableAtStage::TestCondition(FStateTreeExecutionContext& Context) const
{
	const FT3STC_PatternAvailableAtStageInstanceData& Data = Context.GetInstanceData(*this);

	if (!Data.Boss)
	{
		UE_LOG(LogDesecration, Warning, TEXT("T3_ST: PatternAvailableAtStage — Boss가 바인딩되지 않음"));
		return false;
	}

	if (Data.PatternName.IsNone())
	{
		// 패턴 이름 미설정 시 항상 사용 가능으로 간주
		return true;
	}

	const FMidBossAttackPattern* PatternData = Data.Boss->FindPatternData(Data.PatternName);
	if (!PatternData)
	{
		UE_LOG(LogDesecration, Warning, TEXT("T3_ST: PatternAvailableAtStage — 패턴 '%s' 데이터 없음"),
			*Data.PatternName.ToString());
		return false;
	}

	const bool bAvailable = Data.Boss->BossStage >= PatternData->RequiredStage;

	UE_LOG(LogDesecration, Verbose,
		TEXT("T3_ST: PatternAvailableAtStage('%s') — BossStage:%d >= Required:%d = %s"),
		*Data.PatternName.ToString(),
		Data.Boss->BossStage,
		PatternData->RequiredStage,
		bAvailable ? TEXT("true") : TEXT("false"));

	return bAvailable;
}

// ============================================================
// Condition: FT3STC_DistanceToTarget
// 타겟과의 거리 비교 — 평가 시점에만 계산 (폴링 없음)
// ============================================================

bool FT3STC_DistanceToTarget::TestCondition(FStateTreeExecutionContext& Context) const
{
	const FT3STC_DistanceToTargetInstanceData& Data = Context.GetInstanceData(*this);

	if (!Data.Boss)
	{
		UE_LOG(LogDesecration, Warning, TEXT("T3_ST: DistanceToTarget — Boss가 바인딩되지 않음"));
		return false;
	}

	if (!Data.Boss->CombatTarget)
	{
		UE_LOG(LogDesecration, Warning, TEXT("T3_ST: DistanceToTarget — CombatTarget이 없음"));
		return false;
	}

	const float Distance = FVector::Dist(
		Data.Boss->GetActorLocation(),
		Data.Boss->CombatTarget->GetActorLocation());

	bool bResult = false;
	switch (Data.CompareOp)
	{
	case EDistanceCompareOp::LessOrEqual:
		bResult = Distance <= Data.Threshold;
		break;
	case EDistanceCompareOp::GreaterThan:
		bResult = Distance > Data.Threshold;
		break;
	}

	UE_LOG(LogDesecration, Verbose,
		TEXT("T3_ST: DistanceToTarget(%.0f %s %.0f) = %s"),
		Distance,
		Data.CompareOp == EDistanceCompareOp::LessOrEqual ? TEXT("<=") : TEXT(">"),
		Data.Threshold,
		bResult ? TEXT("true") : TEXT("false"));

	return bResult;
}

// ============================================================
// Consideration: FT3Consideration_DisengageUrge
// ActionCount 기반 — 패턴 많이 할수록 Disengage 확률 상승
// ============================================================

float FT3Consideration_DisengageUrge::GetScore(FStateTreeExecutionContext& Context) const
{
	const FT3Consideration_DisengageUrgeInstanceData& Data = Context.GetInstanceData(*this);

	if (!Data.Boss || Data.MaxUrge <= 0)
	{
		return 0.f;
	}

	// 스테이지별 카운트 × 기여도 합산
	const int32 TotalUrge =
		Data.Boss->StagePatternCounts[0] * Data.Stage1UrgeCost +
		Data.Boss->StagePatternCounts[1] * Data.Stage2UrgeCost +
		Data.Boss->StagePatternCounts[2] * Data.Stage3UrgeCost;

	const float Ratio = static_cast<float>(TotalUrge) / static_cast<float>(Data.MaxUrge);
	return FMath::Clamp(Ratio, Data.MinScore, 1.f);
}

// ============================================================
// Consideration: FT3Consideration_PatternOffCooldown
// 쿨다운 중 → 0.0 (선택 제외), 사용 가능 → 1.0
// ============================================================

float FT3Consideration_PatternOffCooldown::GetScore(FStateTreeExecutionContext& Context) const
{
	const FT3Consideration_PatternOffCooldownInstanceData& Data = Context.GetInstanceData(*this);

	if (!Data.Boss || Data.PatternName.IsNone())
	{
		return 0.f;
	}

	return Data.Boss->IsPatternOffCooldown(Data.PatternName) ? 1.f : 0.f;
}

// ============================================================
// Consideration: FT3Consideration_PatternAvailableAtStage
// 스테이지 미달 → 0.0 (선택 제외), 사용 가능 → 1.0
// ============================================================

float FT3Consideration_PatternAvailableAtStage::GetScore(FStateTreeExecutionContext& Context) const
{
	const FT3Consideration_PatternAvailableAtStageInstanceData& Data = Context.GetInstanceData(*this);

	if (!Data.Boss)
	{
		return 0.f;
	}

	if (Data.PatternName.IsNone())
	{
		return 1.f;
	}

	const FMidBossAttackPattern* PatternData = Data.Boss->FindPatternData(Data.PatternName);
	if (!PatternData)
	{
		return 0.f;
	}

	return Data.Boss->BossStage >= PatternData->RequiredStage ? 1.f : 0.f;
}

// ============================================================
// Consideration: FT3Consideration_ConsecutiveDisengagePenalty
// 연속 Disengage 시 점수 감쇄 — PenaltyPerCount ^ ConsecutiveDisengageCount
// ============================================================

float FT3Consideration_ConsecutiveDisengagePenalty::GetScore(FStateTreeExecutionContext& Context) const
{
	const FT3Consideration_ConsecutiveDisengagePenaltyInstanceData& Data = Context.GetInstanceData(*this);

	if (!Data.Boss)
	{
		return 0.f;
	}

	const int32 Count = Data.Boss->ConsecutiveDisengageCount;
	if (Count <= 0)
	{
		return 1.f;
	}

	// PenaltyPerCount ^ Count (0.5^1=0.5, 0.5^2=0.25 ...)
	return FMath::Pow(Data.PenaltyPerCount, static_cast<float>(Count));
}

// ============================================================
// Consideration: FT3Consideration_ReactionWindow
// bAllowAsReaction=true 패턴이 PostBlock/PostRoll 윈도우에서만 강하게 가중되도록 부풀림.
// (※ ParryWindow 카운터 패턴과 무관)
// ============================================================

float FT3Consideration_ReactionWindow::GetScore(FStateTreeExecutionContext& Context) const
{
	const FT3Consideration_ReactionWindowInstanceData& Data = Context.GetInstanceData(*this);

	if (!Data.Boss || Data.PatternName.IsNone())
	{
		// Boss/패턴 미바인딩 시 무영향 — 다른 Consideration이 판정
		return 1.f;
	}

	const FMidBossAttackPattern* PatternData = Data.Boss->FindPatternData(Data.PatternName);
	if (!PatternData)
	{
		return 1.f;
	}

	// 리액션 후보가 아닌 일반 패턴은 무영향
	if (!PatternData->bAllowAsReaction)
	{
		return 1.f;
	}

	// 리액션 후보 — PendingReactionSource로 분기 (Roll/Block 직후 1회만 활성)
	const bool bPostBlockActive = Data.bRespondToPostBlock
		&& Data.Boss->PendingReactionSource == EBossReactionSource::FromBlock;
	const bool bPostRollActive = Data.bRespondToPostRoll
		&& Data.Boss->PendingReactionSource == EBossReactionSource::FromRoll;

	return (bPostBlockActive || bPostRollActive) ? Data.BoostScore : Data.IdleScore;
}

// ============================================================
// Consideration: FT3Consideration_GaugePressure
// 스턴 게이지 ratio = Clamp(CurrentStunGauge / StunThreshold, 0, 1)
// bInverse=true:  점수 = MinScore + (1 - MinScore) × (1 - ratio^Exponent)
// bInverse=false: 점수 = MinScore + (1 - MinScore) × ratio^Exponent
// ============================================================

float FT3Consideration_GaugePressure::GetScore(FStateTreeExecutionContext& Context) const
{
	const FT3Consideration_GaugePressureInstanceData& Data = Context.GetInstanceData(*this);

	if (!Data.Boss)
	{
		return 0.f;
	}

	const float Threshold = Data.Boss->MidBossStats.StunThreshold;
	if (Threshold <= KINDA_SMALL_NUMBER)
	{
		// 임계치 미설정 — 영향 없게 1.0 반환 (다른 Consideration 판정에 맡김)
		return 1.f;
	}

	const float Ratio = FMath::Clamp(Data.Boss->MidBossStats.CurrentStunGauge / Threshold, 0.f, 1.f);
	const float Curve = FMath::Pow(Ratio, Data.Exponent);
	const float NormalizedScore = Data.bInverse ? (1.f - Curve) : Curve;

	const float MinClamped = FMath::Clamp(Data.MinScore, 0.f, 1.f);
	return MinClamped + (1.f - MinClamped) * NormalizedScore;
}
