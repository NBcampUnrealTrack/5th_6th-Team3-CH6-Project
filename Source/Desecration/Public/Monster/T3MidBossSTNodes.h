// T3MidBossSTNodes.h
// StateTree 커스텀 노드 — Mid-Boss (Evaluator 1개 + Task 6개 + Condition 3개 + Consideration 2개)

#pragma once

#include "CoreMinimal.h"
#include "StateTreeTaskBase.h"
#include "StateTreeEvaluatorBase.h"
#include "StateTreeConditionBase.h"
#include "StateTreeConsiderationBase.h"
#include "T3MidBossSTNodes.generated.h"

class AT3MidBossMonster;

// ============================================================
// Evaluator: FT3STE_MidBossCombat
// 이벤트 기반으로 전환됨 — 폴링 제거
// - Dead/Stunned 트랜지션: SendStateTreeEvent()로 처리
// - Distance/ActionCount: 필요 시점에 Task에서 직접 Boss 변수 읽기
// - 변수는 하위 호환용으로 유지 (TreeStart에서 초기값만 설정)
// ============================================================

USTRUCT()
struct FT3STE_MidBossCombatInstanceData
{
	GENERATED_BODY()

	// 입력 — 컨텍스트에서 바인딩 (Boss 포인터를 하위 노드에 전달)
	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<AT3MidBossMonster> Boss = nullptr;
};

USTRUCT(meta = (DisplayName = "Mid-Boss Combat Evaluator"))
struct DESECRATION_API FT3STE_MidBossCombat : public FStateTreeEvaluatorCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FT3STE_MidBossCombatInstanceData;

	virtual const UStruct* GetInstanceDataType() const override
	{
		return FT3STE_MidBossCombatInstanceData::StaticStruct();
	}

	virtual void TreeStart(FStateTreeExecutionContext& Context) const override;
};

// ============================================================
// Task: FT3STT_ExecutePattern
// 범용 패턴 실행 — PatternName만 바꿔서 모든 패턴 커버
// ============================================================

USTRUCT()
struct FT3STT_ExecutePatternInstanceData
{
	GENERATED_BODY()

	// 파라미터 — 에디터에서 설정
	UPROPERTY(EditAnywhere, Category = "Parameter")
	FName PatternName = NAME_None;

	// 입력 — 컨텍스트에서 바인딩
	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<AT3MidBossMonster> Boss = nullptr;

	// 내부 상태
	UPROPERTY()
	bool bPatternStarted = false;

	UPROPERTY()
	int32 CachedActionCountCost = 1;

	UPROPERTY()
	int32 CachedRequiredStage = 1;

	// 델리게이트 기반 완료 감지 (non-UPROPERTY, 런타임 전용)
	bool bPatternCompleted = false;
	FDelegateHandle PatternCompletedHandle;
};

USTRUCT(meta = (DisplayName = "Execute Pattern"))
struct DESECRATION_API FT3STT_ExecutePattern : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FT3STT_ExecutePatternInstanceData;

	virtual const UStruct* GetInstanceDataType() const override
	{
		return FT3STT_ExecutePatternInstanceData::StaticStruct();
	}

	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context,
		const FStateTreeTransitionResult& Transition) const override;
	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context,
		const float DeltaTime) const override;
	virtual void ExitState(FStateTreeExecutionContext& Context,
		const FStateTreeTransitionResult& Transition) const override;
};

// ============================================================
// Task: FT3STT_ApproachTarget
// AddMovementInput으로 타겟 접근 (NavMesh 불필요)
// ============================================================

USTRUCT()
struct FT3STT_ApproachTargetInstanceData
{
	GENERATED_BODY()

	// 파라미터 — 에디터에서 설정
	UPROPERTY(EditAnywhere, Category = "Parameter")
	float AcceptableRadius = 300.f;

	UPROPERTY(EditAnywhere, Category = "Parameter")
	int32 ActionCountReset = 3;

	UPROPERTY(EditAnywhere, Category = "Parameter")
	bool bResetActionCount = true;

