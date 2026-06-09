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
class UT3MidBossMaterialSet;
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
// 회피(롤) i-frame 구간 — TakeDamage에서 데미지 0 처리
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Boss_State_Invulnerable);
// 막기(블록) 자세 — TakeDamage에서 정면/후방 배율로 데미지 경감
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Boss_State_Blocking);
// 막기 직후 짧은 윈도우 (~0.5초) — Consideration이 리액션 패턴(StartSectionIndex>0 + ReactionPlayRateMultiplier>1) 가중치 부풀림
// (※ 기존 ParryWindow 카운터 패턴과 무관 — 그쪽은 원거리 공격 모방 시 근접 접근하는 별도 스킬)
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Boss_State_PostBlock);
// 회피 직후 짧은 윈도우 (~0.5초) — Consideration이 리액션 패턴 가중치 부풀림 (※ 기존 카운터 패턴과 무관)
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Boss_State_PostRoll);
// 플레이어가 보스 공격을 패링 성공했을 때 외부 알림
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnParriedByPlayer);

// StateTree 이벤트 태그 (extern — STNodes에서 참조)
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Boss_Event_StunRecovered);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Boss_Event_ActionCountDepleted);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Boss_Event_ParriedByPlayer);
// 막기 중 피격 + 리액션 확률 굴림 성공 시 송신 — ST에서 빠른 반격/회피로 트랜지션
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Boss_Event_BlockReaction);

