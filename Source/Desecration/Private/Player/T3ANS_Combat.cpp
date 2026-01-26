// T3ANS_Combat.cpp


#include "Player/T3ANS_Combat.h"
#include "Player/T3CharacterBase.h"
#include "Player/T3CombatComponent.h"
#include "Player/T3DamageTypes.h"


UT3ANS_Combat::UT3ANS_Combat()
{
    DamageTypeClass = UT3DamageType_Base::StaticClass();
}

void UT3ANS_Combat::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
    UAnimNotifyState::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

    if (!MeshComp || !MeshComp->GetOwner()) return;

    if (AT3CharacterBase* Char = Cast<AT3CharacterBase>(MeshComp->GetOwner()))
    {
        if (UT3CombatComponent* Combat = Char->GetCombatComponent())
        {
            switch (StatusType)
            {
            case ECombatWindowType::Parry:
                Combat->SetParryingEnabled(true);
                break;
            case ECombatWindowType::Invincible:
                // Combat->SetInvincible(true); // 추후 구현 시
                break;
            case ECombatWindowType::Attack:
                Combat->SetAttackDetectionEnabled(true, AttackDamageMultiflier, DamageTypeClass);
                break;
            }
        }
    }
}

void UT3ANS_Combat::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
    UAnimNotifyState::NotifyEnd(MeshComp, Animation, EventReference);

    if (!MeshComp || !MeshComp->GetOwner()) return;

    if (AT3CharacterBase* Char = Cast<AT3CharacterBase>(MeshComp->GetOwner()))
    {
        if (UT3CombatComponent* Combat = Char->GetCombatComponent())
        {
            switch (StatusType)
            {
            case ECombatWindowType::Parry:
                Combat->SetParryingEnabled(false);
                break;
            case ECombatWindowType::Invincible:
                // Combat->SetInvincible(false);
                break;
            case ECombatWindowType::Attack:
                Combat->SetAttackDetectionEnabled(false);
                break;
            }
        }
    }
}