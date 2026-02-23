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
#include "T3MidBossMonster.generated.h"

class UT3BossWeaponComponent;
class UT3MidBossHPBarWidget;
class UCurveFloat;
class UAudioComponent;
class UWidgetComponent;
class USphereComponent;

// 상태 태그 (extern — 분할 .cpp에서 참조)
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Boss_State_Dead);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Boss_State_Stunned);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Boss_State_ExecutingPattern);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Boss_State_SuperArmor);

// StateTree 이벤트 태그 (extern — STNodes에서 참조)
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Boss_Event_StunRecovered);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Boss_Event_ActionCountDepleted);

// ============================================================
// AT3MidBossMonster
// ============================================================

UCLASS()
class DESECRATION_API AT3MidBossMonster : public ACharacter, public IT3LockOnTarget
{
	GENERATED_BODY()

public:
	AT3MidBossMonster();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

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

	// 이 거리 초과 시 이동 워프 비활성화 (제자리 루트모션 공격)
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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MidBoss|Patterns")
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

	// --- 활성화 트리거 ---
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "MidBoss|Activation")
	TObjectPtr<USphereComponent> ActivationTriggerSphere;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MidBoss|Activation")
	float ActivationRadius = 1500.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MidBoss|Activation")
	TObjectPtr<UAnimMontage> IntroMontage;

	UFUNCTION(BlueprintCallable, Category = "MidBoss|Flow")
	void ActivateBoss(AActor* Activator);

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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MidBoss|Dissolve", meta = (EditCondition = "bEnableDissolve"))
	TObjectPtr<USoundBase> DissolveSound;

	UFUNCTION(BlueprintCallable, Category = "MidBoss|Dissolve")
	void StartDissolve();

	// --- 사운드 ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MidBoss|Sound")
	float SoundVolume = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MidBoss|Sound")
	TObjectPtr<USoundBase> HitSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MidBoss|Sound")
	float HitVolumeMultiplier = 3.0f;

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
	void OnActivationTriggerOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

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

	const FMidBossAttackPattern* FindPatternData(FName PatternName) const;
	void PlayCurrentChainMontage();

	UFUNCTION()
	void OnMontageEnded(UAnimMontage* Montage, bool bInterrupted);

	void AdvanceChainOrComplete();
	void RegisterCooldown(FName PatternName, float CooldownSeconds);
	void ResetPatternState();

	bool ShouldTriggerNotify(FName NotifyName) const;

#pragma endregion Private_Pattern

#pragma region Private_Combat

	FTimerHandle StunTimerHandle;

	UAnimMontage* GetDirectionalHitReactMontage(AActor* DamageCauser) const;

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
};
