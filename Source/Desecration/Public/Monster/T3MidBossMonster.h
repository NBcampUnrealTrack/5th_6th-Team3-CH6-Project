// T3MidBossMonster.h

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "GameplayTagContainer.h"
#include "Monster/T3MidBossTypes.h"
#include "Monster/T3MidBossNotifyModifier.h"
#include "Player/T3LockOnTarget.h"
#include "Components/StateTreeComponent.h"
#include "Components/TimelineComponent.h"
#include "MotionWarpingComponent.h"
#include "NativeGameplayTags.h"
#include "Interface/T3Monster.h"
#include "T3MidBossMonster.generated.h"

class UT3BossWeaponComponent;
class UT3MidBossHPBarWidget;
class UCurveFloat;
class USoundAttenuation;
class UAudioComponent;
class UWidgetComponent;

class UNiagaraSystem;
class AT3BossProjectile;

// 상태 태그 (extern — 분할 .cpp에서 참조)
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Boss_State_Dead);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Boss_State_Stunned);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Boss_State_ExecutingPattern);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Boss_State_SuperArmor);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Boss_State_ParryWindow);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Boss_State_Disengaging);
// 플레이어가 보스 공격을 패링 성공했을 때 외부 알림
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnParriedByPlayer);

// StateTree 이벤트 태그 (extern — STNodes에서 참조)
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Boss_Event_StunRecovered);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Boss_Event_ActionCountDepleted);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Boss_Event_ParriedByPlayer);

// ============================================================
// AT3MidBossMonster
// ============================================================

UCLASS()
class DESECRATION_API AT3MidBossMonster : public ACharacter, public IT3LockOnTarget, public IT3Monster
{
	GENERATED_BODY()

public:
	AT3MidBossMonster();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

public:

	// ============================================================
	// region: Core (컴포넌트, 상태, 기본 정보)
	// ============================================================
#pragma region Core

	// --- 록온 ---
protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "MidBoss|UI")
	TObjectPtr<UWidgetComponent> LockOnWidgetComponent;

public:
	virtual void SetLockOnWidgetVisible(bool bVisible) override;

	// --- AI / StateTree ---
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "MidBoss|AI")
	TObjectPtr<UStateTreeComponent> StateTreeComponent;

	UPROPERTY(BlueprintReadWrite, Category = "MidBoss|AI")
	TObjectPtr<AActor> CombatTarget;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MidBoss|AI")
	int32 ActionCount = 3;

	// 스테이지별 패턴 실행 횟수 (Disengage Consideration용, 발동 시 리셋)
	// [0]=Stage1, [1]=Stage2, [2]=Stage3
	int32 StagePatternCounts[3] = {0, 0, 0};

	// 연속 Disengage 횟수 (패턴 실행 시 리셋, ConsecutiveDisengagePenalty Consideration용)
	int32 ConsecutiveDisengageCount = 0;

	// --- 상태 태그 ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MidBoss|State")
	FGameplayTagContainer ActiveGameplayTags;

	UFUNCTION(BlueprintCallable, Category = "MidBoss|State")
	void AddStateTag(FGameplayTag Tag);

	UFUNCTION(BlueprintCallable, Category = "MidBoss|State")
	void RemoveStateTag(FGameplayTag Tag);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "MidBoss|State")
	bool HasStateTag(FGameplayTag Tag) const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "MidBoss|State")
	bool IsDead() const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "MidBoss|State")
	bool IsStunned() const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "MidBoss|State")
	bool IsExecutingPattern() const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "MidBoss|State")
	bool HasSuperArmor() const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "MidBoss|State")
	bool IsDisengaging() const;

	// --- 보스 정보 & 스탯 ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MidBoss|Info")
	FString BossName = "MidBoss";

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MidBoss|Stats")
	FMidBossStats MidBossStats;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MidBoss|Stage")
	int32 BossStage = 1;

	// --- 무기 ---
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "MidBoss|Weapon")
	TObjectPtr<UT3BossWeaponComponent> WeaponComponent;

	// --- HP바 위젯 ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MidBoss|UI")
	TSubclassOf<UT3MidBossHPBarWidget> BossHPBarWidgetClass;

