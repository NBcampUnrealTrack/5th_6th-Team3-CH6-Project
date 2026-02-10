// T3SkillComponentBase.h

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "T3SkillComponentBase.generated.h"


UCLASS( Blueprintable, ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class DESECRATION_API UT3SkillComponentBase : public UActorComponent
{
    GENERATED_BODY()

public:
    // 슬롯 1, 2에 장착된 스킬 번호
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill")
    int32 Slot_1_SkillID = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill")
    int32 Slot_2_SkillID = 1;

    // 슬롯 번호를 입력받아 스킬 실행
    virtual void ExecuteSkillNotify(int32 Index);
    virtual void ExecuteSkill(int32 SkillSlot);


protected:
    // 쿨타임 체크용 맵 (SkillID, LastExecutionTime)
    TMap<int32, float> SkillCooldownMap;
};
