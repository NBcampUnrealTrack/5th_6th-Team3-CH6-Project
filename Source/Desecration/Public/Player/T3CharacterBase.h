// T3CharacterBase.h

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "T3PlayerInputState.h"
#include "InputActionValue.h"
#include "Item/Data/T3ItemBaseData.h"
#include "T3CharacterBase.generated.h"


class USpringArmComponent;
class UCameraComponent;
class UT3CombatComponent;
class UDataTable;
class UT3InventoryComponent; 
class UT3PlayerEquipmentComponent;
class UT3ItemUseComponent;
class UT3CharacterDataAsset;


UENUM(BlueprintType)
enum class ET3StatType : uint8
{
	HP,
	MP,
	Stamina,
	Attack,
	Defense,
	CriticalChance,
	CriticalDamage,
	MoveSpeed
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnStatChangedDelegate, ET3StatType, StatType, float, CurrentValue, float, MaxValue);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnForcedMoveEndSignature);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnSellItemRequested, const FInventorySlot&, SlotData, const int32&, Count, EItemType, ItemType);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnUndyingTriggered);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDamageDealt, AActor*, HitTarget);

UCLASS()
class DESECRATION_API AT3CharacterBase : public ACharacter
{
	GENERATED_BODY()

public:
AT3CharacterBase();

// 캐릭터 스탯 델리게이트 바인딩 함수
UPROPERTY(BlueprintAssignable, Category = "Stat | Events")
FOnStatChangedDelegate OnStatChanged;

// 강제 이동 완료 델리게이트 바인딩 함수
UPROPERTY(BlueprintAssignable, Category = "Events")
FOnForcedMoveEndSignature OnForcedMoveEnd;

// 아이템 관련 델리게이트 바인딩 함수
UFUNCTION(BlueprintCallable)
void RequestSellItem(const FInventorySlot& SlotData, const int32& Count = 1, EItemType ItemType = EItemType::None);
UPROPERTY(BlueprintAssignable)
FOnSellItemRequested OnSellItemRequested;

UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Inventory|Data") 
TObjectPtr <UDataTable> ItemDataTable;

protected:
	virtual void BeginPlay() override;
	virtual void PostInitializeComponents() override; // BeginPlay보다 앞선 초기화 지점
	virtual void Tick( float DeltaTime ) override;
	virtual void OnMovementModeChanged(EMovementMode PrevMovementMode, uint8 PreviousCustomMode = 0) override;
	
	// 스탯 변경 시 내부적으로 델리게이트를 호출해주는 헬퍼 함수
	void BroadcastStatChange(ET3StatType StatType);

	UPROPERTY(EditAnywhere, Category = "Character Data")
	TObjectPtr<class UT3CharacterDataAsset> CharacterData;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	TObjectPtr<USpringArmComponent> CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	TObjectPtr<UCameraComponent> FollowCamera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat")
	TObjectPtr<UT3CombatComponent> CombatComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "State")
	bool bMoveLock = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "State")
	bool bCameraLock = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "State")
	bool bIsKnockback = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "State")
	bool bIsLying = false;
	
	void ApplyCharacterData(UT3CharacterDataAsset* Data);


public:

	// 행동 가능 여부 판단 함수
	bool CanExecuteAction() const;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PlayerInputState")
	FT3PlayerInputState PlayerInputState;
	

	FORCEINLINE TObjectPtr <USpringArmComponent> GetCameraBoom() const { return CameraBoom; }
	FORCEINLINE TObjectPtr <UCameraComponent> GetFollowCamera() const { return FollowCamera; }
	FORCEINLINE TObjectPtr <UT3CombatComponent> GetCombatComponent() const { return CombatComponent; }

	void Move(const FVector2D& Value);
	void Look(const FVector2D& Value);
	void Roll(const FInputActionValue& Value);
	UFUNCTION(BlueprintImplementableEvent)
	void OnRollTriggered();
	UFUNCTION(BlueprintImplementableEvent)
	void OnWakeUp();
	UFUNCTION(BlueprintImplementableEvent)
	void OnAttack();
	UFUNCTION(BlueprintImplementableEvent)
	void OnHit();
	UFUNCTION(BlueprintCallable, Category = "Death")
	void OnDeath();
	UFUNCTION(BlueprintImplementableEvent, Category = "Item")
	void OnActivatePotion();
	UFUNCTION(BlueprintImplementableEvent)
	void OnInteract();
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Death")
	bool bIsDead = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	bool bIsUsingItem = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SuperArmor")
	bool bIsSuperArmor = false;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Inventory")
	TObjectPtr<UT3InventoryComponent> InventoryComponent; 
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Inventory")
	TObjectPtr<UT3ItemUseComponent> ItemUseComponent;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Inventory")
	TObjectPtr<UT3PlayerEquipmentComponent> EquipComp;

	UFUNCTION(BlueprintCallable, Category = "Equipment")
	void OnEquipmentStatsUpdated(float Atk, float Def);


	// Stat 관련

