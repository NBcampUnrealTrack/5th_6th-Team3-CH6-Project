// T3LevelUpData.h

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "T3LevelUpData.generated.h"

USTRUCT(BlueprintType)
struct FT3LevelUpData : public FTableRowBase
{
    GENERATED_BODY()

    // 해당 레벨로 올라갈 때 필요한 룬
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 RequiredRune = 0;
};