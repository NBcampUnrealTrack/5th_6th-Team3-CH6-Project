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

	// true면 리액션 모드로 실행 — bAllowAsReaction 패턴의 ReactionStartSectionOverride 사용 (PostBlock/PostRoll 분기용)
	// (※ ParryWindow 카운터 패턴과 무관)
	UPROPERTY(EditAnywhere, Category = "Parameter")
	bool bAsReaction = false;

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
	// 몽타주는 AT3MidBossMonster::BackStepMontage 참조 (보스별 스켈레톤 대응)
	UPROPERTY(EditAnywhere, Category = "Parameter")
	bool bBackStep = false;

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
// Task: FT3STT_TestRoll
// 임시 데모용 — 8방향 회피 모션 시각 검증 (Desmond 프로토타입)
// 인덱스 순서: 0=0° / 1=45° / 2=90° / 3=135° / 4=180° / 5=225° / 6=270° / 7=315°
//              (Forward / FR / R / BR / Backward / BL / L / FL)
// 실제 회피 시스템(i-frame, 빈도 제어, 후속 공격)은 별도 구현 예정
// ============================================================

USTRUCT()
struct FT3STT_TestRollInstanceData
{
	GENERATED_BODY()

	// 8방향 몽타주 / 방향별 루트모션 배율은 Boss->RollMontages_8Dir / Boss->RollDirectionScales로 이동됨
	// (BackStepMontage와 동일 패턴 — 캐릭터 애셋은 Boss 소유, Task는 참조만)

	// true면 8방향 중 랜덤 선택 / false면 FixedDirectionYaw 사용
	UPROPERTY(EditAnywhere, Category = "Parameter")
	bool bRandomDirection = true;

	// 보스 정면 기준 각도 (0/45/90/...) — bRandomDirection=false일 때만 사용
	UPROPERTY(EditAnywhere, Category = "Parameter", meta = (EditCondition = "!bRandomDirection"))
	float FixedDirectionYaw = 0.f;

	UPROPERTY(EditAnywhere, Category = "Parameter", meta = (ClampMin = "0.1"))
	float PlayRate = 1.f;

	// 루트모션 거리 배율 (마스터) — 몽타주 원본 기준 스케일
	// 1.0 = 원본 거리, 0.5 = 절반, 1.5 = 1.5배, 0 = 제자리 (모션만 재생)
	// 재생 속도(PlayRate)와 무관하게 이동 거리만 조절
	UPROPERTY(EditAnywhere, Category = "Parameter", meta = (ClampMin = "0.0", ClampMax = "3.0"))
	float RootMotionScale = 1.0f;

	// 방향별 추가 배율은 Boss->RollDirectionScales로 이동됨 (최종 = RootMotionScale × Boss->RollDirectionScales[Idx])

	// 롤 종료 후 다음 트랜지션까지 대기 (시각 검증/연속 롤 간격 시뮬용)
	UPROPERTY(EditAnywhere, Category = "Parameter", meta = (ClampMin = "0.0"))
	float PostRollDelay = 1.5f;

	// i-frame 부여 — Boss.State.Invulnerable 태그를 몽타주 전체 구간에 적용
	// (정식 무적 분기는 AT3MidBossMonster::TakeDamage 참조 — 데미지 0 처리)
	// false 시 STT가 태그를 토글하지 않음 — 몽타주 ANS_BossInvulnerable 트랙으로 정밀 구간(5~45f) 제어할 때 사용
	UPROPERTY(EditAnywhere, Category = "Parameter")
	bool bUseInvulnerableTag = true;

	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<AT3MidBossMonster> Boss = nullptr;

	// 내부 상태
	UPROPERTY()
	TObjectPtr<UAnimMontage> ActiveRoll = nullptr;

	UPROPERTY()
	bool bRollEnded = false;

	UPROPERTY()
	float DelayElapsed = 0.f;

	// ExitState에서 스케일 복원 여부 판단용 (EnterState에서 적용했을 때만 true)
	UPROPERTY()
	bool bAppliedScale = false;
};

USTRUCT(meta = (DisplayName = "Test Roll (Desmond)"))
struct DESECRATION_API FT3STT_TestRoll : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FT3STT_TestRollInstanceData;

	virtual const UStruct* GetInstanceDataType() const override
	{
		return FT3STT_TestRollInstanceData::StaticStruct();
	}

	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context,
		const FStateTreeTransitionResult& Transition) const override;
	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context,
		const float DeltaTime) const override;
	virtual void ExitState(FStateTreeExecutionContext& Context,
		const FStateTreeTransitionResult& Transition) const override;
};

// ============================================================
// Task: FT3STT_Block
// 막기 시퀀스 트리거 — Boss->StartBlockSequence() 호출, In→Loop(자기루프)→Out 흐름은 Boss가 직접 관리
// 종료 정책 (STT 측):
//   1. 막힘 횟수가 ResolvedHitThreshold 도달 → RequestEndBlockSequence (가드 정상 종료, PostBlock 경로)
//      ※ ResolvedHitThreshold는 EnterState에서 RandRange(BlockReactionMinHits, BlockReactionMaxHits)로 1회 추첨
//   2. MaxDuration 경과 → RequestEndBlockSequence (Loop가 다음 BlendingOut에 Out으로 전환 → PostBlock 경로)
//   3. CurrentBlockPhase == Idle 도달 시 Succeeded 반환 (Out 끝까지 자연 종료)
// 정상 종료 시 ExitState에서 PendingReactionSource=FromBlock 세팅 → 다음 ExecutePattern이 리액션 모드
// 외부 인터럽트 (사망/스턴 등 ST 트랜지션) → ExitState에서 StopBlockSequence 호출
// ============================================================

