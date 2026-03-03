// T3Valkyrie_SkillComponent.h

#pragma once

#include "CoreMinimal.h"
#include "Player/T3SkillComponentBase.h"
#include "T3Valkyrie_SkillComponent.generated.h"


UCLASS()
class DESECRATION_API UT3Valkyrie_SkillComponent : public UT3SkillComponentBase
{
	GENERATED_BODY()

public:	
    virtual void ExecuteSkill(int32 SkilSolt) override;
    virtual void ExecuteSkillNotify(int32 Index) override;
    virtual void CancelCurrentSkill() override;

    virtual FSkillData* GetSkillDataByID(int32 SkillID) override;
	
#pragma region PowerStrike
public:
	UPROPERTY(EditAnywhere, Category = "PowerStrike")
	bool bIsCharging = false;
	UPROPERTY(EditAnywhere, Category = "PowerStrike")
	int32 ChargingLevel = 0;
	UPROPERTY(EditAnywhere, Category = "PowerStrike")
	int32 MaxChargingLevel = 3;
	UPROPERTY(EditAnywhere, Category = "PowerStrike")
	FTimerHandle ChargingTimerHandle;
	
	UFUNCTION(BlueprintImplementableEvent)
	void PowerStrike();
	UFUNCTION(BlueprintCallable)
	void StartCharge();
	UFUNCTION(BlueprintCallable)
	void ChargingTick();
	UFUNCTION(BlueprintCallable)
	void EndCharging();
#pragma endregion
	
    UPROPERTY(EditAnywhere, Category = "Skill Data")
    FSkillData EmptySkillData;

    // 아래에 추가될 스킬의 데이터 추가
    // 
    //UPROPERTY(EditAnywhere, Category = "Skill Data")
    //FSkillData SwordWaveData;


	
};
