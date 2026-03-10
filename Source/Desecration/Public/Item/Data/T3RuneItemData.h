#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "T3ItemBaseData.h"
#include "Item/Rune/T3RuneBase.h"
#include "T3RuneItemData.generated.h"

USTRUCT(BlueprintType)
struct FT3RuneItemData : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item Data")
	FT3ItemBaseData ItemData;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rune")
	TSubclassOf<UT3RuneBase> RuneLogicClass;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rune")
	ET3EquipmentType EquipmentType = ET3EquipmentType::Weapon;
};