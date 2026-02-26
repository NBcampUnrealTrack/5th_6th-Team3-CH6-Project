// T3Taoist_SkillComponent.h

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameFramework/Character.h"
#include "Player/T3SkillComponentBase.h"
#include "T3Taoist_SkillComponent.generated.h"

class AT3TalismanProjectile;
class UAnimMontage;

UCLASS()
class DESECRATION_API UT3Taoist_SkillComponent : public UT3SkillComponentBase
{
	GENERATED_BODY()
	
public:
    UT3Taoist_SkillComponent();

    virtual void ExecuteSkill(int32 SkilSolt) override;
    virtual void ExecuteSkillNotify(int32 Index) override;
    virtual void CancelCurrentSkill() override;

    virtual FSkillData* GetSkillDataByID(int32 SkillID) override;


    UPROPERTY(EditAnywhere, Category = "Skill Data")
    FSkillData EmptySkillData;

protected:

    void OnSkillMontageEnded(UAnimMontage* Montage, bool bInterrupted);


    // ===== 기본 공격 (부적 날리기)
  
    // 에디터에서 할당할 부적 블루프린트 클래스
    UPROPERTY(EditAnywhere, Category = "Combat|Skill")
    TSubclassOf<class AT3TalismanProjectile> TalismanClass;

    // 부적 스폰 로직
    void SpawnTalisman();
};
