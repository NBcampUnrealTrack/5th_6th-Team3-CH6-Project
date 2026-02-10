// T3MidBossMonster.h

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "GameplayTagContainer.h"
#include "Player/T3DamageTypes.h"
#include "Monster/T3MidBossNotifyModifier.h"
#include "Components/StateTreeComponent.h"
#include "T3MidBossMonster.generated.h"

// ============================================================
// Enum: 패턴 분류 (고정 — 거리 기반 선택에 사용)
// ============================================================

UENUM(BlueprintType)
enum class EMidBossPatternCategory : uint8
{
	Melee		UMETA(DisplayName = "Melee"),
	Ranged		UMETA(DisplayName = "Ranged"),
	Skill		UMETA(DisplayName = "Skill"),
	Evasion		UMETA(DisplayName = "Evasion")
};

// ============================================================
// Struct: 공격 패턴 데이터
// ============================================================

// 체인 내 개별 몽타주 데이터
USTRUCT(BlueprintType)
struct FPatternMontageData
{
	GENERATED_BODY()

	// 재생할 몽타주
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<UAnimMontage> Montage = nullptr;

	// 시작 배속
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float PlayRate = 1.0f;

	// 이 구간 데미지
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Damage = 20.f;

	// 경직 강도
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EHitIntensity HitIntensity = EHitIntensity::Light;

	// 데미지 타입 (Base, Unparryable 등)
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSubclassOf<UDamageType> DamageTypeClass;
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
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FPatternMontageData> MontageChain;

	// 해금 스테이지 (1 = 항상 사용 가능)
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 RequiredStage = 1;

	// 쿨다운 초 (0이면 쿨다운 없음)
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Cooldown = 0.f;

	// ActionCount 소모량 (0 = 소모 안 함, 예: Evasion)
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 ActionCountCost = 1;

	// 노티파이별 기본 발동 확률 (BP에서 ModifyNotifyChance로 상황별 보정 가능)
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FNotifyChanceConfig NotifyChances;
};

// ============================================================
// Delegate
// ============================================================

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnMidBossHit);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnMidBossStun);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnMidBossDeath);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnMidBossDamaged);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnMidBossSpawned);

// 패턴 완료 델리게이트 — FName으로 어떤 패턴이 끝났는지 전달
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPatternCompleted, FName, CompletedPatternName);

// C++ 전용 Non-dynamic 델리게이트 (StateTree Task 바인딩용)
DECLARE_MULTICAST_DELEGATE(FOnStunRecoveredNative);
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

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float AttackPower = 20.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float StunThreshold = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float CurrentStunGauge = 0.f;
};

// ============================================================
// AT3MidBossMonster
// ============================================================

