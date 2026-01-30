// T3AN_SkillEvent.h

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "T3AN_SkillEvent.generated.h"

/**
 * 
 */
UCLASS()
class DESECRATION_API UT3AN_SkillEvent : public UAnimNotify
{
	GENERATED_BODY()
	
public:
    // 에디터에서 이 노티파이가 몇 번째 스킬인지 지정
    UPROPERTY(EditAnywhere, Category = "Skill")
    int32 SkillIndex = 0;

    virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;
};