protected:
	// 캐릭터 스탯 (고정값)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stat")
	float MaxHP = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stat")
	float CurrentHP;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stat")
	float AttackPower = 50.f; // 장비 착용하면 변경

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stat")
	float Defense = 0.1f; // 장비 착용하면 변경

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stat")
	float MaxStamina = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stat")
	float CurrentStamina;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stat")
	float StaminaRegenRate = 25.f; // 스태미나 초당 회복량

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stat")
	float StaminaRegenLowRate = 5.f; // 스태미나 초당 회복량

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stat")
	float MaxMana = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stat")
	float CurrentMana;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stat")
	float CriticalChance = 0.1f;  // 크확

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stat")
	float CriticalDamage = 1.5f;  // 크뎀
	
	UFUNCTION(BlueprintImplementableEvent, Category = "Death")
	void OnDeathAnimation();

public:
	// 7개 스탯 + 이동속도 Get / Set 함수

	// HP
	FORCEINLINE float GetMaxHP() const { return MaxHP; }
	FORCEINLINE float GetCurrentHP() const { return CurrentHP; }
	void SetCurrentHP(float NewHP) { CurrentHP = FMath::Clamp(NewHP, 0.f, MaxHP); BroadcastStatChange(ET3StatType::HP);}
	void SetMaxHP(float NewHP) { MaxHP = NewHP; BroadcastStatChange(ET3StatType::HP); }

	// Mana
	FORCEINLINE float GetMaxMana() const { return MaxMana; }
	FORCEINLINE float GetCurrentMana() const { return CurrentMana; }
	void SetCurrentMana(float NewMana) { CurrentMana = FMath::Clamp(NewMana, 0.f, MaxMana); BroadcastStatChange(ET3StatType::MP);}
	void ConsumeMana(float Amount);


	// Stamina
	FORCEINLINE float GetMaxStamina() const { return MaxStamina; }
	FORCEINLINE float GetCurrentStamina() const { return CurrentStamina; }
	void SetCurrentStamina(float NewStamina) { CurrentStamina = FMath::Clamp(NewStamina, 0.f, MaxStamina); BroadcastStatChange(ET3StatType::Stamina);}
	bool bCanRegenStamina = true;

	// Attack
	UFUNCTION(BlueprintCallable, Category = "Stat")
	virtual float GetAttackPower() const { return AttackPower + CachedRuneAttackBonus; }
	FORCEINLINE void SetAttackPower(float NewPower) { AttackPower = NewPower; BroadcastStatChange(ET3StatType::Attack);}

	// Defense
	FORCEINLINE float GetDefense() const { return Defense; }
	FORCEINLINE void SetDefense(float NewDefense) { Defense = NewDefense; BroadcastStatChange(ET3StatType::Defense);}

	// Critical
	FORCEINLINE float GetCriticalChance() const { return CriticalChance; }
	FORCEINLINE void SetCriticalChance(float NewChance) { CriticalChance = NewChance; BroadcastStatChange(ET3StatType::CriticalChance);	}
	FORCEINLINE float GetCriticalDamage() const { return CriticalDamage; }
	FORCEINLINE void SetCriticalDamage(float NewDamage) { CriticalDamage = NewDamage; BroadcastStatChange(ET3StatType::CriticalDamage);	}

	// Speed
	UFUNCTION(BlueprintCallable, Category = "Stat")
	float GetMoveSpeed() const;

	UFUNCTION(BlueprintCallable, Category = "Stat")
	void SetMoveSpeed(float NewSpeed);


	// 액티브 회복 함수

	// 체력 회복 (HealAmount만큼 증가)
	UFUNCTION(BlueprintCallable, Category = "Stat")
	void RestoreHP(float HealAmount);

	// 마나 회복 (Amount만큼 증가)
	UFUNCTION(BlueprintCallable, Category = "Stat")
	void RestoreMP(float Amount);

	// 클래스
	FORCEINLINE ECharacterClass GetCurrentClass() const { return CurrentClass; }