UCLASS()
class DESECRATION_API AT3MidBossMonster : public ACharacter
{
	GENERATED_BODY()

public:
	AT3MidBossMonster();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

public:
	// ==========================================================
	// StateTree (standalone 스키마 — AI Controller 미사용)
	// ==========================================================
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "MidBoss|AI")
	TObjectPtr<UStateTreeComponent> StateTreeComponent;

	// ==========================================================
	// 전투 타겟
	// ==========================================================
	UPROPERTY(BlueprintReadWrite, Category = "MidBoss|AI")
	TObjectPtr<AActor> CombatTarget;

	// ==========================================================
	// 상태 태그 컨테이너 (Bool 플래그 대체)
	// ==========================================================
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MidBoss|State")
	FGameplayTagContainer ActiveGameplayTags;

	// --- 태그 헬퍼 함수 ---
	UFUNCTION(BlueprintCallable, Category = "MidBoss|State")
	void AddStateTag(FGameplayTag Tag);

	UFUNCTION(BlueprintCallable, Category = "MidBoss|State")
	void RemoveStateTag(FGameplayTag Tag);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "MidBoss|State")
	bool HasStateTag(FGameplayTag Tag) const;

	// --- Bool 호환 getter (태그 기반) ---
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "MidBoss|State")
	bool IsDead() const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "MidBoss|State")
	bool IsStunned() const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "MidBoss|State")
	bool IsExecutingPattern() const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "MidBoss|State")
	bool HasSuperArmor() const;

	// ==========================================================
	// 보스 정보 & 스탯
	// ==========================================================
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MidBoss|Info")
	FString BossName = "MidBoss";

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MidBoss|Stats")
	FMidBossStats MidBossStats;

	// 스테이지 번호 (1~3, 레벨별로 에디터에서 설정)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MidBoss|Stage")
	int32 BossStage = 1;

	// ==========================================================
	// ActionCount (공격 버스트 카운터 — StateTree 조건에서 바인딩)
	// ==========================================================
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MidBoss|AI")
	int32 ActionCount = 3;

	// ==========================================================
	// 타겟 방향 부드러운 회전
	// ==========================================================
	UFUNCTION(BlueprintCallable, Category = "MidBoss|Movement")
	void FaceTarget(float InterpSpeed = 10.f);

	// ==========================================================
	// 애디티브 히트 리액션 몽타주 (방향별)
	// ==========================================================

	// 기본 피격 (방향 판별 불가 시 폴백)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MidBoss|Animation")
	TObjectPtr<UAnimMontage> HitReactMontage_Default;

	// 전방 피격 (앞에서 맞았을 때)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MidBoss|Animation")
	TObjectPtr<UAnimMontage> HitReactMontage_F;

	// 후방 피격 (뒤에서 맞았을 때)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MidBoss|Animation")
	TObjectPtr<UAnimMontage> HitReactMontage_B;

	// 좌측 피격 (왼쪽에서 맞았을 때)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MidBoss|Animation")
	TObjectPtr<UAnimMontage> HitReactMontage_L;

	// 우측 피격 (오른쪽에서 맞았을 때)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MidBoss|Animation")
	TObjectPtr<UAnimMontage> HitReactMontage_R;

	// ==========================================================
	// 공격 패턴 데이터 (에디터에서 세팅)
	// ==========================================================

	// 전체 패턴 배열 — BP 에디터에서 몽타주/데미지/스테이지 등 설정
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MidBoss|Patterns")
	TArray<FMidBossAttackPattern> AttackPatterns;

	// 무기 판정용 컴포넌트 (BP에서 할당)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MidBoss|Combat")
	TObjectPtr<UPrimitiveComponent> WeaponCollisionComponent;

	// ==========================================================
	// 노티파이 보정기 시스템
	// ==========================================================

	// 보정기 DataAsset (에디터에서 할당 — 배속 범위, 거리/Pity/HP/Usage 보정)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MidBoss|Modifier")
	TObjectPtr<UMidBossModifierDataAsset> ModifierDataAsset;

	// 런타임 보정기 (BeginPlay에서 자동 생성)
	UPROPERTY(BlueprintReadOnly, Category = "MidBoss|Modifier")
	TObjectPtr<UMidBossNotifyModifier> NotifyModifier;

	// ==========================================================
	// 공격 패턴 실행
	// ==========================================================

	// 패턴 이름으로 공격 실행 (성공 시 true)
	UFUNCTION(BlueprintCallable, Category = "MidBoss|Pattern")
	bool ExecutePattern(FName PatternName);

	// 현재 실행 중인 패턴 강제 중단
	UFUNCTION(BlueprintCallable, Category = "MidBoss|Pattern")
	void CancelCurrentPattern();

	// 패턴 완료 델리게이트
	UPROPERTY(BlueprintAssignable, Category = "MidBoss|Events")
	FOnPatternCompleted OnPatternCompleted;

	// ==========================================================
	// 패턴 검색 (StateTree/BP에서 사용)
	// ==========================================================

	// 현재 스테이지에서 사용 가능한 패턴 이름 목록 (카테고리 필터)
	UFUNCTION(BlueprintCallable, Category = "MidBoss|Pattern")
	TArray<FName> GetAvailablePatterns(EMidBossPatternCategory Category) const;

	// 현재 스테이지에서 사용 가능한 전체 패턴 이름 목록
	UFUNCTION(BlueprintCallable, Category = "MidBoss|Pattern")
	TArray<FName> GetAllAvailablePatterns() const;

	// 패턴 데이터 조회 (BP에서 ActionCountCost 등 확인용)
	UFUNCTION(BlueprintCallable, Category = "MidBoss|Pattern")
	bool GetPatternData(FName PatternName, FMidBossAttackPattern& OutData) const;

	// ==========================================================
	// 몽타주 노티파이 핸들러
	// ==========================================================

	// 노티파이 이름 기반 공통 처리 (Slow, Fast, Normal, Step_N, AttackStart, AttackEnd)
	UFUNCTION(BlueprintCallable, Category = "MidBoss|Pattern")
	void HandlePatternNotify(FName NotifyName);

	// 노티파이 확률 보정 — BP에서 오버라이드하여 상황별 확률 조절
	// 기본 구현: 구조체의 기본 확률을 그대로 반환
	// BP 오버라이드 예: 거리 멀면 StepChance 올리기, HP 낮으면 SlowChance 내리기
	UFUNCTION(BlueprintNativeEvent, Category = "MidBoss|Pattern")
	float ModifyNotifyChance(FName NotifyName, float BaseChance) const;
	virtual float ModifyNotifyChance_Implementation(FName NotifyName, float BaseChance) const;

	// ==========================================================
	// 유틸리티 함수
	// ==========================================================

	UFUNCTION(BlueprintCallable, Category = "MidBoss|Movement")
	void MoveToTarget(float Duration, float Distance);

	UFUNCTION(BlueprintCallable, Category = "MidBoss|Combat")
	void SetAttackCollisionEnabled(bool bEnable);

	UFUNCTION(BlueprintCallable, Category = "MidBoss|Pattern")
	bool IsPatternOffCooldown(FName PatternName) const;

	// ==========================================================
	// 델리게이트 (기존)
	// ==========================================================
	UPROPERTY(BlueprintAssignable, Category = "MidBoss|Events")
	FOnMidBossHit OnMidBossHit;

	UPROPERTY(BlueprintAssignable, Category = "MidBoss|Events")
	FOnMidBossStun OnMidBossStun;

	UPROPERTY(BlueprintAssignable, Category = "MidBoss|Events")
	FOnMidBossDeath OnMidBossDeath;

	UPROPERTY(BlueprintAssignable, Category = "MidBoss|Events")
	FOnMidBossDamaged OnMidBossDamaged;

	UPROPERTY(BlueprintAssignable, Category = "MidBoss|Events")
	FOnMidBossSpawned OnMidBossSpawned;

	// ==========================================================
	// 데미지 처리 (기존)
	// ==========================================================

	UFUNCTION(BlueprintCallable, Category = "MidBoss|Combat")
	void ApplyDamageToMidBoss(float DamageAmount, float StunAmount, AActor* DamageCauser = nullptr);

	virtual float TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent,
		AController* EventInstigator, AActor* DamageCauser) override;

	// ==========================================================
	// 히트 리액션 / 스턴
	// ==========================================================

	// 방향별 애디티브 히트 리액션 (DamageCauser 위치 기준 F/B/L/R 판별)
	UFUNCTION(BlueprintCallable, Category = "MidBoss|Combat")
	void PlayAdditiveHitReaction(AActor* DamageCauser = nullptr);

	// 스턴 지속 시간 (타이머로 자동 해제)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MidBoss|Combat")
	float StunDuration = 3.0f;

	UFUNCTION(BlueprintCallable, Category = "MidBoss|Combat")
	void ApplyStun();

	UFUNCTION(BlueprintCallable, Category = "MidBoss|Combat")
	void RecoverFromStun();

	// C++ 전용 델리게이트 (StateTree Task에서 Lambda 바인딩)
	FOnStunRecoveredNative OnStunRecoveredNative;
	FOnPatternCompletedNative OnPatternCompletedNative;