// ============================================================
// 리액션 트리거 소스 — Roll/Block 정상 종료로 "다음 공격은 리액션 모드" 플래그가 설정된 출처.
// ExecutePattern 성공 시 None으로 소모됨. 시간 기반 만료 없음 (다음 공격 1회 = 1소모).
// ============================================================
UENUM(BlueprintType)
enum class EBossReactionSource : uint8
{
	None		UMETA(DisplayName = "None"),
	FromBlock	UMETA(DisplayName = "After Block"),
	FromRoll	UMETA(DisplayName = "After Roll"),
};

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

	// 일정 시간 후 자동 제거되는 상태 태그 부여 (PostBlock/PostRoll 같은 짧은 윈도우용)
	// 같은 태그가 이미 활성 상태면 타이머만 갱신(연장). Duration<=0이면 즉시 부여 후 다음 틱 제거.
	UFUNCTION(BlueprintCallable, Category = "MidBoss|State")
	void ApplyTransientStateTag(FGameplayTag Tag, float Duration);

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

	// 롤 i-frame 활성 여부 (몽타주의 ANS_BossInvulnerable이 토글)
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "MidBoss|State")
	bool IsInvulnerable() const;

	// 막기 자세 활성 여부 (Block STT 노드가 토글)
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "MidBoss|State")
	bool IsBlocking() const;

	// 피격이 보스 정면(앞 반구)에서 들어왔는지 — 막기 정면/후방 배율 분기용
	// DamageCauser nullptr 또는 좌표 동일 시 true 반환 (안전 기본값)
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "MidBoss|Combat")
	bool IsHitFromFront(const AActor* DamageCauser) const;

	// --- 막기 시퀀스 (STT_Block에서 트리거) ---
	// In → Loop(자기 자신 무한) → 외부 RequestEnd → Out → Idle
	// 단계 전환은 OnBlockMontageBlendingOut 콜백이 BlendingOut 시점에 다음 PlayAnimMontage 호출 → 자연 크로스페이드
	// CurrentBlockPhase / bBlockEndRequested는 BlueprintReadOnly로 노출되어 STT가 폴링

	// 막기 시퀀스 시작 — InEntry 재생 + BlendingOut 콜백 등록 + Phase=In + BlockHitsCount 리셋
	UFUNCTION(BlueprintCallable, Category = "MidBoss|Defense")
	void StartBlockSequence();

	// 종료 요청 — bBlockEndRequested=true 만 set. 다음 BlendingOut 시점에 OnBlockMontageBlendingOut이 Out 진입
	UFUNCTION(BlueprintCallable, Category = "MidBoss|Defense")
	void RequestEndBlockSequence();

	// 외부 인터럽트(BlockReaction 이벤트, 사망 등) — 즉시 정리. Stop + 태그 제거 + Phase=Idle
	UFUNCTION(BlueprintCallable, Category = "MidBoss|Defense")
	void StopBlockSequence();

	// --- 보스 정보 & 스탯 ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MidBoss|Info")
	FText BossDisplayName = FText::FromString(TEXT("MidBoss"));

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MidBoss|Stats")
	FMidBossStats MidBossStats;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MidBoss|Stage")
	int32 BossStage = 1;

	// --- 무기 ---
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "MidBoss|Weapon")
	TObjectPtr<UT3BossWeaponComponent> WeaponComponent;

	// --- 머티리얼 세트 적용 ---
	UFUNCTION(BlueprintCallable, Category = "MidBoss|Visual")
	void ApplyMaterialSet(const UT3MidBossMaterialSet* MaterialSet);

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

	// bAsReaction=true면 PostBlock/PostRoll 직후 "리액션 패턴" 모드로 실행 — ReactionStartSectionOverride 적용.
	// (※ ParryWindow 카운터 패턴과 무관 — 별도 메커니즘)
	UFUNCTION(BlueprintCallable, Category = "MidBoss|Pattern")
	bool ExecutePattern(FName PatternName, bool bAsReaction = false);

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

	// --- 막기 데미지 배율 ---
	// 정면 피격 시 들어오는 데미지 배율 (0 = 완전 무효, 0.1 = 90% 경감, 1 = 풀데미지)
	// Unparryable / Unblockable 공격은 이 배율 무시하고 풀데미지
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MidBoss|Defense", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float BlockDamageMultiplier_Front = 0.1f;

	// 후방(뒤 반구) 피격 시 들어오는 데미지 배율 — 뒤잡 메커니즘 도입 전까지는 정면과 같은 값 사용 권장
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MidBoss|Defense", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float BlockDamageMultiplier_Back = 0.1f;

	// 막기 중 피격 시 리액션 발동 확률 (0 = 비활성, 1 = 항상 발동)
	// 발동 성공 시 Boss.Event.BlockReaction 이벤트 송신 → ST에서 빠른 반격 패턴으로 트랜지션
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MidBoss|Defense", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float BlockReactionChance = 0.0f;

	UFUNCTION(BlueprintCallable, Category = "MidBoss|Combat")
	void PlayAdditiveHitReaction(AActor* DamageCauser = nullptr);

	UFUNCTION(BlueprintCallable, Category = "MidBoss|Combat")
	void ApplyStun();

	UFUNCTION(BlueprintCallable, Category = "MidBoss|Combat")
	void RecoverFromStun();

	// --- AoE (장판기) ---
	// DamageType 미지정(nullptr) 시 UT3DamageType_Base로 fallback — BP 하위호환
	UFUNCTION(BlueprintCallable, Category = "MidBoss|Combat")
	void ExecuteAoEDamage(float Radius, float DamageAmount, EHitIntensity Intensity = EHitIntensity::Heavy,
		TSubclassOf<UT3DamageType_Base> DamageType = nullptr);

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

	// --- 백스텝 (Disengage ST Task에서 참조) ---
	// 보스별 스켈레톤이 다르므로 캐릭터에 두고 ST는 Boss->BackStepMontage 참조
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MidBoss|Animation")
	TObjectPtr<UAnimMontage> BackStepMontage;

	// --- 8방향 회피 (TestRoll ST Task에서 참조) ---
	// 인덱스 순서: 0=0°(F) / 1=45°(FR) / 2=90°(R) / 3=135°(BR) / 4=180°(B) / 5=225°(BL) / 6=270°(L) / 7=315°(FL)
	// 보스별 스켈레톤/애셋이 다르므로 캐릭터에 두고 ST는 Boss->RollMontages_8Dir 참조 (BackStepMontage와 동일 패턴)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MidBoss|Animation")
	TArray<TObjectPtr<UAnimMontage>> RollMontages_8Dir;

	// 방향별 루트모션 거리 배율 — 인덱스는 RollMontages_8Dir와 동일
	// 비어있거나 인덱스 미존재 시 1.0 처리 (앞구르기는 크게, 뒷/옆구르기는 짧게 조절용)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MidBoss|Animation")
	TArray<float> RollDirectionScales;

	// --- 막기 (Block STT Task에서 참조) ---
	// 보스별 스켈레톤/애셋이 다르므로 캐릭터에 두고 STT는 Boss->BlockMontageData 참조 (RollMontages_8Dir와 동일 패턴)
	// In/Loop/Out 3 Entry 슬롯 — 같은 몽타주 다른 섹션이든 짜집기든 자유 (단계 전환은 BlendingOut 자연 크로스페이드)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MidBoss|Animation")
	FBlockMontageData BlockMontageData;

	// 현재 막기 자세 진입 후 막아낸 피격 횟수 (Block STT EnterState에서 캐시, TakeDamage에서 막기 성공 시 ++)
	UPROPERTY(BlueprintReadOnly, Category = "MidBoss|Defense")
	int32 BlockHitsCount = 0;

	// 현재 막기 시퀀스 단계 — STT_Block이 폴링하여 Idle 도달 시 Succeeded 처리
	UPROPERTY(BlueprintReadOnly, Category = "MidBoss|Defense")
	EBlockPhase CurrentBlockPhase = EBlockPhase::Idle;

	// 종료 요청 플래그 — RequestEndBlockSequence가 set, 다음 BlendingOut 시점에 Out으로 전환
	UPROPERTY(BlueprintReadOnly, Category = "MidBoss|Defense")
	bool bBlockEndRequested = false;

	// 현재 활성 막기 몽타주 캐시 — PlayBlockEntry에서 set / StopBlockSequence가 이 몽타주만 외과적으로 정지
	// (StopAllMontages 사용 시 동시에 진입한 다른 몽타주(예: 스턴 진입)까지 죽이는 부작용 방지)
	UPROPERTY()
	TObjectPtr<UAnimMontage> CurrentBlockMontage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MidBoss|Combat")
	float StunDuration = 3.0f;

	// --- 경직치 게이지 (스테미너형) ---
	// 다크나이트는 4개 값 모두 0으로 두면 자동 비활성. 데스몬드는 값 채워서 활성
	// 누적 경로: ① Roll STT 진입(StaggerOnRoll) ② 막기 중 정면 피격(StaggerOnBlockedHit)
	// 임계 도달 시 ApplyStun() 자동 호출 → 진입 시 CurrentStunGauge 0 초기화 (기존 인프라 재활용)

	// Roll STT 1회 진입당 누적량 — 0이면 회피로 게이지 안 쌓음
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MidBoss|Stagger")
	float StaggerOnRoll = 0.f;

	// 막기 중 정면 피격 1회당 누적량 — TakeDamage StunAmount와 별도로 추가 누적 (가드는 HP 대신 게이지를 깎음)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MidBoss|Stagger")
	float StaggerOnBlockedHit = 0.f;

	// 초당 게이지 회복량 — 0이면 회복 없음 (다크나이트 기본값)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MidBoss|Stagger")
	float StaggerRecoveryRate = 0.f;

	// 마지막 누적 이벤트로부터 회복 시작까지 대기 시간(초)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MidBoss|Stagger")
	float StaggerRecoveryDelay = 2.5f;

	// 마지막 누적 시점 — Tick 회복 분기에서 Delay 비교용
	UPROPERTY(BlueprintReadOnly, Category = "MidBoss|Stagger")
	double LastStaggerEventTime = 0.0;

	// 경직치 누적 헬퍼 — 누적/임계체크/시간갱신 일원화
	// IsStunned/IsDead 가드 + Amount<=0 early-out 포함. 임계 도달 시 ApplyStun() 자동 호출
	UFUNCTION(BlueprintCallable, Category = "MidBoss|Stagger")
	void AddStunGauge(float Amount);

	// [DEBUG:StaggerGauge] 디자이너 튜닝용 임시 디버그. 제거 시 `[DEBUG:StaggerGauge]` 태그 grep 후 일괄 삭제
	// 보스 머리 위에 경직치 게이지 텍스트 + 비율 바 매 틱 표시. 출시 전 토글 OFF 또는 코드 제거.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MidBoss|Stagger")
	bool bShowDebugStaggerGauge = false;

	// 리액션 트리거 — Roll/Block 정상 종료 시 세팅, 다음 ExecutePattern 1회로 소모.
	// 시간 만료 없음 — 다음 공격이 발사되기 전까지 유지. ReactionWindow Consideration / ExecutePattern 자동감지가 이 필드를 읽음.
	UPROPERTY(BlueprintReadOnly, Category = "MidBoss|Reaction")
	EBossReactionSource PendingReactionSource = EBossReactionSource::None;

	// 리액션 윈도우 활성 시(=PendingReactionSource != None) 다음 패턴을 리액션 모드로 실행할 확률 [0..1].
	// ExecutePattern 진입 시점에 FRand()로 1회 판정. 0=항상 일반, 1=항상 리액션, 0.7=70% 리액션 / 30% 일반.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MidBoss|Reaction",
		meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float ReactionApplyChance = 0.7f;

	// [DEBUG:ReactionTest] 리액션 패턴 흐름 검증용. 제거 시 `[DEBUG:ReactionTest]` 태그 grep 후 일괄 삭제
	// 막기/회피(Roll/Block) 정상 종료 직후 DebugReactionPatternName 패턴을 bAsReaction=true로 강제 실행.
	// → ReactionStartSectionOverride / ReactionSectionNameOverride 발동 검증용. 출시 전 토글 OFF.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MidBoss|Debug")
	bool bDebugForceReactionAfterDefense = false;

	// [DEBUG:ReactionTest] 강제 실행할 패턴 이름. bAllowAsReaction=true로 BP 등록되어 있어야 의미 있음.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MidBoss|Debug",
		meta = (EditCondition = "bDebugForceReactionAfterDefense"))
	FName DebugReactionPatternName = NAME_None;

	// [DEBUG:ReactionWindow] ReactionWindow Consideration 점수 계산 가시화 토글.
	// ON 시 패턴 후보 평가마다 분기별 (일반/Boost/Idle) 로그 출력. 출시 전 토글 OFF 또는 코드 제거.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MidBoss|Debug")
	bool bDebugLogReactionWindow = false;

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
#pragma region Private_Core

	// 생성자 분해 헬퍼 — CDO 생성 시점 호출
	void ConfigureRotationSettings();
	void CreateLockOnWidget();

	// BeginPlay 분해 헬퍼 — 초기화 순서(록온 → 무기 → 스탯/태그/Modifier → 디졸브 → 트리거 → Broadcast → 로그) 유지
	void AttachLockOnWidgetToSocket();
	void BindWeaponComponent();
	void InitializeStatsAndTags();
	void SetupDissolveTimeline();

	// Tick 분해 헬퍼 — 매 프레임 호출 (MoveToTarget 보간 / AI SetFocus + 회전 속도 분기 / 경직치 회복)
	void UpdateMoveToTargetInterpolation(float DeltaTime);
	void UpdateAIFocusAndRotation();

	// StaggerRecoveryRate>0 + Stunned/Dead 아님 + 마지막 누적으로부터 Delay 경과 시 게이지 감소
	void UpdateStaggerRecovery(float DeltaTime);

	// [DEBUG:StaggerGauge] bShowDebugStaggerGauge ON 일 때 매 틱 — 머리 위 텍스트 시각화
	void DrawDebugStaggerGauge(float DeltaTime) const;

	// ApplyTransientStateTag로 부여된 태그의 자동 제거 타이머 — 태그별 1개 핸들 (재호출 시 갱신)
	TMap<FGameplayTag, FTimerHandle> TransientStateTagHandles;

