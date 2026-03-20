#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "T3ShopData.generated.h"

USTRUCT(BlueprintType)
struct FT3ShopData : public FTableRowBase
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Buy Data")
	FText Name = FText::GetEmpty(); // 플레이어에게 보여질 아이템 이름
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item Data")
	UTexture2D* Icon = nullptr;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Buy Data")
	int32 BuyPrice = 0; // 플레이어가 살 때 가격
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Buy Data")
	uint8 bCanBuy : 1 = false; // 플레이어가 살 수 있는지
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sell Data")
	int32 SellPrice = 0; // 플레이어가 팔 때 가격
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sell Data")
	uint8 bCanSell : 1 = false; // 플레이어가 팔 수 있는지
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item Data", meta = (MultiLine = true))
	FText ItemInfo = FText::GetEmpty();
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item Data")
	ECharacterClass CharacterClass = ECharacterClass::None;
};