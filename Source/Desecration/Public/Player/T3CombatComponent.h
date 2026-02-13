// T3CombatComponent.h

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "T3CharacterDataAsset.h"
#include "Player/T3DamageTypes.h"
#include "T3CombatComponent.generated.h"

class AAICharacter;

// 캐릭터 상태 ENUM
UENUM(BlueprintType)
enum class ECharacterCombatState : uint8
{
	Idle,
	Blocking,
	Parrying,
	Dodge,
	Attacking,
	Dead,
	Cooldown
};

// 노티파이용 ENUM
UENUM(BlueprintType)
enum class ECombatWindowType : uint8
{
	None,
	Parry      UMETA(DisplayName = "Parry Window"),
	Dodge UMETA(DisplayName = "Dodge Window"),
	Attack     UMETA(DisplayName = "Attack Collision"),
	PrevenRegen UMETA(DisplayName = "PrevenRegen")
	
};

// 피격 방향 ENUM
UENUM(BlueprintType)
enum class EHitDirection : uint8
{
	Front   UMETA(DisplayName = "Front"),
	Back    UMETA(DisplayName = "Back"),
	Left    UMETA(DisplayName = "Left"),
	Right   UMETA(DisplayName = "Right")
};

// 슬롯 체인지 타입 구분
UENUM(BlueprintType)
enum class ESlotType : uint8
{
	Skill,
	Consumable, // 소모아이템
	Potion
};

// 현재 선택된 슬롯이 바뀔 때 (전투 화면에서 슬롯 체인지)
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnSlotSelectionChanged, ESlotType, SlotType, int32, NewSlotIndex);


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class DESECRATION_API UT3CombatComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UT3CombatComponent();

	void InitializeWeapons(const TMap<EEquipSlot, FWeaponEquipInfo>& WeaponMap);

	void ExecuteHitLogic(AActor* DamageCauser, float Damage, const UDamageType* DamageType, AController* InstigatedBy, EHitIntensity Intensity, float ReceievedDamageMultiplier);

	void SetOwnerChar(ACharacter* InChar) { AIChar = InChar; };


	UPROPERTY(BlueprintAssignable, Category = "Combat|UI")
	FOnSlotSelectionChanged OnSlotSelectionChanged;

	UFUNCTION(BlueprintCallable)
	void RequestUpdateSkill(int32 SkillID, bool bIsEquip);

protected:
	virtual void BeginPlay() override;



public:	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EHitDirection HitDirection;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EHitIntensity HitIntensity;
	// 상태 변수
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	ECharacterCombatState CurrentState;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector CurrentDamageCauserLocation;
	
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// 막기/패링
	void StartBlock();
	void EndBlock();
	
	void Attack();

	UFUNCTION(BlueprintCallable)
	void SetParryingEnabled(bool bEnabled);

	UFUNCTION(BlueprintCallable)
	void SetDodgingEnabled(bool bEnabled);


	// 록온
	void ToggleLockOn();
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LockOn")
	float TargetHeightPercent = 0.3f;
	TObjectPtr<class USpringArmComponent> SpringArm;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LockOn")
	float DefaultArmLength = 400.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LockOn")
	float MaxArmLength = 2000.f; // 보스가 높이 뜰 때 멀어질 최대 거리

	AActor* GetCurrentTarget() const { return CurrentTarget; }

	void UpdateLockOnWidgetScale();

	// 캐릭터 상태 Getter
	FORCEINLINE ECharacterCombatState GetCurrentState() const { return CurrentState; }

	// 공격 함수
	UFUNCTION(BlueprintCallable)
	void RequestAttackDamage(AActor* TargetActor, float DamageAmount, EHitIntensity Intensity = EHitIntensity::Light, float DamageMultiflier = 1.0f , TSubclassOf<class UT3DamageType_Base> DamageTypeClass = nullptr, float InStunAmount = 0.f);

	// 스태미너 소모 함수
	void ConsumeStamina(float Amount);




	// 무기 가져오기
	UFUNCTION(BlueprintCallable, Category = "Combat")
	class AT3WeaponBase* GetWeaponBySlot(EEquipSlot Slot) const;


	// 무기 제거 로직
	void ClearWeapons();


	// ======== 스킬, 아이템 슬롯 전환 및 슬롯 실행 ==========

public:
	// 슬롯 전환 함수 (키 입력에 대응)
	UFUNCTION(BlueprintCallable, Category = "Input")
	void ChangeActiveSlot(ESlotType Type);

	// 현재 슬롯 실행 (실제 키 입력 시 호출)
	UFUNCTION(BlueprintCallable, Category = "Input")
	void ExecuteCurrentSlotAction(ESlotType Type);

	void SetSkillComponent(UT3SkillComponentBase* InSkillComp) { SkillComp = InSkillComp; }
	UFUNCTION(BlueprintCallable, Category = "Skill")
	UT3SkillComponentBase* GetSkillComponent() const { return SkillComp; }

private:
	// 현재 선택된 인덱스들
	int32 CurrentSkillSlot = 1;
	int32 CurrentConsumableSlot = 1;
	int32 CurrentPotionSlot = 1;

	// 최대 슬롯 수 (직업별 확장성 고려)
	int32 MaxSkillSlots = 2;
	int32 MaxConsumableSlots = 2;
	int32 MaxPotionSlots = 2;

	// 캐싱된 컴포넌트
	UPROPERTY()
	class UT3SkillComponentBase* SkillComp;

	UPROPERTY()
	class UT3ItemUseComponent* ItemComp;
	
	
	
	
	
	
	
	
	// 상태별 데미지 경감 로직
	float CalculateFinalDamage(float IncomingDamage, const class UDamageType* DamageType, float ReceievedDamageMultiplier);
	
	// 내부 로직용
	AActor* FindBestTarget();
	void ResetLockOn();
	void UpdateTargetUI(AActor* Target, bool bIsVisible);
	bool IsTargetVisible(AActor* Target) const;
	// void SetLockOnTarget(AActor* NewTarget);

	// 패링
	FTimerHandle ParryingToBlockingTimerHandle;
	UFUNCTION()
	void SwitchToBlockingState();

	UPROPERTY()
	TObjectPtr<class AT3CharacterBase> OwnerChar;

	UPROPERTY()
	TObjectPtr<class APlayerController> OwnerPC;

	UPROPERTY()
	TObjectPtr<class ACharacter> AIChar;

	UPROPERTY()
	TObjectPtr<class AController> AIPC;

	// 록온 변수
	bool bIsLockOn = false;

	// 록온 타깃 식별 태그
	UPROPERTY(EditAnywhere, Category = "Combat|LockOn")
	FName TargetTag = FName("Enemy");

	float SearchRadius = 2500.f;
	float InterpSpeed = 20.f;


	// 무기
	UPROPERTY()
	TMap<EEquipSlot, TObjectPtr<class AT3WeaponBase>> EquippedWeapons;

	// 방향 계산 함수
	EHitDirection CalculateHitDirection(const FVector& HitLocation);


	protected:
	// 블락 타이머 (무한 패링 방지)
	FTimerHandle BlockingCooldownTimerHandle;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Block")
	float BlockCooldownTime = 1.f;
	bool bCanBlock = true;
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
	TObjectPtr<AActor> CurrentTarget;
	void ResetBlockCooldown();


};
