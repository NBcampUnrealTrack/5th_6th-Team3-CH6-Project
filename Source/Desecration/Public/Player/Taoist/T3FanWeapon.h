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

// 부채 상태 변경 시 호출될 델리게이트 (애니메이션 연동용)
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnFanStateChanged, EFanState, NewState);

UCLASS()
class DESECRATION_API AT3FanWeapon : public AT3WeaponBase
{
	GENERATED_BODY()
	
public:

    AT3FanWeapon();

    // 상태 변경 함수 (내부 로직에서 호출)
    void SetFanState(EFanState NewState);

    // 공격/스킬 시작 시 호출 
    UFUNCTION(BlueprintCallable, Category = "Weapon|Action")
    void OpenFan();

    // 공격/스킬 종료 시 호출 
    UFUNCTION(BlueprintCallable, Category = "Weapon|Action")
    void CloseFan();

protected:
    // 실제 상태 값은 캡슐화하고 BlueprintReadOnly로 노출
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State")
    EFanState CurrentState = EFanState::Closed; // 기본은 접힌 상태 권장

    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnFanStateChanged OnFanStateChanged;

    // 애니메이션 몽타주 
    UPROPERTY(EditAnywhere, Category = "Animation")
    TObjectPtr<class UAnimMontage> FanOpenMontage;

    UPROPERTY(EditAnywhere, Category = "Animation")
    TObjectPtr<class UAnimMontage> FanCloseMontage;
};
