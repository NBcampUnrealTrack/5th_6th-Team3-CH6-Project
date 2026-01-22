// T3ANS_Combat.h

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "T3ANS_Combat.generated.h"

enum class ECombatWindowType : uint8;

UCLASS()
class DESECRATION_API UT3ANS_Combat : public UAnimNotifyState
{
    GENERATED_BODY()

public:
    // 에디터 디테일 창에서 원하는 윈도우 타입 선택
    UPROPERTY(EditAnywhere, Category = "Combat")
    ECombatWindowType StatusType;

    virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference) override;
    virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;
};