// T3MidBossTypes.h
// 중간보스 공용 타입 정의 (구조체, 열거형, 델리게이트)

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "Player/T3DamageTypes.h"
#include "T3MidBossTypes.generated.h"

class UAnimMontage;
class UT3MidBossMaterialSet;

// ============================================================
// Enum: 패턴 분류 (고정 — 거리 기반 선택에 사용)
// ============================================================

UENUM(BlueprintType)
enum class EMidBossPatternCategory : uint8
{
	Melee		UMETA(DisplayName = "Melee"),
	Ranged		UMETA(DisplayName = "Ranged"),
	Skill		UMETA(DisplayName = "Skill"),
	Evasion		UMETA(DisplayName = "Evasion"),
	GapCloser	UMETA(DisplayName = "GapCloser")
};

// ============================================================
// Enum: 막기 시퀀스 단계
// In → Loop(자기 자신 무한) → 외부 신호 → Out → Idle
// ============================================================

UENUM(BlueprintType)
enum class EBlockPhase : uint8
{
	Idle	UMETA(DisplayName = "Idle"),
	In		UMETA(DisplayName = "In"),
	Loop	UMETA(DisplayName = "Loop"),
	Out		UMETA(DisplayName = "Out")
};

// ============================================================
// Struct: 막기 몽타주 데이터
// 같은 몽타주의 여러 섹션을 In/Loop/Out에 분할해서 넣어도 되고,
// 완전히 다른 3개 몽타주(짜집기)를 넣어도 동일하게 동작
// 단계 전환은 BlendingOut 콜백에서 다음 PlayAnimMontage 호출 → 자연 크로스페이드
// ============================================================

USTRUCT(BlueprintType)
struct FBlockMontageEntry
{
	GENERATED_BODY()

	// 재생할 몽타주
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<UAnimMontage> Montage = nullptr;

	// 시작 섹션 — NAME_None이면 몽타주 디폴트 첫 섹션
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName SectionName = NAME_None;

	// 재생 배속
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0.1"))
	float PlayRate = 1.0f;
};

USTRUCT(BlueprintType)
struct FBlockMontageData
{
	GENERATED_BODY()

	// 막기 진입 모션
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FBlockMontageEntry InEntry;

	// 무한 루프 (디자이너가 몽타주 자체에 자기 자신 NextSection 또는 bLooping 설정 권장)
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FBlockMontageEntry LoopEntry;

	// 종료 모션 — RequestEndBlockSequence 후 다음 BlendingOut 시점에 자연 크로스페이드로 진입
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FBlockMontageEntry OutEntry;
};

// ============================================================
// Struct: 공격 패턴 데이터
// ============================================================

// 체인 내 개별 몽타주 데이터 (한 섹션 = 한 공격)
USTRUCT(BlueprintType)
struct FPatternMontageData
{
	GENERATED_BODY()

	// 재생할 몽타주
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<UAnimMontage> Montage = nullptr;

	// 섹션 콤보 모드에서 재생할 섹션 이름 (bUseSectionCombo=true일 때만 사용)
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName SectionName = NAME_None;

	// 시작 배속
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float PlayRate = 1.0f;

	// 데미지
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Damage = 20.f;

	// 경직 강도
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EHitIntensity HitIntensity = EHitIntensity::Light;

	// 데미지 타입
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSubclassOf<UT3DamageType_Base> DamageTypeClass;

	// 모션 워프 최대 거리 오버라이드 (0 이하 = 보스 기본값 사용)
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float MaxWarpDistanceOverride = 0.f;

#if WITH_EDITORONLY_DATA
	// 에디터 표시용 (TitleProperty) — 자동 생성, 직접 수정 불필요
	UPROPERTY(VisibleAnywhere, Transient)
	FString DisplayTitle;

	// 에디터에서 값 변경 시 DisplayTitle 갱신
	void UpdateDisplayTitle()
	{
		DisplayTitle = FString::Printf(TEXT("[%s] Dmg:%.2f Rate:%.2f"),
			*SectionName.ToString(),
			Damage,
			PlayRate);

		if (MaxWarpDistanceOverride > 0.f)
		{
			DisplayTitle += FString::Printf(TEXT(" Warp:%.0f"), MaxWarpDistanceOverride);
		}
	}
#endif
};

// 노티파이별 기본 발동 확률
USTRUCT(BlueprintType)
struct FNotifyChanceConfig
{
	GENERATED_BODY()

	// Slow 노티파이 발동 확률 (엇박자 감속)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float SlowChance = 1.0f;

	// Fast 노티파이 발동 확률 (엇박자 가속)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float FastChance = 1.0f;

	// Step 노티파이 발동 확률 (추적 이동)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float StepChance = 1.0f;
};

// 공격 패턴 전체 데이터
USTRUCT(BlueprintType)
struct FMidBossAttackPattern
{
	GENERATED_BODY()

	// 패턴 이름 (자유 입력 — "Melee_1", "DK_HeavySlash" 등)
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName PatternName = NAME_None;

	// 분류 (Melee / Ranged / Skill / Evasion)
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EMidBossPatternCategory Category = EMidBossPatternCategory::Melee;

	// 체인 몽타주 배열 (순서대로 재생, 1개면 단타, 2개면 2타 체인)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (TitleProperty = "{DisplayTitle}"))
	TArray<FPatternMontageData> MontageChain;

	// 섹션 콤보 모드 — MontageChain[0]의 몽타주를 섹션으로 진행
	// true: 같은 몽타주 내 섹션 자동 연결 (idle 복귀 없음)
	// 마지막 엔트리에 다른 몽타주 지정 시 체인으로 연결 (자연스러운 idle 복귀용)
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bUseSectionCombo = false;

