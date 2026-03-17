#include "Monster/T3MidBossSTNodes.h"
#include "Desecration.h"
#include "Monster/T3MidBossMonster.h"
#include "AIController.h"
#include "Navigation/PathFollowingComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "StateTreeExecutionContext.h"

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

	// 패턴 실행 시도
	if (!Data.Boss->ExecutePattern(Data.PatternName))
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

	// 스테이지별 패턴 카운트 리셋
	UE_LOG(LogDesecration, Log, TEXT("T3_ST: Disengage — StagePatternCounts 리셋 [%d,%d,%d → 0]"),
		Data.Boss->StagePatternCounts[0], Data.Boss->StagePatternCounts[1], Data.Boss->StagePatternCounts[2]);
	Data.Boss->StagePatternCounts[0] = 0;
	Data.Boss->StagePatternCounts[1] = 0;
	Data.Boss->StagePatternCounts[2] = 0;

	// 횡이동 방향 랜덤 결정
	if (Data.bStrafe)
	{
		Data.StrafeDirection = FMath::RandBool() ? 1.f : -1.f;
	}

	// Strafe 속도 적용
	if (Data.StrafeSpeed > 0.f)
	{
		if (UCharacterMovementComponent* MoveComp = Data.Boss->GetCharacterMovement())
		{
			Data.CachedDefaultSpeed = MoveComp->MaxWalkSpeed;
			MoveComp->MaxWalkSpeed = Data.StrafeSpeed;
		}
	}

	// 루트모션 거리 스케일 적용
	if (Data.RootMotionScale != 1.0f)
	{
		Data.Boss->SetAnimRootMotionTranslationScale(Data.RootMotionScale);
	}

	// 백스텝 몽타주 재생
	if (Data.bBackStep && Data.BackStepMontage)
	{
		Data.Boss->PlayAnimMontage(Data.BackStepMontage);
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
		// 백스텝 몽타주 정지
		if (Data.bBackStep && Data.BackStepMontage)
		{
			Data.Boss->StopAnimMontage(Data.BackStepMontage);
		}

		// 루트모션 스케일 복원
		if (Data.RootMotionScale != 1.0f)
		{
			Data.Boss->SetAnimRootMotionTranslationScale(1.0f);
		}

		// Strafe 속도 복원 — Failed/Succeeded 모두 안전하게 복원
		if (Data.CachedDefaultSpeed > 0.f)
		{
			if (UCharacterMovementComponent* MoveComp = Data.Boss->GetCharacterMovement())
			{
				MoveComp->MaxWalkSpeed = Data.CachedDefaultSpeed;
			}
			Data.CachedDefaultSpeed = 0.f;
		}
	}
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

	// MaxWalkSpeed 캐시
	if (UCharacterMovementComponent* MoveComp = Data.Boss->GetCharacterMovement())
	{
		Data.CachedDefaultSpeed = MoveComp->MaxWalkSpeed;
	}

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
		if (UCharacterMovementComponent* MoveComp = Data.Boss->GetCharacterMovement())
		{
			MoveComp->MaxWalkSpeed = Data.DashSpeed;
		}
		if (Data.RunMontage)
		{
			Data.Boss->PlayAnimMontage(Data.RunMontage);
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

		// MaxWalkSpeed 복원
		if (Data.CachedDefaultSpeed > 0.f)
		{
			if (UCharacterMovementComponent* MoveComp = Data.Boss->GetCharacterMovement())
			{
				MoveComp->MaxWalkSpeed = Data.CachedDefaultSpeed;
			}
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
