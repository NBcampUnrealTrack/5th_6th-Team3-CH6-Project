// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "T3EquipmentTypes.generated.h"

/**
 * 
 */

class UT3RuneLogicBase; // 전방 선언

// 장비 종류를 구분하는 딱지(Tag)
UENUM(BlueprintType)
enum class ET3EquipmentType : uint8
{
	Weapon,
	Armor
};

UENUM(BlueprintType)
enum class ET3RuneStatType : uint8
{
	None,       // 특수 룬용 (스탯 관여 안 함)
	Attack,
	Defense,
	// 필요 시 Health, Speed 등 추가
};


// 무기 강화 레벨별 데이터
USTRUCT(BlueprintType)
struct FT3WeaponLevelData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float FixedAttackPower = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSoftClassPtr<AActor> VisualEffectClass; // 강화 시 이펙트 액터
};

// 방어구 강화 레벨별 데이터
USTRUCT(BlueprintType)
struct FT3ArmorLevelData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float FixedDefensePower = 0.0f;

	// 방어구는 이펙트 액터 대신 재질(Material)이 빛나는 경우가 많음
	// 예시: UPROPERTY(EditAnywhere) TSoftObjectPtr<UMaterialInterface> GlowMaterial;
};

// 무기 통합 테이블 (Base + Growth)
USTRUCT(BlueprintType)
struct FT3WeaponDataRow : public FTableRowBase
{
	GENERATED_BODY()

	// === Base 정보 ===
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Base")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Base")
	TObjectPtr<UTexture2D> Icon = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Base")
	float BaseAttackPower = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Base")
	TSoftObjectPtr<UStaticMesh> Mesh;

	// === Growth 정보 ===
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Growth")
	TArray<FT3WeaponLevelData> LevelStats;
};

// 방어구 통합 테이블 (Base + Growth)
USTRUCT(BlueprintType)
struct FT3ArmorDataRow : public FTableRowBase
{
	GENERATED_BODY()

	// === Base 정보 ===
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Base")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Base")
	TObjectPtr<UTexture2D> Icon = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Base")
	float BaseDefensePower = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Base")
	TSoftObjectPtr<USkeletalMesh> Mesh;

	// === Growth 정보 ===
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Growth")
	TArray<FT3ArmorLevelData> LevelStats;
};

USTRUCT(BlueprintType)
struct FT3ItemSaveData
{
	GENERATED_BODY()

	UPROPERTY()
	FName ItemID;

	UPROPERTY()
	int32 Level;

	UPROPERTY()
	ET3EquipmentType Type;
};

// 2. 룬 데이터 테이블 구조체 (DT_Runes)
USTRUCT(BlueprintType)
struct FT3RuneDataRow : public FTableRowBase
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FText RuneName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<UTexture2D> Icon = nullptr;

	// [데이터] 어떤 스탯을 올려주는가? (Inject 대상)
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	ET3RuneStatType StatType = ET3RuneStatType::None;

	// [데이터] 얼마나 올려주는가? (Inject 대상)
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float StatValue = 0.0f;

	// [로직] 행동을 정의할 클래스 (BP)
	// 일반 스탯 룬 -> BP_StandardRune (기본 로직)
	// 특수 룬 -> BP_GodRune (커스텀 로직)
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSubclassOf<UT3RuneLogicBase> RuneLogicClass;
};