#pragma endregion Private_Core

#pragma region Private_Flow

	bool bIsActivated = false;

	void BeginDeathSequence();
	void FinishDeathSequence();

	UFUNCTION()
	void OnExternalTriggerOverlap(AActor* OverlappedActor, AActor* OtherActor);

	UFUNCTION()
	void OnIntroMontageEnded(UAnimMontage* Montage, bool bInterrupted);

	void StartBossLogic();

	// ActivateBoss 분해 헬퍼 — 활성화 순서(트리거 비활성 → BGM → HPBar → Broadcast → 로그 → 인트로) 유지
	void PlayBossBGM();
	void ShowBossHPBar();
	void PlayIntroMontageOrStart();

	// 사망 후 물리/이동 차단 — 캡슐 콜리전 off + CharacterMovement DisableMovement
	void DisablePhysicsAndMovement();

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

	// 현재 실행 중인 패턴이 리액션 모드(ExecutePattern bAsReaction=true)로 진입했는지
	// PlaySectionComboFirstEntry가 ReactionSectionNameOverride 적용 여부 판단에 사용
	bool bIsCurrentPatternReaction = false;

	UPROPERTY()
	TMap<FName, double> PatternCooldownExpireMap;

	void PlayCurrentChainMontage();

	// 섹션 콤보 동적 다음 섹션 설정 — PlayCurrentChainMontage 첫 진입 / HandleNextComboNotify 공유
	void SetupDynamicNextSection(UAnimInstance* AnimInst, UAnimMontage* SectionMontage,
		const FMidBossAttackPattern& PatternData, int32 CurrentIdx) const;

	// 섹션 콤보 첫 진입 (Idx==0) — PlayAnimMontage + 동적 섹션 설정 + OnMontageEnded 바인딩
	void PlaySectionComboFirstEntry(const FMidBossAttackPattern& PatternData,
		const FPatternMontageData& MontageData);

	// 일반 체인 모드 — PlayAnimMontage + 단일 섹션 격리 + OnChainBlendingOut 바인딩
	// ReactionPlayRateMultiplier — 패턴 단위 PlayRate 가속 배율 (막기 리액션 등에서 1.0 외 값)
	void PlayChainMontageEntry(const FPatternMontageData& MontageData, float ReactionPlayRateMultiplier = 1.f);

	UFUNCTION()
	void OnMontageEnded(UAnimMontage* Montage, bool bInterrupted);

	// 체인 모드: 블렌드아웃 시작 시 다음 몽타주 겹쳐 재생
	void OnChainBlendingOut(UAnimMontage* Montage, bool bInterrupted);

	// 막기 시퀀스: BlendingOut 시점에 다음 단계 PlayAnimMontage 호출 → 자연 크로스페이드
	// In→Loop / Loop→Loop 자기루프 (bBlockEndRequested 시 Loop→Out) / Out→Idle 전환 처리
	void OnBlockMontageBlendingOut(UAnimMontage* Montage, bool bInterrupted);

	// 막기 단계 진입 헬퍼 — Entry PlayAnimMontage + BlendingOut 콜백 재등록 + Phase 갱신
	void PlayBlockEntry(const FBlockMontageEntry& Entry, EBlockPhase NewPhase);

	// OnChainBlendingOut / OnMontageEnded 공통 prelude — 종료/인터럽트 처리, 호출자 early-return 여부 반환
	bool HandleChainCallbackPrelude(bool bInterrupted, const TCHAR* InterruptLogPrefix);

	void AdvanceChainOrComplete();
	void RegisterCooldown(FName PatternName, float CooldownSeconds);
	void ResetPatternState();

	bool ShouldTriggerNotify(FName NotifyName) const;

	// ShouldTriggerNotify 1단계 — 노티파이 타입별 기본 확률 조회 (Slow/Fast/Step)
	float GetBaseNotifyChance(FName NotifyName, const FMidBossAttackPattern& PatternData) const;

	// ShouldTriggerNotify 2단계 — 거리/HP 등 NotifyModifier에 넘길 컨텍스트 빌드
	FNotifyModifierContext BuildNotifyModifierContext(FName NotifyName) const;

	// GetAvailablePatterns / GetAllAvailablePatterns 공통 필터 — Stage/쿨다운 + ExtraFilter
	TArray<FName> CollectAvailablePatterns(TFunctionRef<bool(const FMidBossAttackPattern&)> ExtraFilter) const;

	// 현재 체인 엔트리의 데미지/강도/타입 조회
	void GetCurrentHitData(float& OutDamage, EHitIntensity& OutIntensity, TSubclassOf<UT3DamageType_Base>& OutDamageType) const;

	// ============================================================
	// 노티파이 공통 헬퍼 — HandlePatternNotify에서 사용
	// ============================================================

	// 현재 체인 엔트리의 PlayRate × CurrentAttackAnimRate [× ExtraMultiplier] 적용
	void ApplyCurrentChainPlayRate(UAnimInstance* AnimInst, UAnimMontage* Montage, float ExtraMultiplier = 1.0f) const;

	// ShouldTriggerNotify + RecordNotifyResult 쌍 처리 — 발동 시 OnTriggered 람다 실행
	void ProcessProbabilisticNotify(FName NotifyName, TFunctionRef<void()> OnTriggered);

	// 무기 판정 노티파이 공통 분기 — AttackStart/End, WideAttackStart/End, BodyAttackStart/End
	void SetWeaponCollisionByNotify(FName NotifyName, bool bEnabled);

	// ============================================================
	// 노티파이 분기별 핸들러
	// ============================================================

	void HandleDropWeaponNotify();
	void HandleSlowNotify(UAnimInstance* AnimInst, UAnimMontage* Montage);
	void HandleFastNotify(UAnimInstance* AnimInst, UAnimMontage* Montage);
	void HandleNormalNotify(UAnimInstance* AnimInst, UAnimMontage* Montage);
	void HandleStepNotify(UAnimInstance* AnimInst, UAnimMontage* Montage);
	void HandleSpawnProjectileNotify();
	void HandleGroundSlamPreviewNotify();
	void HandleGroundSlamNotify();
	void HandleParryWindowNotify(bool bOpen);
	void HandleWarpTargetNotify();
	void HandleNextComboNotify(UAnimInstance* AnimInst, UAnimMontage* Montage);

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

	// 보스 사운드 공통 재생 헬퍼 — nullptr 가드 + SoundVolume × Mult + Attenuation 일원화
	void PlayBossSoundAt(USoundBase* Sound, const FVector& Loc, float VolumeMultiplier) const;

	// StateTree 이벤트 전송 헬퍼 — StateTreeComponent nullptr 가드 일원화
	// STT(FT3STT_Block::Tick의 BlockReaction 송신)에서도 호출하므로 public 노출
