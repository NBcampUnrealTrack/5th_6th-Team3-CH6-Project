// T3MidBossMonster.h

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "GameplayTagContainer.h"
#include "Monster/T3MidBossTypes.h"
#include "Monster/T3MidBossNotifyModifier.h"
#include "Components/StateTreeComponent.h"
#include "Components/TimelineComponent.h"
#include "MotionWarpingComponent.h"
#include "NativeGameplayTags.h"
#include "T3MidBossMonster.generated.h"

class UT3BossWeaponComponent;
class UT3MidBossHPBarWidget;
class UCurveFloat;
class UAudioComponent;

// StateTree 이벤트 태그 (extern — STNodes에서 참조)
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Boss_Event_StunRecovered);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Boss_Event_ActionCountDepleted);

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
	// MotionWarping (루트모션 워프)
	// ==========================================================
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "MidBoss|Movement")
	TObjectPtr<UMotionWarpingComponent> MotionWarpingComponent;

	// 이동 워프: 타겟 앞에서 멈출 오프셋 거리 (거리에 따라 동적 클램프)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MidBoss|Movement")
	float WarpTargetOffset = 150.f;

	// 이동 워프: 최소 접근 거리 (이 이하로는 오프셋이 0이 되어 후진 방지)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MidBoss|Movement")
	float MinWarpDistance = 50.f;

	// 이동용 워프 타겟 (오프셋 적용)
	static inline const FName MotionWarpTargetName = FName(TEXT("CombatTarget"));

	// 회전용 워프 타겟 (오프셋 없음 — 플레이어 정확한 위치)
	static inline const FName MotionWarpTargetRotationName = FName(TEXT("CombatTargetRotation"));

	// MotionWarping 타겟 갱신 (이동용 + 회전용)
	UFUNCTION(BlueprintCallable, Category = "MidBoss|Movement")
	void UpdateMotionWarpTarget();

	// ==========================================================
	// 입장 (Entry) — 트리거에서 호출, StateTree 시작
	// ==========================================================
	UFUNCTION(BlueprintCallable, Category = "MidBoss|Flow")
	void ActivateBoss(AActor* Activator);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "MidBoss|Flow")
	bool IsActivated() const { return bIsActivated; }

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

	// 비공격 시 회전 속도 (SetFocus 기반, 도/초)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MidBoss|Movement")
	float IdleRotationRate = 360.f;

	// 공격 중 회전 속도 — 느리게 트래킹해서 다음 공격 시 스냅 방지 (도/초, 0이면 완전 고정)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MidBoss|Movement")
	float AttackRotationRate = 90.f;

	// ==========================================================
	// 피격 카메라 쉐이크 (공격 중에도 항상 재생)
	// ==========================================================

	// 카메라 쉐이크 클래스 (에디터에서 할당)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MidBoss|Combat")
	TSubclassOf<UCameraShakeBase> HitCameraShakeClass;

	// 카메라 쉐이크 외부 반경
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MidBoss|Combat")
	float HitShakeOuterRadius = 600.f;

	// 카메라 쉐이크 감쇠
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MidBoss|Combat")
	float HitShakeFalloff = 1.2f;

	// ==========================================================
	// 히트 리액션 몽타주 (방향별, 비공격 시에만 재생)
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
	// 사망 연출
	// ==========================================================

	// 사망 몽타주 (없으면 즉시 FinishDeathSequence)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MidBoss|Animation")
	TObjectPtr<UAnimMontage> DeathMontage;

	// 사망 연출 완료 후 액터 제거까지 대기 시간 (초)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MidBoss|Flow")
	float DeathCleanupDelay = 30.f;

	// true: FinishDeathSequence에서 자동 무기 드롭 / false: AnimNotify 등에서 수동 호출
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MidBoss|Flow")
	bool bAutoDropWeapon = true;

	// ==========================================================
	// 디졸브 연출
	// ==========================================================

	// 디졸브 활성화 여부 (false면 디졸브 없이 기존 방식으로 사라짐)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MidBoss|Dissolve")
	bool bEnableDissolve = true;

	// 디졸브 재생 시간 (초)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MidBoss|Dissolve", meta = (EditCondition = "bEnableDissolve"))
	float DissolveDuration = 3.f;

	// 디졸브 커브 (nullptr이면 선형 0→1 자동 생성)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MidBoss|Dissolve", meta = (EditCondition = "bEnableDissolve"))
	TObjectPtr<UCurveFloat> DissolveCurve;

	// 디졸브 머티리얼 파라미터 이름
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MidBoss|Dissolve", meta = (EditCondition = "bEnableDissolve"))
	FName DissolveParameterName = TEXT("Dissolve");

	// 디졸브 사운드 (선택사항)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MidBoss|Dissolve", meta = (EditCondition = "bEnableDissolve"))
	TObjectPtr<USoundBase> DissolveSound;

	// 디졸브 시작 (외부에서 호출 가능 — AnimNotify 등)
	UFUNCTION(BlueprintCallable, Category = "MidBoss|Dissolve")
	void StartDissolve();

	// ==========================================================
	// 사운드
	// ==========================================================

	// 효과음 볼륨 기본값 (개별 배수와 곱하여 최종 볼륨 결정)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MidBoss|Sound")
	float SoundVolume = 1.0f;

	// 피격 사운드 (매 피격마다 재생)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MidBoss|Sound")
	TObjectPtr<USoundBase> HitSound;

	// 피격 볼륨 배수
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MidBoss|Sound")
	float HitVolumeMultiplier = 3.0f;

	// 스턴 사운드
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MidBoss|Sound")
	TObjectPtr<USoundBase> StunSound;

	// 스턴 볼륨 배수
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MidBoss|Sound")
	float StunVolumeMultiplier = 1.5f;

	// 사망 사운드
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MidBoss|Sound")
	TObjectPtr<USoundBase> DeathSound;

	// 사망 볼륨 배수
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MidBoss|Sound")
	float DeathVolumeMultiplier = 1.5f;

	// 보스전 BGM (ActivateBoss에서 재생, 사망 시 페이드아웃)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MidBoss|Sound")
	TObjectPtr<USoundBase> BossBGM;

	// BGM 볼륨
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MidBoss|Sound")
	float BGMVolume = 1.0f;

	// BGM 페이드아웃 시간 (사망 시, 초)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MidBoss|Sound")
	float BGMFadeOutDuration = 3.0f;

	// ==========================================================
	// 보스 HP바 위젯
	// ==========================================================

	// HP바 위젯 클래스 (에디터에서 WBP_T3MidBossHPBar 할당)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MidBoss|UI")
	TSubclassOf<UT3MidBossHPBarWidget> BossHPBarWidgetClass;

	// ==========================================================
	// 무기 컴포넌트
	// ==========================================================
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "MidBoss|Weapon")
	TObjectPtr<UT3BossWeaponComponent> WeaponComponent;

	// ==========================================================
	// 공격 패턴 데이터 (에디터에서 세팅)
	// ==========================================================

	// 전체 패턴 배열 — BP 에디터에서 몽타주/데미지/스테이지 등 설정
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MidBoss|Patterns")
	TArray<FMidBossAttackPattern> AttackPatterns;

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

	// 보스 활성화 (트리거 진입 → AI 시작) — Level BP에서 안개벽 등 연출
	UPROPERTY(BlueprintAssignable, Category = "MidBoss|Events")
	FOnMidBossActivated OnMidBossActivated;

	// 사망 연출 완료 (몽타주+무기드롭+충돌해제 후) — Level BP에서 안개벽 해제, 보상
	UPROPERTY(BlueprintAssignable, Category = "MidBoss|Events")
	FOnMidBossDeathFinished OnMidBossDeathFinished;

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

	// 스턴 몽타주 (스턴 진입 시 재생)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MidBoss|Animation")
	TObjectPtr<UAnimMontage> StunMontage;

	// 스턴 지속 시간 (타이머로 자동 해제)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MidBoss|Combat")
	float StunDuration = 3.0f;

	UFUNCTION(BlueprintCallable, Category = "MidBoss|Combat")
	void ApplyStun();

	UFUNCTION(BlueprintCallable, Category = "MidBoss|Combat")
	void RecoverFromStun();

	// C++ 전용 델리게이트 (StateTree Task에서 Lambda 바인딩)
	FOnPatternCompletedNative OnPatternCompletedNative;