#pragma endregion Core

	// ============================================================
	// region: Movement (회전, 워프, 이동)
	// ============================================================
#pragma region Movement

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "MidBoss|Movement")
	TObjectPtr<UMotionWarpingComponent> MotionWarpingComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MidBoss|Movement")
	float WarpTargetOffset = 150.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MidBoss|Movement")
	float MinWarpDistance = 50.f;

	// 이 거리 이하면 워프 비활성화 (제자리 공격)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MidBoss|Movement")
	float WarpDisableDistance = 100.f;

	// 이 거리 초과 시 이동 워프 클램핑
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MidBoss|Movement")
	float MaxWarpDistance = 350.f;

	static inline const FName MotionWarpTargetName = FName(TEXT("CombatTarget"));
	static inline const FName MotionWarpTargetRotationName = FName(TEXT("CombatTargetRotation"));

	UFUNCTION(BlueprintCallable, Category = "MidBoss|Movement")
	void UpdateMotionWarpTarget();

	UFUNCTION(BlueprintCallable, Category = "MidBoss|Movement")
	void FaceTarget(float InterpSpeed = 10.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MidBoss|Movement")
	float IdleRotationRate = 360.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MidBoss|Movement")
	float AttackRotationRate = 90.f;

	UFUNCTION(BlueprintCallable, Category = "MidBoss|Movement")
	void MoveToTarget(float Duration, float Distance);

	// 대시 중 이동 속도
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MidBoss|Movement")
	float DashMoveSpeed = 600.f;

#pragma endregion Movement

	// ============================================================
	// region: Pattern (공격 패턴, 노티파이, 보정기)
	// ============================================================
#pragma region Pattern

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MidBoss|Patterns", meta = (TitleProperty = "PatternName"))
	TArray<FMidBossAttackPattern> AttackPatterns;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MidBoss|Modifier")
	TObjectPtr<UMidBossModifierDataAsset> ModifierDataAsset;

	UPROPERTY(BlueprintReadOnly, Category = "MidBoss|Modifier")
	TObjectPtr<UMidBossNotifyModifier> NotifyModifier;

	UFUNCTION(BlueprintCallable, Category = "MidBoss|Pattern")
	bool ExecutePattern(FName PatternName);

	UFUNCTION(BlueprintCallable, Category = "MidBoss|Pattern")
	void CancelCurrentPattern();

	UFUNCTION(BlueprintCallable, Category = "MidBoss|Pattern")
	TArray<FName> GetAvailablePatterns(EMidBossPatternCategory Category) const;

	UFUNCTION(BlueprintCallable, Category = "MidBoss|Pattern")
	TArray<FName> GetAllAvailablePatterns() const;

	UFUNCTION(BlueprintCallable, Category = "MidBoss|Pattern")
	bool GetPatternData(FName PatternName, FMidBossAttackPattern& OutData) const;

	UFUNCTION(BlueprintCallable, Category = "MidBoss|Pattern")
	void HandlePatternNotify(FName NotifyName);

	UFUNCTION(BlueprintNativeEvent, Category = "MidBoss|Pattern")
	float ModifyNotifyChance(FName NotifyName, float BaseChance) const;
	virtual float ModifyNotifyChance_Implementation(FName NotifyName, float BaseChance) const;

	UFUNCTION(BlueprintCallable, Category = "MidBoss|Pattern")
	bool IsPatternOffCooldown(FName PatternName) const;

	// 패턴 데이터 조회 (ST Condition에서도 사용)
	const FMidBossAttackPattern* FindPatternData(FName PatternName) const;

#pragma endregion Pattern

	// ============================================================
	// region: Combat (데미지, 히트 리액션, 스턴, 카메라 쉐이크)
	// ============================================================
#pragma region Combat

	UFUNCTION(BlueprintCallable, Category = "MidBoss|Combat")
	void ApplyDamageToMidBoss(float DamageAmount, float StunAmount, AActor* DamageCauser = nullptr);

	virtual float TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent,
		AController* EventInstigator, AActor* DamageCauser) override;

	UFUNCTION(BlueprintCallable, Category = "MidBoss|Combat")
	void PlayAdditiveHitReaction(AActor* DamageCauser = nullptr);

	UFUNCTION(BlueprintCallable, Category = "MidBoss|Combat")
	void ApplyStun();

	UFUNCTION(BlueprintCallable, Category = "MidBoss|Combat")
	void RecoverFromStun();

	// --- AoE (장판기) ---
	UFUNCTION(BlueprintCallable, Category = "MidBoss|Combat")
	void ExecuteAoEDamage(float Radius, float DamageAmount, EHitIntensity Intensity = EHitIntensity::Heavy);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MidBoss|Skill")
	float AoERadius = 500.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MidBoss|Skill")
	TObjectPtr<UNiagaraSystem> AoEEffect;

	// AoE 프리뷰 이펙트 (범위 표시용 — 데미지 없이 시각적 경고만)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MidBoss|Skill")
	TObjectPtr<UNiagaraSystem> AoEPreviewEffect;

	// AoE 이펙트 스케일 (BP에서 눈으로 보고 조절)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MidBoss|Skill", meta = (ClampMin = "0.1"))
	float AoEEffectScale = 1.f;

	// AoE 프리뷰 사운드 (범위 경고음)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MidBoss|Sound")
	TObjectPtr<USoundBase> AoEPreviewSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MidBoss|Sound")
	float AoEPreviewVolumeMultiplier = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MidBoss|Sound")
	TObjectPtr<USoundBase> AoESound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MidBoss|Sound")
	float AoEVolumeMultiplier = 1.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MidBoss|Skill")
	TSubclassOf<UCameraShakeBase> AoECameraShakeClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MidBoss|Skill")
	float AoEShakeOuterRadius = 1000.f;

	// --- 투사체 (검기) ---
	UFUNCTION(BlueprintCallable, Category = "MidBoss|Combat")
	void SpawnBossProjectile();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MidBoss|Projectile")
	TSubclassOf<AT3BossProjectile> ProjectileClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MidBoss|Projectile")
	float ProjectileSpeed = 1500.f;

	// 스폰 오프셋 (보스 전방 기준)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MidBoss|Projectile")
	float ProjectileSpawnOffset = 100.f;

	// --- 플레이어 패링 반응 ---
	// 플레이어의 CombatComponent에서 패링 성공 시 호출하는 진입점
	UFUNCTION(BlueprintCallable, Category = "MidBoss|Combat")
	void NotifyParriedByPlayer();

	UPROPERTY(BlueprintAssignable, Category = "MidBoss|Combat")
	FOnParriedByPlayer OnParriedByPlayer;

	// 패링 히트리액션 재생 중 플래그 — 몽타주 종료 시 StateTree 이벤트 전송용
	bool bParryHitReactionPlaying = false;

	// 패링 히트리액션 몽타주 종료 콜백
	UFUNCTION()
	void OnParryHitReactionEnded(UAnimMontage* Montage, bool bInterrupted);

	// --- 패링 카운터 (보스 → 플레이어) ---
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "MidBoss|Combat")
	bool IsParryWindowActive() const;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MidBoss|Parry")
	float ParryWindowDuration = 1.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MidBoss|Sound")
	TObjectPtr<USoundBase> ParrySound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MidBoss|Sound")
	float ParryVolumeMultiplier = 2.0f;

	// --- 카메라 쉐이크 ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MidBoss|Combat")
	TSubclassOf<UCameraShakeBase> HitCameraShakeClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MidBoss|Combat")
	float HitShakeOuterRadius = 600.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MidBoss|Combat")
	float HitShakeFalloff = 1.2f;

	// --- 히트 리액션 몽타주 (방향별) ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MidBoss|Animation")
	TObjectPtr<UAnimMontage> HitReactMontage_Default;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MidBoss|Animation")
	TObjectPtr<UAnimMontage> HitReactMontage_F;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MidBoss|Animation")
	TObjectPtr<UAnimMontage> HitReactMontage_B;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MidBoss|Animation")
	TObjectPtr<UAnimMontage> HitReactMontage_L;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MidBoss|Animation")
	TObjectPtr<UAnimMontage> HitReactMontage_R;

	// --- 스턴 ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MidBoss|Animation")
	TObjectPtr<UAnimMontage> StunMontage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MidBoss|Combat")
	float StunDuration = 3.0f;

