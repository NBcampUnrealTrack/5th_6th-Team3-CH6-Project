#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "T3ItemBaseData.h"
#include "T3EtcItemData.generated.h"

USTRUCT(BlueprintType)
struct FT3EtcItemData : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item Data")
	FT3ItemBaseData ItemData;
};