USTRUCT()
struct FT3STT_BlockInstanceData
{
	GENERATED_BODY()

	// 종료 요청 송신까지의 시간 (초) — 이 시간 경과 시 RequestEndBlockSequence 호출 (PostBlock 정상 경로)
	// (실제 STT Succeeded는 Out 단계까지 끝나고 Phase=Idle 도달 시점)
	UPROPERTY(EditAnywhere, Category = "Parameter", meta = (ClampMin = "0.1"))
	float MaxDuration = 3.0f;

	// 가드 풀림 임계치 추첨 범위 — EnterState에서 RandRange(Min, Max)로 1회 결정.
	// 막힌(또는 가드 뚫린) 누적 피격수가 그 값에 도달하면 RequestEndBlockSequence (PostBlock 정상 경로).
	// MaxDuration과 별도 축 — 둘 중 먼저 도달한 조건이 가드를 풀어줌.
	UPROPERTY(EditAnywhere, Category = "Parameter", meta = (ClampMin = "1"))
	int32 BlockReactionMinHits = 1;

	UPROPERTY(EditAnywhere, Category = "Parameter", meta = (ClampMin = "1"))
	int32 BlockReactionMaxHits = 3;

	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<AT3MidBossMonster> Boss = nullptr;

	// 내부 상태 — 진입 시점의 막힘 카운트 (델타 측정용, StartBlockSequence가 0으로 리셋하므로 보통 0)
	UPROPERTY()
	int32 InitialHitsCount = 0;

	// EnterState에서 RandRange(Min, Max)로 추첨된 임계치 — Tick이 이 값을 폴링
	UPROPERTY()
	int32 ResolvedHitThreshold = 0;

	UPROPERTY()
	float ElapsedTime = 0.f;

	// RequestEndBlockSequence를 한 번만 호출하기 위한 플래그
	UPROPERTY()
	bool bEndRequested = false;
};

USTRUCT(meta = (DisplayName = "Block (Defense)"))
struct DESECRATION_API FT3STT_Block : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FT3STT_BlockInstanceData;

	virtual const UStruct* GetInstanceDataType() const override
	{
		return FT3STT_BlockInstanceData::StaticStruct();
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
// Condition: FT3STC_StunGaugeBelow
// 스턴 게이지가 임계 비율 미만일 때만 true — 회피/구르기 진입 차단용
// 비율 = CurrentStunGauge / StunThreshold (StunThreshold<=0이면 항상 true)
// ============================================================

USTRUCT()
struct FT3STC_StunGaugeBelowInstanceData
{
	GENERATED_BODY()

	// 허용 최대 비율 — Ratio < MaxRatio이면 true (예: 0.6 = 게이지 60% 미만일 때만 통과)
	UPROPERTY(EditAnywhere, Category = "Parameter", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float MaxRatio = 0.6f;

	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<AT3MidBossMonster> Boss = nullptr;
};

USTRUCT(meta = (DisplayName = "Stun Gauge Below Ratio"))
struct DESECRATION_API FT3STC_StunGaugeBelow : public FStateTreeConditionCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FT3STC_StunGaugeBelowInstanceData;

	virtual const UStruct* GetInstanceDataType() const override
	{
		return FT3STC_StunGaugeBelowInstanceData::StaticStruct();
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

// ============================================================
// Consideration: FT3Consideration_GaugePressure
// 스턴 게이지 압력에 비례/반비례하여 점수 산출 — 게이지 누적 시 보스 행동 편향
// 사용 예 (Desmond):
//   - bInverse=true (방어/공격 패턴) → 게이지 높을수록 점수 ↓ (가드/공격 빈도 감소)
//   - bInverse=false (Disengage/회피) → 게이지 높을수록 점수 ↑ (도망/회피 빈도 증가)
// 점수 = MinScore + (1 - MinScore) × Curve, Curve = ratio^Exponent (정방향) or (1-ratio^Exponent) (반방향)
// ratio = Clamp(CurrentStunGauge / StunThreshold, 0, 1)
// ============================================================

USTRUCT()
struct FT3Consideration_GaugePressureInstanceData
{
	GENERATED_BODY()

	// true면 게이지 ↑ → 점수 ↓ (가드/공격 억제용). false면 게이지 ↑ → 점수 ↑ (회피/도망 가속용)
	UPROPERTY(EditAnywhere, Category = "Parameter")
	bool bInverse = true;

	// 곡선 지수 — 1.0=선형, >1=후반 가중(게이지 다 찰수록 급격), <1=초반 가중(조금만 차도 효과)
	UPROPERTY(EditAnywhere, Category = "Parameter", meta = (ClampMin = "0.1", ClampMax = "10.0"))
	float Exponent = 1.0f;

	// 점수 하한 — 0.0이면 극단치에서 완전 제외, >0이면 항상 일정 확률 보장
	UPROPERTY(EditAnywhere, Category = "Parameter", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float MinScore = 0.1f;

	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<AT3MidBossMonster> Boss = nullptr;
};

USTRUCT(meta = (DisplayName = "Gauge Pressure (Consideration)"))
struct DESECRATION_API FT3Consideration_GaugePressure : public FStateTreeConsiderationCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FT3Consideration_GaugePressureInstanceData;

	virtual const UStruct* GetInstanceDataType() const override
	{
		return FT3Consideration_GaugePressureInstanceData::StaticStruct();
	}

protected:
	virtual float GetScore(FStateTreeExecutionContext& Context) const override;
};