	UPROPERTY(EditAnywhere, Category = "Parameter")
	float PostArrivalDelay = 2.5f;

	// 최대 접근 시간 — 초과 시 Failed 반환하여 패턴 재선택 유도
	UPROPERTY(EditAnywhere, Category = "Parameter")
	float Timeout = 8.f;

	// 입력 — 컨텍스트에서 바인딩
	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<AT3MidBossMonster> Boss = nullptr;

	// 내부 상태
	UPROPERTY()
	bool bArrived = false;

	UPROPERTY()
	float DelayElapsed = 0.f;

	UPROPERTY()
	float ElapsedTime = 0.f;
};

USTRUCT(meta = (DisplayName = "Approach Target"))
struct DESECRATION_API FT3STT_ApproachTarget : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FT3STT_ApproachTargetInstanceData;

	virtual const UStruct* GetInstanceDataType() const override
	{
		return FT3STT_ApproachTargetInstanceData::StaticStruct();
	}

	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context,
		const FStateTreeTransitionResult& Transition) const override;
	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context,
		const float DeltaTime) const override;
	virtual void ExitState(FStateTreeExecutionContext& Context,
		const FStateTreeTransitionResult& Transition) const override;
};

// ============================================================
// Task: FT3STT_WaitForStunEnd
// 이벤트 기반 — Boss.Event.StunRecovered로 상태 전환
// ============================================================

USTRUCT()
struct FT3STT_WaitForStunEndInstanceData
{
	GENERATED_BODY()

	// 입력 — 컨텍스트에서 바인딩
	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<AT3MidBossMonster> Boss = nullptr;

	// 이벤트 기반 전환 — 델리게이트 필드 제거됨
};

USTRUCT(meta = (DisplayName = "Wait For Stun End"))
struct DESECRATION_API FT3STT_WaitForStunEnd : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FT3STT_WaitForStunEndInstanceData;

	virtual const UStruct* GetInstanceDataType() const override
	{
		return FT3STT_WaitForStunEndInstanceData::StaticStruct();
	}

	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context,
		const FStateTreeTransitionResult& Transition) const override;
	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context,
		const float DeltaTime) const override;
	virtual void ExitState(FStateTreeExecutionContext& Context,
		const FStateTreeTransitionResult& Transition) const override;
};

// ============================================================
// Task: FT3STT_Disengage
// 이탈 행동 (제자리 대기 / 횡이동 / 백스텝)
// ============================================================

USTRUCT()
struct FT3STT_DisengageInstanceData
{
	GENERATED_BODY()

	// 파라미터
	UPROPERTY(EditAnywhere, Category = "Parameter")
	float Duration = 3.0f;

	// 횡이동 — Duration 동안 타겟 주위를 좌/우 랜덤 이동
	UPROPERTY(EditAnywhere, Category = "Parameter")
	bool bStrafe = false;

	// 백스텝 — 타겟 반대 방향으로 후퇴
	UPROPERTY(EditAnywhere, Category = "Parameter")
	bool bBackStep = false;

	// 백스텝 몽타주 (선택) — 없으면 슬라이드 이동만
	UPROPERTY(EditAnywhere, Category = "Parameter")
	TObjectPtr<UAnimMontage> BackStepMontage = nullptr;

	// 루트모션 거리 스케일 (1.0 = 원본, 0.5 = 절반 거리, 0 = 이동 없음)
	UPROPERTY(EditAnywhere, Category = "Parameter", meta = (ClampMin = "0.0", ClampMax = "2.0"))
	float RootMotionScale = 1.0f;

	// Strafe 시 이동 속도 (0 = 기본 MaxWalkSpeed 사용)
	UPROPERTY(EditAnywhere, Category = "Parameter", meta = (ClampMin = "0.0"))
	float StrafeSpeed = 0.f;

	// 입력 — 컨텍스트에서 바인딩
	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<AT3MidBossMonster> Boss = nullptr;

