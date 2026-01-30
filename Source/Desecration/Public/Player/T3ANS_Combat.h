// T3ANS_Combat.h

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "Player/T3DamageTypes.h"
#include "T3ANS_Combat.generated.h"

enum class ECombatWindowType : uint8;

UCLASS()
class DESECRATION_API UT3ANS_Combat : public UAnimNotifyState
{
    GENERATED_BODY()

public:
   
    UT3ANS_Combat();
  
    // 에디터 디테일 창에서 원하는 윈도우 타입 선택
    UPROPERTY(EditAnywhere, Category = "Combat")
    ECombatWindowType StatusType;

    // 이 공격의 기본 데미지 배율
    UPROPERTY(EditAnywhere, Category = "Combat", meta = (EditCondition = "StatusType == ECombatWindowType::Attack"))
    float AttackDamageMultiflier = 1.f;

    UPROPERTY(EditAnywhere, Category = "Combat", meta = (EditCondition = "StatusType == ECombatWindowType::Attack"))
    EHitIntensity AttackIntensity;

    // 데미지 타입 
    UPROPERTY(EditAnywhere, Category = "Combat", meta = (EditCondition = "StatusType == ECombatWindowType::Attack"))
    TSubclassOf<class UT3DamageType_Base> DamageTypeClass;

    // 스턴값
    UPROPERTY(EditAnywhere, Category = "Combat", meta = (EditCondition = "StatusType == ECombatWindowType::Attack"))
    float StunAmount;

    virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference) override;
    virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;
};