private:
	// --- 패턴 실행 내부 상태 ---
	FName CurrentPatternName = NAME_None;
	int32 CurrentChainIndex = 0;

	// 패턴별 쿨다운 만료 시각 (GameTime 기준)
	UPROPERTY()
	TMap<FName, double> PatternCooldownExpireMap;

	// 스턴 자동 해제 타이머
	FTimerHandle StunTimerHandle;

	// --- MoveToTarget 보간 상태 ---
	bool bIsMovingToTarget = false;
	float MoveToTargetElapsed = 0.f;
	float MoveToTargetDuration = 0.f;
	FVector MoveToTargetDirection = FVector::ZeroVector;
	float MoveToTargetSpeed = 0.f;

	// --- 내부 함수 ---
	const FMidBossAttackPattern* FindPatternData(FName PatternName) const;
	void PlayCurrentChainMontage();

	UFUNCTION()
	void OnMontageEnded(UAnimMontage* Montage, bool bInterrupted);

	void AdvanceChainOrComplete();
	void RegisterCooldown(FName PatternName, float CooldownSeconds);
	void ResetPatternState();

	// 기본 확률 조회 + ModifyNotifyChance 적용 후 판정
	bool ShouldTriggerNotify(FName NotifyName) const;

	// 피격 방향 기반 히트 리액션 몽타주 선택
	UAnimMontage* GetDirectionalHitReactMontage(AActor* DamageCauser) const;
};
