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
	virtual void ExecuteSkill_Completed(int32 SkillSlot) override;
    virtual void ExecuteSkillNotify(int32 Index) override;
    virtual void CancelCurrentSkill() override;

    virtual FSkillData* GetSkillDataByID(int32 SkillID) override;
	
#pragma region PowerStrike
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PowerStrike")
	bool bIsCharging = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PowerStrike")
	bool bHasRelease = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PowerStrike")
	int32 ChargingLevel = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PowerStrike")
	int32 MaxChargingLevel = 2;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PowerStrike")
	FTimerHandle ChargingTimerHandle;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PowerStrike")
	FTimerHandle MaxChargingTimerHandle;
	
	UFUNCTION(BlueprintImplementableEvent)
	void PowerStrike();
	UFUNCTION(BlueprintImplementableEvent)
	void OnEndCharging();
	UFUNCTION(BlueprintCallable)
	void StartCharge();
	UFUNCTION(BlueprintCallable)
	void ChargingTick();
	UFUNCTION(BlueprintCallable)
	void EndCharging();
	UFUNCTION(BlueprintCallable)
	void MaxCharging();
#pragma endregion
	
#pragma region Passive
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Passive")
	int32 CurrentBasicAttackCount = 0;
	
	virtual void BasicAttackCount() override;
#pragma endregion
	
#pragma region LunarSlash
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "LunarSlash")
	TSubclassOf<class AT3LunarSlash> AT3LunarSlash;
#pragma endregion
	
    UPROPERTY(EditAnywhere, Category = "Skill Data")
    FSkillData EmptySkillData;
	UPROPERTY(EditAnywhere, Category = "Skill Data")
	FSkillData PowerStrikeSkillData;

    // 아래에 추가될 스킬의 데이터 추가
    // 
    //UPROPERTY(EditAnywhere, Category = "Skill Data")
    //FSkillData SwordWaveData;


	
};