	// 내부 상태
	UPROPERTY()
	float ElapsedTime = 0.f;

	UPROPERTY()
	float StrafeDirection = 1.f;

	UPROPERTY()
	float CachedDefaultSpeed = 0.f;
};

USTRUCT(meta = (DisplayName = "Disengage"))
struct DESECRATION_API FT3STT_Disengage : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FT3STT_DisengageInstanceData;

	virtual const UStruct* GetInstanceDataType() const override
	{
		return FT3STT_DisengageInstanceData::StaticStruct();
	}

	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context,
		const FStateTreeTransitionResult& Transition) const override;
	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context,
		const float DeltaTime) const override;
	virtual void ExitState(FStateTreeExecutionContext& Context,
		const FStateTreeTransitionResult& Transition) const override;
};

// ============================================================
// Task: FT3STT_HandleDeath
// 사망 상태 — StateTree 정지
// ============================================================

USTRUCT()
struct FT3STT_HandleDeathInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<AT3MidBossMonster> Boss = nullptr;
};

USTRUCT(meta = (DisplayName = "Handle Death"))
struct DESECRATION_API FT3STT_HandleDeath : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FT3STT_HandleDeathInstanceData;

	virtual const UStruct* GetInstanceDataType() const override
	{
		return FT3STT_HandleDeathInstanceData::StaticStruct();
	}

	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context,
		const FStateTreeTransitionResult& Transition) const override;
};

// ============================================================
// Task: FT3STT_RunToAttackRange
// AddMovementInput으로 타겟에 돌진 (NavMesh 불필요, ABP Run 재생)
// 도달 시 Succeeded, 타임아웃 시 Failed
// ============================================================

USTRUCT()
struct FT3STT_RunToAttackRangeInstanceData
{
	GENERATED_BODY()

	// 파라미터 — 에디터에서 설정
	UPROPERTY(EditAnywhere, Category = "Parameter")
	float ApproachDistance = 300.f;

	UPROPERTY(EditAnywhere, Category = "Parameter")
	float DashSpeed = 600.f;

	UPROPERTY(EditAnywhere, Category = "Parameter")
	float Timeout = 5.f;

	// 돌진 중 재생할 런 몽타주 (InPlace 권장 — 이동은 AddMovementInput 담당)
	UPROPERTY(EditAnywhere, Category = "Parameter")
	TObjectPtr<UAnimMontage> RunMontage = nullptr;

	// 돌진 전 회전 대기 (SetFocus 후 몸 돌릴 시간)
	UPROPERTY(EditAnywhere, Category = "Parameter")
	float PreDashDelay = 0.3f;

	// 입력 — 컨텍스트에서 바인딩
	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<AT3MidBossMonster> Boss = nullptr;

	// 내부 상태
	UPROPERTY()
	float CachedDefaultSpeed = 0.f;

	UPROPERTY()
	float ElapsedTime = 0.f;
};

USTRUCT(meta = (DisplayName = "Run To Attack Range"))
struct DESECRATION_API FT3STT_RunToAttackRange : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FT3STT_RunToAttackRangeInstanceData;

	virtual const UStruct* GetInstanceDataType() const override
	{
		return FT3STT_RunToAttackRangeInstanceData::StaticStruct();
	}

	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context,
		const FStateTreeTransitionResult& Transition) const override;
	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context,
		const float DeltaTime) const override;
	virtual void ExitState(FStateTreeExecutionContext& Context,
		const FStateTreeTransitionResult& Transition) const override;
};

// ============================================================
// Condition: FT3STC_PatternOffCooldown
// 패턴 쿨다운 체크 — EnterCondition에서 사용
// 쿨다운 중이면 false → RandomWeighted에서 후보 제외
// ============================================================

USTRUCT()
struct FT3STC_PatternOffCooldownInstanceData
{
	GENERATED_BODY()

	// 파라미터 — 에디터에서 패턴 이름 설정
	UPROPERTY(EditAnywhere, Category = "Parameter")
	FName PatternName = NAME_None;