protected:

	ECharacterClass CurrentClass;

	// 스태미나 자연 회복
	void RegenerateStamina();
	// 스테미나 회복 주기
	float StaminaRegenInterval = 0.1f;
	FTimerHandle StaminaRegenTimerHandle;
	
	public:
	UFUNCTION(BlueprintPure)
	ERollDirection GetRollDirection(float Angle) const;

private:
	// 내부 수치 계산 및 제한(Clamp)용 로직
	void AddHP(float Amount);
	void AddMP(float Amount);
	void AddStamina(float Amount);

	// 속도 복구용 함수
	void ResetMoveSpeed();
	FTimerHandle SpeedResetTimerHandle;
	// 복구할 원본 속도 저장
	float OriginalMoveSpeed;



	//사망 후 이 시간이 지나면 게임 로드 실행 (단위 : 초)
	UPROPERTY(EditAnywhere, Category = "Death")
	float LoadTimeAfterDeath;
	//사망 후 게임 로드용 핸들
	FTimerHandle AfterDeathTimerHandle;

	// 강제 이동 구현
	protected:
		// 강제 이동 관련 변수
		UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ForceMove")
		bool bIsForcedMoving = false;

		FVector ForcedTargetLocation;
		FRotator ForcedTargetRotation;
		float ForcedMoveSpeed = 200.f;
		float DefaultMaxWalkSpeed = 500.f;
		bool bIsRotatingToTarget = false;

	virtual float TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* InstigatedBy, AActor* DamageCauser) override;
public:
	// 툴에서 호출할 함수 (좌표를 인자로 받음)
	UFUNCTION(BlueprintCallable, Category = "Tool")
	void StartForcedMove(FVector TargetLocation, FRotator TargetRotation, float Speed = 200.f);

	void StopForcedMove();

	void UpdateForcedMovement(float DeltaTime);
	void UpdateForcedRotation(float DeltaTime);

	//스킬 사용 불가 변수
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ForceMove")
	bool bIsSkillCanNotUse = false;

#pragma region Rune
public:
	FOnUndyingTriggered OnUndyingTriggered;
	
	FOnDamageDealt OnDamageDealt;
	
    void SetRuneAttackBonus(UObject* RuneSource, float Bonus);
    
	void RemoveRuneAttackBonus(UObject* RuneSource);

	void SetPotionUsePlayRate(float NewPlayRate);
	
	void SetEvasionPlayRate(float NewPlayRate);

	FORCEINLINE bool GetIsUndyingState() const { return bIsUndyingState; }
	
	void SetIsUndyingState(bool NewState);

	FORCEINLINE float GetSmiteMultiplier() { return SmiteMultiplier; }
	
	void SetSmiteMultiplier(float NewMultiplier);

	FORCEINLINE int32 GetSmiteThreshold() const { return SmiteThreshold; }

	void SetSmiteThreshold(int32 NewThreshold);
	
	void IncrementSmiteCounter();

	FORCEINLINE int32 GetSmiteCounter() const { return SmiteCounter; }

	void SetSmiteCounter(int32 NewCount);

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rune")
	float PotionUsePlayRate = 1.0f;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rune")
	float EvasionPlayRate = 1.0f;
	
private:
    TMap<TObjectPtr<UObject>, float> RuneAttackBonusMap;
	
    float CachedRuneAttackBonus = 0.f;
	
	uint8 bIsUndyingState : 1 = false;
	
	float SmiteMultiplier = 1.f;
	
	UPROPERTY(EditDefaultsOnly, Category ="Rune")
	int32 SmiteThreshold = 0;
	
	int32 SmiteCounter = 0;

	void RecalculateRuneBonus();
	
#pragma endregion
};
