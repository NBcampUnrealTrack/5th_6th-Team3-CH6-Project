// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "GameFramework/Actor.h"
#include "Interface/T3Monster.h"
#include "Player/T3LockOnTarget.h"
#include "T3BossMonster.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FBossHitDelegate);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FBossStunDelegate);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FBossDeathDelegate);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FBossDamagedDelegate);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FBossSpawnedDelegate);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FBoss25perDelegate);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FBoss50perDelegate);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FBoss75perDelegate);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FBossActionSpeedChanged);

USTRUCT(BlueprintType)
struct FBossMonsterStats
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite) float MaxHP = 2500.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite) float CurrentHP = 2500.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite) float AttackPower = 25.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float StunThreshold = 100.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float CurrentStunGauge = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float MoveSpeed = 1.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float AttackSpeed = 1.f;
};

UCLASS()
class DESECRATION_API AT3BossMonster : public ACharacter, public IT3LockOnTarget, public IT3Monster
{
	GENERATED_BODY()

protected:
	virtual void BeginPlay() override;

public:
	UPROPERTY(BlueprintReadWrite, Category = "AI")
	AActor* CombatTarget;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
	bool bBossStun = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
	bool bSuperPattern = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
	FString BossName = "DefaultName";

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
	float CurrentAttackRate = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
	FBossMonsterStats BossStats;

	UPROPERTY(BlueprintAssignable, Category = "Events")
	FBossHitDelegate OnBossHit;

	UPROPERTY(BlueprintAssignable, Category = "Events")
	FBossStunDelegate OnBossStun;

	UPROPERTY(BlueprintAssignable, Category = "Events")
	FBossStunDelegate OnBossDeath;

	UPROPERTY(BlueprintAssignable, Category = "Events")
	FBossDamagedDelegate OnBossDamaged;

	UPROPERTY(BlueprintAssignable, Category = "Events")
	FBossSpawnedDelegate OnBossSpawned;

	UPROPERTY(BlueprintAssignable, Category = "Events")
	FBoss25perDelegate OnBoss25per;

	UPROPERTY(BlueprintAssignable, Category = "Events")
	FBoss50perDelegate OnBoss50per;

	UPROPERTY(BlueprintAssignable, Category = "Events")
	FBoss75perDelegate OnBoss75per;

	UPROPERTY(BlueprintAssignable, Category = "Events")
	FBossActionSpeedChanged OnBossActionSpeedChanged;

	UFUNCTION(BlueprintCallable, Category = "Combat")
	void Damage(float DamageAmount, float StunAmount);

	virtual float TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;



protected:
	// 록온 위젯을 담을 컴포넌트
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UI")
	class UWidgetComponent* LockOnWidgetComponent;

public:
	AT3BossMonster();
	virtual void SetLockOnWidgetVisible(bool bVisible) override;
	
#pragma region ExecuteRune
public:
	virtual float GetHPPercent() const override;
	
	virtual ET3MonsterType GetMonsterType() const override;
	
	virtual void ApplyBonusDamage(float BonusDamage) override;
	
private:
	ET3MonsterType MonsterType = ET3MonsterType::Boss;
	
#pragma endregion

#pragma region 장신구

public:
	virtual void SetAnimationSpeedMultiplier(float MoveAnimMultiplier, float AttackAnimMultiplier) override;
	
#pragma endregion
};