#pragma endregion Combat

	// ============================================================
	// region: Flow (활성화, 인트로, 사망, 디졸브, 사운드)
	// ============================================================
#pragma region Flow

	// --- 활성화 트리거 (외부 액터) ---
	// 레벨에 배치한 TriggerBox/TriggerSphere를 스포이드로 지정
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "MidBoss|Activation")
	TObjectPtr<AActor> ExternalActivationTrigger;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MidBoss|Activation")
	TObjectPtr<UAnimMontage> IntroMontage;

	UFUNCTION(BlueprintCallable, Category = "MidBoss|Flow")
	void ActivateBoss(AActor* Activator);

	/** 외부 활성화 트리거 바인딩 (스포너 등 런타임 세팅용) */
	UFUNCTION(BlueprintCallable, Category = "MidBoss|Flow")
	void BindExternalTrigger(AActor* Trigger);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "MidBoss|Flow")
	bool IsActivated() const { return bIsActivated; }

	// --- 사망 연출 ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MidBoss|Animation")
	TObjectPtr<UAnimMontage> DeathMontage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MidBoss|Flow")
	float DeathCleanupDelay = 30.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MidBoss|Flow")
	bool bAutoDropWeapon = true;

	// --- 디졸브 ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MidBoss|Dissolve")
	bool bEnableDissolve = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MidBoss|Dissolve", meta = (EditCondition = "bEnableDissolve"))
	float DissolveDuration = 3.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MidBoss|Dissolve", meta = (EditCondition = "bEnableDissolve"))
	TObjectPtr<UCurveFloat> DissolveCurve;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MidBoss|Dissolve", meta = (EditCondition = "bEnableDissolve"))
	FName DissolveParameterName = TEXT("Dissolve");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MidBoss|Sound", meta = (EditCondition = "bEnableDissolve"))
	TObjectPtr<USoundBase> DissolveSound;

	UFUNCTION(BlueprintCallable, Category = "MidBoss|Dissolve")
	void StartDissolve();

	// --- 사운드 ---
	// 거리 감쇠 설정 (nullptr이면 감쇠 없이 글로벌 재생)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MidBoss|Sound")
	TObjectPtr<USoundAttenuation> SoundAttenuationSettings;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MidBoss|Sound")
	float SoundVolume = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MidBoss|Sound")
	TObjectPtr<USoundBase> HitSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MidBoss|Sound")
	float HitVolumeMultiplier = 3.0f;

	// 피격 사운드 최소 재생 간격 (초)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MidBoss|Sound")
	float HitSoundMinInterval = 0.1f;

	// 마지막 피격 사운드 재생 시간
	double LastHitSoundTime = 0.0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MidBoss|Sound")
	TObjectPtr<USoundBase> StunSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MidBoss|Sound")
	float StunVolumeMultiplier = 1.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MidBoss|Sound")
	TObjectPtr<USoundBase> DeathSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MidBoss|Sound")
	float DeathVolumeMultiplier = 1.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MidBoss|Sound")
	TObjectPtr<USoundBase> BossBGM;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MidBoss|Sound")
	float BGMVolume = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MidBoss|Sound")
	float BGMFadeOutDuration = 3.0f;

