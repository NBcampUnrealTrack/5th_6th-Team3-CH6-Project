// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "T3EquipmentTypes.generated.h"

/**
 * 
 */

// 장비 종류를 구분하는 딱지(Tag)
UENUM(BlueprintType)
enum class ET3EquipmentType : uint8
{
	Weapon,
	Armor
};

// 강화석 등급
UENUM(BlueprintType)
enum class ET3UpgradeStoneGrade : uint8
{
	Normal,     // 하급 (1~3강)
	Epic,       // 중급 (1~5강)
	Legendary   // 상급 (1~7강)
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

	// 소켓된 룬 ID 목록 (저장/로드용)
	UPROPERTY()
	TArray<FName> SocketedRuneIDs;
};

