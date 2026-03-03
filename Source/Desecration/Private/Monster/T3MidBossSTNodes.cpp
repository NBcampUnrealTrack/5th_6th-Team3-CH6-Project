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

	// ActionCountCost 캐시
	FMidBossAttackPattern PatternData;
	if (Data.Boss->GetPatternData(Data.PatternName, PatternData))
	{
		Data.CachedActionCountCost = PatternData.ActionCountCost;
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
		UE_LOG(LogDesecration, Log,
			TEXT("T3_ST: ExecutePattern 완료 — '%s' (ActionCount: %d)"),
			*Data.PatternName.ToString(), Data.Boss->ActionCount);

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

	if (!Data.Boss)
	{
		UE_LOG(LogDesecration, Warning, TEXT("T3_ST: Disengage — Boss가 바인딩되지 않음"));
		return EStateTreeRunStatus::Failed;
	}

	// 횡이동 시 랜덤 좌/우 결정 + NavMesh 이동 요청
	if (Data.bStrafe && Data.Boss->CombatTarget)
	{
		Data.StrafeDirection = FMath::RandBool() ? 1.f : -1.f;

		// 타겟 기준 측면 위치 계산
		const FVector ToTarget = (Data.Boss->CombatTarget->GetActorLocation()
			- Data.Boss->GetActorLocation()).GetSafeNormal2D();
		const FVector RightVec = FVector::CrossProduct(FVector::UpVector, ToTarget);
		const FVector StrafeTarget = Data.Boss->GetActorLocation()
			+ RightVec * Data.StrafeDirection * 400.f;

		AAIController* AIC = GetBossAIController(Data.Boss);
		if (AIC)
		{
			AIC->MoveToLocation(StrafeTarget, 50.f);
		}
	}

	UE_LOG(LogDesecration, Log,
		TEXT("T3_ST: Disengage 시작 (Duration:%.1f, Strafe:%s)"),
		Data.Duration, Data.bStrafe ? TEXT("true") : TEXT("false"));

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

	// 회전은 MovementComponent가 SetFocus로 처리
	if (Data.ElapsedTime >= Data.Duration)
	{
		return EStateTreeRunStatus::Succeeded;
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
		AAIController* AIC = GetBossAIController(Data.Boss);
		if (AIC)
		{
			AIC->StopMovement();
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

	// MaxWalkSpeed 캐시 → DashSpeed로 변경
	if (UCharacterMovementComponent* MoveComp = Data.Boss->GetCharacterMovement())
	{
		Data.CachedDefaultSpeed = MoveComp->MaxWalkSpeed;
		MoveComp->MaxWalkSpeed = Data.DashSpeed;
	}

	// 런 몽타주 재생 (ABP 스테이트 머신 위에서 블렌드)
	if (Data.RunMontage)
	{
		Data.Boss->PlayAnimMontage(Data.RunMontage);
	}

	UE_LOG(LogDesecration, Log,
		TEXT("T3_ST: RunToAttackRange 시작 (DashSpeed:%.0f, ApproachDist:%.0f, Timeout:%.1f)"),
		Data.DashSpeed, Data.ApproachDistance, Data.Timeout);

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

	// 타임아웃 체크 (무한 추적 방지)
	if (Data.ElapsedTime >= Data.Timeout)
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