public:
	void SendStateTreeStateEvent(FGameplayTag Tag) const;
private:

	// 피격 피드백 묶음 — HitSound(레이트리밋) + 카메라 쉐이크 + 히트 리액션(조건부)
	void PlayHitFeedback(const FVector& HitLoc, AActor* DamageCauser);

	// 사망 상태 진입 — 태그/HP clamp/패턴 캔슬/델리게이트/StateTree 이벤트/사망 시퀀스
	void EnterDeathState();

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
	
#pragma region 장신구

public:
	virtual void SetAnimationSpeedMultiplier(float MoveAnimMultiplier, float AttackAnimMultiplier) override;

	// 외부 장신구가 적용한 공격 애님 배율 (1.0 = 영향 없음) — 패턴 PlayRate 계산에 곱해짐
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "MidBoss|Accessory")
	float GetAttackAnimRateMultiplier() const { return CurrentAttackAnimRate; }

	// 외부 장신구가 적용한 이동 애님 배율 (1.0 = 영향 없음) — BackStep 등 이동 몽타주에 곱해짐
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "MidBoss|Accessory")
	float GetMoveAnimRateMultiplier() const { return CurrentMoveAnimRate; }

	// STNodes(Strafe/Dash 등)가 베이스 이동 속도를 푸시할 때 사용 — 내부에서 MaxWalkSpeed = NewBase * MoveRate 적용
	// 호출 측은 이전 GetActiveBaseWalkSpeed() 값을 캐시해뒀다가 종료 시 다시 SetActiveBaseWalkSpeed로 복원
	UFUNCTION(BlueprintCallable, Category = "MidBoss|Movement")
	void SetActiveBaseWalkSpeed(float NewBaseSpeed);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "MidBoss|Movement")
	float GetActiveBaseWalkSpeed() const { return ActiveBaseWalkSpeed; }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "MidBoss|Movement")
	float GetDefaultMaxWalkSpeed() const { return DefaultMaxWalkSpeed; }

	// 비-공격-체인 몽타주 재생 진입점 (이동: BackStep/Roll/Dash Run, 리액션: HitReact/Stun 등)
	// BaseRate를 LastMoveBaseRate에 캐시 후 (BaseRate * CurrentMoveAnimRate)로 재생 — 슬로우 변경 시 실시간 갱신 가능
	// 호출 측은 BaseRate만 넘기고 슬로우 곱셈은 신경쓰지 말 것 (raw PlayAnimMontage 금지)
	UFUNCTION(BlueprintCallable, Category = "MidBoss|Movement")
	float PlayMoveMontageWithSlow(UAnimMontage* Montage, float BaseRate = 1.f);

