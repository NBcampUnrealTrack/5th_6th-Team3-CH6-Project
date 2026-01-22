#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "T3ItemBaseData.h"
#include "T3ComsumableItemData.generated.h"

UENUM(BlueprintType)
enum class EEffectType : uint8
{
	None,
	HP,
	MP,
	Power,
	Defense,
	Speed,
	Berserk
};

USTRUCT(BlueprintType)
struct FT3ComsumableItemData : public FTableRowBase
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item Data")
	FT3ItemBaseData ItemData;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "EffectType")
	EEffectType EffectType;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Comsumable")
	float BuffValue; // 초기 버프 수치
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Comsumable")
	float DebuffValue; // 디버프 수치
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Comsumable")
	float ActiveTime; // 지속시간
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Comsumable")
	float CoolTime; // 쿨타임
};