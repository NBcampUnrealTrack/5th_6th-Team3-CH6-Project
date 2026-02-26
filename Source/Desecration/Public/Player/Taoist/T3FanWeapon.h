// T3FanWeapon.h

#pragma once

#include "CoreMinimal.h"
#include "Player/T3WeaponBase.h"
#include "T3FanWeapon.generated.h"

UENUM(BlueprintType)
enum class EFanState : uint8
{
    Opened,
    Closed,
    Opening,
    Closing
};


UCLASS()
class DESECRATION_API AT3FanWeapon : public AT3WeaponBase
{
	GENERATED_BODY()
	
public:
    // 기본은 펴진 상태로 설정
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "State")
    EFanState CurrentState = EFanState::Opened;

    // 애니메이션 에셋들
    UPROPERTY(EditAnywhere, Category = "Animation")
    class UAnimSequence* OpenAnim;

    UPROPERTY(EditAnywhere, Category = "Animation")
    class UAnimSequence* CloseAnim;

    UFUNCTION(BlueprintCallable)
    void ToggleFan();
};