	// 입력 — 컨텍스트에서 바인딩
	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<AT3MidBossMonster> Boss = nullptr;
};

USTRUCT(meta = (DisplayName = "Pattern Off Cooldown"))
struct DESECRATION_API FT3STC_PatternOffCooldown : public FStateTreeConditionCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FT3STC_PatternOffCooldownInstanceData;

	virtual const UStruct* GetInstanceDataType() const override
	{
		return FT3STC_PatternOffCooldownInstanceData::StaticStruct();
	}

	virtual bool TestCondition(FStateTreeExecutionContext& Context) const override;
};

// ============================================================
// Condition: FT3STC_PatternAvailableAtStage
// 패턴의 RequiredStage ≤ Boss의 BossStage인지 체크
// 맵별 스테이지에 따라 패턴 사용 가능 여부 필터링
// ============================================================

USTRUCT()
struct FT3STC_PatternAvailableAtStageInstanceData
{
	GENERATED_BODY()

	// 파라미터 — 에디터에서 패턴 이름 설정
	UPROPERTY(EditAnywhere, Category = "Parameter")
	FName PatternName = NAME_None;

	// 입력 — 컨텍스트에서 바인딩
	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<AT3MidBossMonster> Boss = nullptr;
};

USTRUCT(meta = (DisplayName = "Pattern Available At Stage"))
struct DESECRATION_API FT3STC_PatternAvailableAtStage : public FStateTreeConditionCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FT3STC_PatternAvailableAtStageInstanceData;

	virtual const UStruct* GetInstanceDataType() const override
	{
		return FT3STC_PatternAvailableAtStageInstanceData::StaticStruct();
	}

	virtual bool TestCondition(FStateTreeExecutionContext& Context) const override;
};

// ============================================================
// Condition: FT3STC_DistanceToTarget
// 타겟과의 거리 비교 — EnterCondition 평가 시점에만 계산
// ============================================================

UENUM(BlueprintType)
enum class EDistanceCompareOp : uint8
{
	LessOrEqual		UMETA(DisplayName = "≤"),
	GreaterThan		UMETA(DisplayName = ">")
};

USTRUCT()
struct FT3STC_DistanceToTargetInstanceData
{
	GENERATED_BODY()

	// 파라미터 — 에디터에서 설정
	UPROPERTY(EditAnywhere, Category = "Parameter")
	float Threshold = 300.f;

	UPROPERTY(EditAnywhere, Category = "Parameter")
	EDistanceCompareOp CompareOp = EDistanceCompareOp::LessOrEqual;

	// 입력 — 컨텍스트에서 바인딩
	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<AT3MidBossMonster> Boss = nullptr;
};

USTRUCT(meta = (DisplayName = "Distance To Target"))
struct DESECRATION_API FT3STC_DistanceToTarget : public FStateTreeConditionCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FT3STC_DistanceToTargetInstanceData;

	virtual const UStruct* GetInstanceDataType() const override
	{
		return FT3STC_DistanceToTargetInstanceData::StaticStruct();
	}

	virtual bool TestCondition(FStateTreeExecutionContext& Context) const override;
};

// ============================================================
// Consideration: FT3Consideration_DisengageUrge
// ActionCount가 줄어들수록 점수 증가 (패턴 많이 할수록 Disengage 확률 상승)
// 점수 = Clamp(1.0 - ActionCount / MaxActionCount, 0, 1)
// ============================================================

USTRUCT()
struct FT3Consideration_DisengageUrgeInstanceData
{
	GENERATED_BODY()

	// 스테이지별 기여도 (패턴 1회당)
	UPROPERTY(EditAnywhere, Category = "Parameter")
	int32 Stage1UrgeCost = 1;

	UPROPERTY(EditAnywhere, Category = "Parameter")
	int32 Stage2UrgeCost = 2;

	UPROPERTY(EditAnywhere, Category = "Parameter")
	int32 Stage3UrgeCost = 3;

	// 이 값에 도달하면 점수 1.0
	UPROPERTY(EditAnywhere, Category = "Parameter")
	int32 MaxUrge = 10;