#pragma endregion Flow

	// ============================================================
	// region: Delegates (이벤트 델리게이트)
	// ============================================================
#pragma region Delegates

	UPROPERTY(BlueprintAssignable, Category = "MidBoss|Events")
	FOnPatternCompleted OnPatternCompleted;

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

	UPROPERTY(BlueprintAssignable, Category = "MidBoss|Events")
	FOnMidBossActivated OnMidBossActivated;

	UPROPERTY(BlueprintAssignable, Category = "MidBoss|Events")
	FOnMidBossDeathFinished OnMidBossDeathFinished;

	// C++ 전용 (StateTree Task에서 Lambda 바인딩)
	FOnPatternCompletedNative OnPatternCompletedNative;

#pragma endregion Delegates

	// ============================================================
	// region: Private (내부 상태 / 내부 함수)
	// ============================================================
private:
#pragma region Private_Flow

	bool bIsActivated = false;

	void BeginDeathSequence();
	void FinishDeathSequence();

	UFUNCTION()
	void OnExternalTriggerOverlap(AActor* OverlappedActor, AActor* OtherActor);

	UFUNCTION()
	void OnIntroMontageEnded(UAnimMontage* Montage, bool bInterrupted);

	void StartBossLogic();

	UPROPERTY()
	TObjectPtr<UAudioComponent> BGMAudioComponent;

	UPROPERTY()
	TObjectPtr<UT3MidBossHPBarWidget> BossHPBarWidget;

	UPROPERTY()
	TObjectPtr<UTimelineComponent> DissolveTimeline;

	UPROPERTY()
	TArray<TObjectPtr<UMaterialInstanceDynamic>> DynamicMaterials;

	UPROPERTY()
	TObjectPtr<UCurveFloat> DefaultDissolveCurve;

	void CreateDynamicMaterials();

	UFUNCTION()
	void OnDissolveUpdate(float Value);

	UFUNCTION()
	void OnDissolveFinished();

	UFUNCTION()
	void OnDeathMontageEnded(UAnimMontage* Montage, bool bInterrupted);