	// 해금 스테이지 (1 = 항상 사용 가능)
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 RequiredStage = 1;

	// 쿨다운 초 (0이면 쿨다운 없음)
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Cooldown = 0.f;

	// 공용 쿨다운 그룹 (같은 그룹 패턴은 쿨다운 공유 — 예: "DK_Combo2" 그룹이면 Short/Mid/Full 동시 쿨다운)
	// NAME_None이면 개별 쿨다운만 사용
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName CooldownGroup = NAME_None;

	// 그룹 쿨다운 초 (이 패턴 사용 시 그룹 전체에 걸리는 쿨다운)
	// Cooldown = 개별, GroupCooldown = 그룹 — 둘 다 적용됨
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = "CooldownGroup != NAME_None"))
	float GroupCooldown = 0.f;

	// ActionCount 소모량 (0 = 소모 안 함, 예: Evasion)
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 ActionCountCost = 1;

	// 패링 윈도우 사용 여부 (ParryWindowStart 노티파이가 이 패턴에서만 동작)
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bHasParryWindow = false;

	// 노티파이별 기본 발동 확률 (BP에서 ModifyNotifyChance로 상황별 보정 가능)
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FNotifyChanceConfig NotifyChances;

	// 시작 체인 인덱스 — 0이 아니면 패턴 진입 시 앞쪽 N개 엔트리 스킵하고 해당 인덱스부터 시작
	// 막기 리액션 패턴 등에서 "1번 모션부터 곧장 진입" 용도. 클램프는 ExecutePattern에서 처리
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0"))
	int32 StartSectionIndex = 0;

	// 패턴 전체 PlayRate에 곱해지는 배율 — 막기 리액션 패턴에서 모션을 빠르게 굴릴 때 사용
	// 1.0 = 변화 없음, 1.5 = 50% 빠르게. MontageData.PlayRate × CurrentAttackAnimRate × 이 값
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0.1"))
	float ReactionPlayRateMultiplier = 1.0f;

	// true면 PostBlock/PostRoll 윈도우 활성 시 "리액션 패턴" 후보로 가중. false(기본)면 일반 패턴으로만 사용.
	// (※ ParryWindow 카운터 패턴과 무관 — 별도 메커니즘)
	// false인 패턴은 ReactionWindow Consideration이 가중 부풀림에서 제외 → 기존 Dark Knight 등 무영향
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bAllowAsReaction = false;

	// bAsReaction=true로 실행 시에만 적용되는 시작 인덱스 오버라이드.
	// -1(기본) = StartSectionIndex 그대로 사용. 0 이상이면 리액션 모드 한정으로 이 값 사용.
	// 예: 일반 모드는 0번 엔트리부터, 리액션 모드는 2번 엔트리(빠른 진입)부터.
	// ※ 체인형 패턴(bUseSectionCombo=false) 전용. 섹션 콤보는 ReactionSectionNameOverride 사용.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "-1", EditCondition = "bAllowAsReaction"))
	int32 ReactionStartSectionOverride = -1;

	// 섹션 콤보 패턴(bUseSectionCombo=true) 한정 — 리액션 모드 진입 시 시작할 섹션 이름.
	// NAME_None(기본) = MontageChain[0].SectionName 그대로 사용. 지정 시 해당 섹션부터 재생.
	// 예: 일반 모드는 Wind_Up 섹션부터, 리액션 모드는 Strike 섹션부터 (윈드업 스킵).
	// ※ ReactionStartSectionOverride(체인 인덱스용)와 별개 메커니즘 — 섹션 콤보는 MontageChain[0]만 몽타주이므로 인덱스 점프 불가, 섹션 점프만 가능.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = "bAllowAsReaction"))
	FName ReactionSectionNameOverride = NAME_None;
};

// ============================================================
// Delegate
// ============================================================

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnMidBossHit);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnMidBossStun);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnMidBossDeath);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnMidBossDamaged);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnMidBossSpawned);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnMidBossActivated);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnMidBossDeathFinished);

// 패턴 완료 델리게이트 — FName으로 어떤 패턴이 끝났는지 전달
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPatternCompleted, FName, CompletedPatternName);

// C++ 전용 Non-dynamic 델리게이트 (StateTree Task 바인딩용)
DECLARE_MULTICAST_DELEGATE(FOnPatternCompletedNative);

// ============================================================
// Struct: 스탯
// ============================================================

USTRUCT(BlueprintType)
struct FMidBossStats
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float MaxHP = 1500.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float CurrentHP = 1500.f;

	// 공격력 배율 (섹션 Damage × 이 값 = 최종 데미지)
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float AttackMultiplier = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float StunThreshold = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float CurrentStunGauge = 0.f;
};

// ============================================================
// DataTable Row: 스테이지별 보스 스탯 (RowName = "1", "2", "3" ...)
// ============================================================

USTRUCT(BlueprintType)
struct FMidBossStageRow : public FTableRowBase
{
	GENERATED_BODY()

	// 보스 표시 이름 (로컬라이징 대응)
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FText BossDisplayName;

	// 최대 체력
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float MaxHP = 1500.f;

	// 공격력 배율 (섹션 Damage × 이 값 = 최종 데미지)
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float AttackMultiplier = 1.f;

	// 스턴 임계치
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float StunThreshold = 100.f;

	// 머티리얼 세트 (nullptr이면 BP 기본 머티리얼 사용)
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<UT3MidBossMaterialSet> MaterialSet;
};
