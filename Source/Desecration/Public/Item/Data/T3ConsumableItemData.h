#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "T3ItemBaseData.h"
#include "T3ConsumableItemData.generated.h"

class UNiagaraSystem;

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
struct FT3ConsumableItemData : public FTableRowBase
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item Data")
	FT3ItemBaseData ItemData;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "EffectType")
	EEffectType EffectType = EEffectType::None;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Comsumable")
	float BuffValue = 0.f; // 초기 버프 수치
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Comsumable")
	float DebuffValue = 0.f; // 디버프 수치
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Comsumable")
	float ActiveTime = 0.f; // 지속시간
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Comsumable")
	float CoolTime = 0.f; // 쿨타임
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "VFX")
	UNiagaraSystem* UseEffect = nullptr; // 재생할 이펙트
};