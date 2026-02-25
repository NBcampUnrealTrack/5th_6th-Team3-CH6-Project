// T3TaoistData.h

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "T3TaoistData.generated.h"


USTRUCT(BlueprintType)
struct FTalismanData : public FTableRowBase
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
    float InitialSpeed = 1500.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
    float GravityScale = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wave")
    float Amplitude = 50.f;  // 흔들림 폭

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wave")
    float Frequency = 10.f;  // 흔들림 속도

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visual")
    FLinearColor TalismanColor = FLinearColor::White;
};
