// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "T3BossMonster.generated.h"

USTRUCT(BlueprintType)
struct FBossMonsterStats
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere) float MaxHP = 2500.f;
	UPROPERTY(EditAnywhere) float AttackPower = 25.f;
	UPROPERTY(EditAnywhere) float StunThreshold = 100.f;
	UPROPERTY(EditAnywhere) float CurrentStunGauge = 0.f;
};

UCLASS()
class DESECRATION_API AT3BossMonster : public ACharacter
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadWrite, Category = "AI")
	AActor* CombatTarget;

	UPROPERTY(EditAnywhere, Category = "Stats")
	FBossMonsterStats BossStats;


};