	// 최소 점수 (0이어도 이 확률은 유지)
	UPROPERTY(EditAnywhere, Category = "Parameter")
	float MinScore = 0.05f;

	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<AT3MidBossMonster> Boss = nullptr;
};

USTRUCT(meta = (DisplayName = "Disengage Urge (Consideration)"))
struct DESECRATION_API FT3Consideration_DisengageUrge : public FStateTreeConsiderationCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FT3Consideration_DisengageUrgeInstanceData;

	virtual const UStruct* GetInstanceDataType() const override
	{
		return FT3Consideration_DisengageUrgeInstanceData::StaticStruct();
	}

protected:
	virtual float GetScore(FStateTreeExecutionContext& Context) const override;
};

// ============================================================
// Consideration: FT3Consideration_PatternOffCooldown
// 쿨다운 중이면 0.0 → 랜덤 풀에서 제외
// 사용 가능하면 1.0 → Weight 기반 랜덤 참여
// ============================================================

USTRUCT()
struct FT3Consideration_PatternOffCooldownInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Parameter")
	FName PatternName = NAME_None;

	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<AT3MidBossMonster> Boss = nullptr;
};

USTRUCT(meta = (DisplayName = "Pattern Off Cooldown (Consideration)"))
struct DESECRATION_API FT3Consideration_PatternOffCooldown : public FStateTreeConsiderationCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FT3Consideration_PatternOffCooldownInstanceData;

	virtual const UStruct* GetInstanceDataType() const override
	{
		return FT3Consideration_PatternOffCooldownInstanceData::StaticStruct();
	}

protected:
	virtual float GetScore(FStateTreeExecutionContext& Context) const override;
};

// ============================================================
// Consideration: FT3Consideration_PatternAvailableAtStage
// 스테이지 미달이면 0.0 → 랜덤 풀에서 제외
// 사용 가능하면 1.0 → Weight 기반 랜덤 참여
// ============================================================

USTRUCT()
struct FT3Consideration_PatternAvailableAtStageInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Parameter")
	FName PatternName = NAME_None;

	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<AT3MidBossMonster> Boss = nullptr;
};

USTRUCT(meta = (DisplayName = "Pattern Available At Stage (Consideration)"))
struct DESECRATION_API FT3Consideration_PatternAvailableAtStage : public FStateTreeConsiderationCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FT3Consideration_PatternAvailableAtStageInstanceData;

	virtual const UStruct* GetInstanceDataType() const override
	{
		return FT3Consideration_PatternAvailableAtStageInstanceData::StaticStruct();
	}

protected:
	virtual float GetScore(FStateTreeExecutionContext& Context) const override;
};

// ============================================================
// Consideration: FT3Consideration_ConsecutiveDisengagePenalty
// 연속 Disengage 시 점수 감쇄 — PenaltyPerCount ^ ConsecutiveDisengageCount
// (0회=1.0, 1회=0.5, 2회=0.25 ...)
// ============================================================

USTRUCT()
struct FT3Consideration_ConsecutiveDisengagePenaltyInstanceData
{
	GENERATED_BODY()

	// 연속 1회당 곱해지는 배율 (0.5 = 매회 절반)
	UPROPERTY(EditAnywhere, Category = "Parameter", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float PenaltyPerCount = 0.5f;

	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<AT3MidBossMonster> Boss = nullptr;
};

USTRUCT(meta = (DisplayName = "Consecutive Disengage Penalty (Consideration)"))
struct DESECRATION_API FT3Consideration_ConsecutiveDisengagePenalty : public FStateTreeConsiderationCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FT3Consideration_ConsecutiveDisengagePenaltyInstanceData;

	virtual const UStruct* GetInstanceDataType() const override
	{
		return FT3Consideration_ConsecutiveDisengagePenaltyInstanceData::StaticStruct();
	}

protected:
	virtual float GetScore(FStateTreeExecutionContext& Context) const override;
};
