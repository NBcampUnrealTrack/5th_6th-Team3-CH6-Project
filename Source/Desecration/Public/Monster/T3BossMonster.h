// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "GameFramework/Actor.h"
#include "T3BossMonster.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FBossHitDelegate);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FBossStunDelegate);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FBossDeathDelegate);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FBossDamagedDelegate);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FBossSpawnedDelegate);

USTRUCT(BlueprintType)
struct FBossMonsterStats
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite) float MaxHP = 2500.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite) float CurrentHP = 2500.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite) float AttackPower = 25.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float StunThreshold = 100.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float CurrentStunGauge = 0.f;
};

UCLASS()
class DESECRATION_API AT3BossMonster : public ACharacter
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

	UFUNCTION(BlueprintCallable, Category = "Combat")
	void Damage(float DamageAmount, float StunAmount);

	virtual float TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;

};