#pragma endregion Private_Flow

#pragma region Private_Pattern

	FName CurrentPatternName = NAME_None;
	int32 CurrentChainIndex = 0;

	UPROPERTY()
	TMap<FName, double> PatternCooldownExpireMap;

	void PlayCurrentChainMontage();

	UFUNCTION()
	void OnMontageEnded(UAnimMontage* Montage, bool bInterrupted);

	// 체인 모드: 블렌드아웃 시작 시 다음 몽타주 겹쳐 재생
	void OnChainBlendingOut(UAnimMontage* Montage, bool bInterrupted);

	void AdvanceChainOrComplete();
	void RegisterCooldown(FName PatternName, float CooldownSeconds);
	void ResetPatternState();

	bool ShouldTriggerNotify(FName NotifyName) const;

	// 현재 체인 엔트리의 데미지/강도/타입 조회
	void GetCurrentHitData(float& OutDamage, EHitIntensity& OutIntensity, TSubclassOf<UT3DamageType_Base>& OutDamageType) const;

#pragma endregion Private_Pattern

#pragma region Private_Combat

	FTimerHandle StunTimerHandle;
	FTimerHandle ParryWindowTimerHandle;

	// 패링 성공 플래그 — SpawnProjectile 노티파이에서 검기 스킵 판정
	bool bParrySucceeded = false;

	UAnimMontage* GetDirectionalHitReactMontage(AActor* DamageCauser) const;

	void OpenParryWindow();
	void CloseParryWindow();
	void ExecuteParryCounter(AActor* ParriedAttacker);

	UFUNCTION()
	void OnWeaponHit(AActor* HitActor);

#pragma endregion Private_Combat

#pragma region Private_Movement

	bool bIsMovingToTarget = false;
	float MoveToTargetElapsed = 0.f;
	float MoveToTargetDuration = 0.f;
	FVector MoveToTargetDirection = FVector::ZeroVector;
	float MoveToTargetSpeed = 0.f;

#pragma endregion Private_Movement
	
#pragma region ExecuteRune

public:
	virtual float GetHPPercent() const override;
	
	virtual ET3MonsterType GetMonsterType() const override;
	
	virtual void ApplyBonusDamage(float BonusDamage) override;
	
private:
	ET3MonsterType MonsterType = ET3MonsterType::MiddleBoss;

#pragma endregion
};
