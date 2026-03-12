// T3ANS_Combat.cpp


#include "Player/T3ANS_Combat.h"
#include "Player/T3CharacterBase.h"
#include "Player/T3WeaponBase.h"


UT3ANS_Combat::UT3ANS_Combat()
{
    DamageTypeClass = UT3DamageType_Base::StaticClass();
}

void UT3ANS_Combat::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
    UAnimNotifyState::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

    if (!MeshComp || !MeshComp->GetOwner()) return;

    AT3CharacterBase* Char = Cast<AT3CharacterBase>(MeshComp->GetOwner());
    if (!Char) return;

    UT3CombatComponent* Combat = Char->GetCombatComponent();
    if (!Combat) return;

    // 장비를 장착한 위치 반환
    AT3WeaponBase* TargetWeapon = Combat->GetWeaponBySlot(TargetSlot);

            switch (StatusType)
            {
            case ECombatWindowType::Attack:
                if (TargetWeapon)
                {
                 TargetWeapon->SetWeaponCollisionEnabled(true, AttackDamageMultiflier, DamageTypeClass, AttackIntensity, StunAmount, StaminaAmount);
                 Combat->ConsumeStamina(StaminaAmount);
                 const FString SlotName = StaticEnum<EEquipSlot>()->GetNameStringByValue((int64)TargetSlot);
                 UE_LOG(LogTemp, Log, TEXT("TargetSlot: %s"), *SlotName);
                }
                break;
            case ECombatWindowType::Parry:
                Combat->SetParryingEnabled(true);
                break;
            case ECombatWindowType::Dodge:
                Combat->SetDodgingEnabled(true);
                break;
            case ECombatWindowType::PrevenRegen:
                Char->bCanRegenStamina = false;
                break;
            case ECombatWindowType::PowerStrike:
                Combat->SetPowerStrike(true);
                break;
                
            }
}

void UT3ANS_Combat::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
    UAnimNotifyState::NotifyEnd(MeshComp, Animation, EventReference);

    if (!MeshComp || !MeshComp->GetOwner()) return;

    AT3CharacterBase* Char = Cast<AT3CharacterBase>(MeshComp->GetOwner());
    if (!Char) return;

    UT3CombatComponent* Combat = Char->GetCombatComponent();
    if (!Combat) return;

    // 장비를 장착한 위치 반환
    AT3WeaponBase* TargetWeapon = Combat->GetWeaponBySlot(TargetSlot);

            switch (StatusType)
            {
            case ECombatWindowType::Parry:
                Combat->SetParryingEnabled(false);
                break;
            case ECombatWindowType::Dodge:
                Combat->SetDodgingEnabled(false);
                break;
            case ECombatWindowType::Attack:
                TargetWeapon->SetWeaponCollisionEnabled(false);
                break;
            case ECombatWindowType::PrevenRegen:
                Char->bCanRegenStamina = true;
                break;
            case ECombatWindowType::PowerStrike:
                Combat->SetPowerStrike(false);
                break;
            }
}