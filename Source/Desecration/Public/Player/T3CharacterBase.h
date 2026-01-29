// T3CharacterBase.h

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "T3PlayerInputState.h"
#include "InputActionValue.h"
#include "T3CharacterBase.generated.h"


class USpringArmComponent;
class UCameraComponent;
class UT3CombatComponent;
class UDataTable;
class UT3InventoryComponent; 
class UT3ItemUseComponent;
class UT3CharacterDataAsset;

UCLASS()
class DESECRATION_API AT3CharacterBase : public ACharacter
{
	GENERATED_BODY()

public:
AT3CharacterBase();

UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Inventory|Data") 
TObjectPtr <UDataTable> ItemDataTable;

protected:
	virtual void BeginPlay() override;
	virtual void Tick( float DeltaTime ) override;
	virtual void OnMovementModeChanged(EMovementMode PrevMovementMode, uint8 PreviousCustomMode = 0) override;
	
	UPROPERTY(EditAnywhere, Category = "Character Data")
	TObjectPtr<class UT3CharacterDataAsset> CharacterData;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	TObjectPtr<USpringArmComponent> CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	TObjectPtr<UCameraComponent> FollowCamera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat")
	TObjectPtr<UT3CombatComponent> CombatComponent;

	void ApplyCharacterData(UT3CharacterDataAsset* Data);

public:

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
	void OnAttack();
	UFUNCTION(BlueprintImplementableEvent)
	void OnHit();
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Inventory")
	TObjectPtr<UT3InventoryComponent> InventoryComponent; 
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Inventory")
	TObjectPtr<UT3ItemUseComponent> ItemUseComponent;



	// Stat 관련

protected:
	// 캐릭터 스탯 (고정값)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stat")
	float MaxHP = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stat")
	float CurrentHP;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stat")
	float AttackPower = 20.f; // 장비 착용하면 변경

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stat")
	float Defense = 10.f; // 장비 착용하면 변경

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stat")
	float MaxStamina = 100.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Stat")
	float CurrentStamina;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stat")
	float StaminaRegenRate = 25.f; // 스태미나 초당 회복량

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stat")
	float MaxMana = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stat")
	float CurrentMana;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stat")
	float CriticalChance = 0.1f;  // 크확

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stat")
	float CriticalDamage = 1.5f;  // 크뎀


public:
	// 7개 스탯 + 이동속도 Get / Set 함수

	// HP
	FORCEINLINE float GetMaxHP() const { return MaxHP; }
	FORCEINLINE float GetCurrentHP() const { return CurrentHP; }
	void SetCurrentHP(float NewHP) { CurrentHP = FMath::Clamp(NewHP, 0.f, MaxHP); }

	// Mana
	FORCEINLINE float GetMaxMana() const { return MaxMana; }
	FORCEINLINE float GetCurrentMana() const { return CurrentMana; }
	void SetCurrentMana(float NewMana) { CurrentMana = FMath::Clamp(NewMana, 0.f, MaxMana); }


	// Stamina
	FORCEINLINE float GetMaxStamina() const { return MaxStamina; }
	FORCEINLINE float GetCurrentStamina() const { return CurrentStamina; }
	void SetCurrentStamina(float NewStamina) { CurrentStamina = FMath::Clamp(NewStamina, 0.f, MaxStamina); }
	bool bCanRegenStamina = true;

	// Attack
	FORCEINLINE float GetAttackPower() const { return AttackPower; }
	FORCEINLINE void SetAttackPower(float NewPower) { AttackPower = NewPower; }

	// Defense
	FORCEINLINE float GetDefense() const { return Defense; }
	FORCEINLINE void SetDefense(float NewDefense) { Defense = NewDefense; }

	// Critical
	FORCEINLINE float GetCriticalChance() const { return CriticalChance; }
	FORCEINLINE void SetCriticalChance(float NewChance) { CriticalChance = NewChance; }
	FORCEINLINE float GetCriticalDamage() const { return CriticalDamage; }
	FORCEINLINE void SetCriticalDamage(float NewDamage) { CriticalDamage = NewDamage; }

	// Speed
	UFUNCTION(BlueprintCallable, Category = "Stat")
	FORCEINLINE float GetMoveSpeed() const;

	UFUNCTION(BlueprintCallable, Category = "Stat")
	void SetMoveSpeed(float NewSpeed);

	// 이동 속도 버프 함수
	UFUNCTION(BlueprintCallable, Category = "Stat")
	void SetMoveSpeedTemporary(float NewSpeedMultiflier, float Duration);


	// 액티브 회복 함수

	// 체력 회복 (HealAmount만큼 증가)
	UFUNCTION(BlueprintCallable, Category = "Stat")
	void RestoreHP(float HealAmount);

	// 마나 회복 (Amount만큼 증가)
	UFUNCTION(BlueprintCallable, Category = "Stat")
	void RestoreMP(float Amount);

protected:
	// 스태미나 자연 회복
	void RegenerateStamina();
	// 스테미나 회복 주기
	float StaminaRegenInterval = 0.1f;
	FTimerHandle StaminaRegenTimerHandle;
	
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


	virtual float TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* InstigatedBy, AActor* DamageCauser) override;
};