private:
	// --- 입장/사망 내부 상태 ---
	bool bIsActivated = false;

	void BeginDeathSequence();
	void FinishDeathSequence();

	// --- BGM AudioComponent ---
	UPROPERTY()
	TObjectPtr<UAudioComponent> BGMAudioComponent;

	// --- HP바 위젯 인스턴스 ---
	UPROPERTY()
	TObjectPtr<UT3MidBossHPBarWidget> BossHPBarWidget;

	// --- 디졸브 내부 ---
	UPROPERTY()
	TObjectPtr<UTimelineComponent> DissolveTimeline;

	UPROPERTY()
	TArray<TObjectPtr<UMaterialInstanceDynamic>> DynamicMaterials;

	// 기본 선형 커브 (DissolveCurve가 nullptr일 때 자동 생성)
	UPROPERTY()
	TObjectPtr<UCurveFloat> DefaultDissolveCurve;

	void CreateDynamicMaterials();

	UFUNCTION()
	void OnDissolveUpdate(float Value);

	UFUNCTION()
	void OnDissolveFinished();

	UFUNCTION()
	void OnDeathMontageEnded(UAnimMontage* Montage, bool bInterrupted);

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

	// 무기 히트 → 데미지 적용
	UFUNCTION()
	void OnWeaponHit(AActor* HitActor);
};
