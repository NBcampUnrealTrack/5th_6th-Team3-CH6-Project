// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "GameFramework/Actor.h"
#include "T3BossMonster.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FBossHitDelegate);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FBossStunDelegate);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FBossDeathDelegate);

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

public:
	UPROPERTY(BlueprintReadWrite, Category = "AI")
	AActor* CombatTarget;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
	bool bBossStun = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
	FBossMonsterStats BossStats;

	UPROPERTY(BlueprintAssignable, Category = "Events")
	FBossHitDelegate OnBossHit;

	UPROPERTY(BlueprintAssignable, Category = "Events")
	FBossStunDelegate OnBossStun;

	UPROPERTY(BlueprintAssignable, Category = "Events")
	FBossStunDelegate OnBossDeath;


	UFUNCTION(BlueprintCallable, Category = "Combat")
	void Damage(float DamageAmount, float StunAmount);

};