private:
	// 외부 슬로우/가속 영향 (천사 장신구 등) — 절대 배율, 누적되지 않음
	float CurrentMoveAnimRate = 1.f;
	float CurrentAttackAnimRate = 1.f;

	// 슬로우 미적용 상태의 MaxWalkSpeed (BeginPlay 시 캐시) — 복원 기준값
	float DefaultMaxWalkSpeed = 0.f;

	// 현재 활성 베이스 이동 속도 (Default 또는 STNodes가 푸시한 Strafe/Dash 속도)
	// 실제 MaxWalkSpeed = ActiveBaseWalkSpeed * CurrentMoveAnimRate
	float ActiveBaseWalkSpeed = 0.f;

	// 마지막 이동 계열 몽타주의 BaseRate (PlayMoveMontageWithSlow 호출 시 캐시)
	// 슬로우 실시간 갱신 시 PlayRate = LastMoveBaseRate * CurrentMoveAnimRate 재계산용 — 누적 방지
	float LastMoveBaseRate = 1.f;

	// 헬퍼 통과한 몽타주만 슬로우 추적 — Intro/Death 등 raw PlayAnimMontage는 자동 제외 (시네마틱 보존)
	// 약참조: 몽타주 GC되거나 다른 몽타주로 교체되면 자동 무효화
	TWeakObjectPtr<UAnimMontage> SlowManagedMontage;

	// CharacterMovement->MaxWalkSpeed에 (ActiveBaseWalkSpeed * CurrentMoveAnimRate) 적용 — 단일 계산 진입점
	void ApplyCurrentWalkSpeed();

	// 진행 중 이동 계열 몽타주에 (LastMoveBaseRate * CurrentMoveAnimRate) 적용 — 단일 계산 진입점
	// SetAnimationSpeedMultiplier에서 슬로우 진입/이탈 시 즉시 갱신 위해 호출
	void ApplyCurrentMoveMontagePlayRate(UAnimInstance* AnimInst, UAnimMontage* Montage) const;

#pragma endregion
};
