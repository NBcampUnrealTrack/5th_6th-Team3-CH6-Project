#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "T3ItemBaseData.generated.h"

UENUM(BlueprintType)
enum class EItemType : uint8
{
	None,
	Rune,
	Comsumable
};

USTRUCT(BlueprintType)
struct DESECRATION_API FT3ItemBaseData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item Data")
	FText Name; // 플레이어에게 보여질 아이템 이름
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item Data")
	int32 Price; // 아이템 가격
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item Data")
	UTexture2D* Icon; // 아이콘
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item Data")
	EItemType ItemType = EItemType::None;
};