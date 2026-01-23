// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "EquipmentTypes.generated.h"

/**
 * 
 */

class URuneLogicBase; // 전방 선언

// 장비 종류를 구분하는 딱지(Tag)
UENUM(BlueprintType)
enum class EEquipmentType : uint8
{
	Weapon,
	Armor
};

UENUM(BlueprintType)
enum class ERuneStatType : uint8
{
	None,       // 특수 룬용 (스탯 관여 안 함)
	Attack,
	Defense,
	// 필요 시 Health, Speed 등 추가
};


// [신규] 레벨 하나하나의 정보를 담는 작은 구조체
USTRUCT(BlueprintType)
struct FWeaponLevelData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float FixedAttackPower = 0.0f; 

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSoftClassPtr<AActor> VisualEffectClass; // 무기는 이펙트 액터를 씀
};

USTRUCT(BlueprintType)
struct FWeaponGrowthRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FWeaponLevelData> LevelStats; 
};

USTRUCT(BlueprintType)
struct FArmorLevelData
{
	GENERATED_BODY()

	// [변경] 여기도 고정 방어력 수치입니다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float FixedDefensePower = 0.0f;

	// 방어구는 이펙트 액터 대신 재질(Material)이 빛나는 경우가 많음
	// 예시: UPROPERTY(EditAnywhere) TSoftObjectPtr<UMaterialInterface> GlowMaterial;
};

USTRUCT(BlueprintType)
struct FArmorGrowthRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FArmorLevelData> LevelStats; 
};


USTRUCT(BlueprintType)
struct FWeaponBaseRow : public FTableRowBase
{
	GENERATED_BODY()
    
	UPROPERTY(EditAnywhere)
	float BaseAttackPower; // 공격력

	UPROPERTY(EditAnywhere)
	TSoftObjectPtr<UStaticMesh> Mesh; 
};

// 방어구용 (구조는 같지만 이름이 다름)
USTRUCT(BlueprintType)
struct FArmorBaseRow : public FTableRowBase
{
	GENERATED_BODY()
    
	UPROPERTY(EditAnywhere)
	float BaseDefensePower; // 방어력 (이름 변경!)

	UPROPERTY(EditAnywhere)
	TSoftObjectPtr<USkeletalMesh> Mesh; // 방어구는 보통 스켈레탈 메쉬를 씀
};

USTRUCT(BlueprintType)
struct FItemSaveData
{
	GENERATED_BODY()

	UPROPERTY()
	FName ItemID;

	UPROPERTY()
	int32 Level;

	UPROPERTY()
	EEquipmentType Type;
};

// 2. 룬 데이터 테이블 구조체 (DT_Runes)
USTRUCT(BlueprintType)
struct FRuneDataRow : public FTableRowBase
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FText RuneName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UTexture2D* Icon;

	// [데이터] 어떤 스탯을 올려주는가? (Inject 대상)
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	ERuneStatType StatType = ERuneStatType::None;

	// [데이터] 얼마나 올려주는가? (Inject 대상)
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float StatValue = 0.0f;

	// [로직] 행동을 정의할 클래스 (BP)
	// 일반 스탯 룬 -> BP_StandardRune (기본 로직)
	// 특수 룬 -> BP_GodRune (커스텀 로직)
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSubclassOf<URuneLogicBase> RuneLogicClass;
};
UCLASS()
class DESECRATION_API UEquipmentTypes : public UObject
{
	GENERATED_BODY()
};
