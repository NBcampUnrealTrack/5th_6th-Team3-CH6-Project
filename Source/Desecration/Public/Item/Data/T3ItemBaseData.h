#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "T3ItemBaseData.generated.h"

UENUM(BlueprintType)
enum class EItemType : uint8
{
	None,
	Rune,
	Consumable,
	Etc,
	Acc
};

USTRUCT(BlueprintType)
struct DESECRATION_API FT3ItemBaseData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item Data")
	FText Name = FText::GetEmpty(); // 플레이어에게 보여질 아이템 이름
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item Data")
	UTexture2D* Icon = nullptr; // 아이콘
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item Data")
	EItemType ItemType = EItemType::None;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item Data")
	FText Description = FText::GetEmpty();
};