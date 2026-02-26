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


    UPROPERTY(EditAnywhere, Category = "Skill Data")
    FSkillData EmptySkillData;

    // 아래에 추가될 스킬의 데이터 추가
    // 
    //UPROPERTY(EditAnywhere, Category = "Skill Data")
    //FSkillData SwordWaveData;


	
};
