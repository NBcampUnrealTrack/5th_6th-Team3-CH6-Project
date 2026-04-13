// T3Valkyrie_SkillComponent.cpp


#include "Player/Valkyrie/T3Valkyrie_SkillComponent.h"

#include "Desecration.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Player/Valkyrie/T3LunarSlash.h"
#include "Player/T3CharacterBase.h"
#include "Player/T3CombatComponent.h"

FSkillData* UT3Valkyrie_SkillComponent::GetSkillDataByID(int32 SkillID)
{
    switch (SkillID)
    {
    case 1: return &PowerStrikeSkillData;
    case 2: return &EnduranceSkillData;
    case 3: return &LunarSwordSkillData;
    case 4: return &LunarSlashSkillData;
    default: return nullptr;
    }
}

void UT3Valkyrie_SkillComponent::StartCharge()
{
    if (bIsCharging) return;
    
    bIsCharging = true;
    
    if (OwnerChar && OwnerChar->GetCharacterMovement())
    {
        OwnerChar->GetCharacterMovement()->bAllowPhysicsRotationDuringAnimRootMotion = true;
        if (!OwnerChar->PlayerInputState.bIsLockOn)
        {
            OwnerChar->bUseControllerRotationYaw=true;
            OwnerChar->GetCharacterMovement()->bOrientRotationToMovement = false;
        }
    }
    
    bHasRelease = false;
    ChargingLevel = 0;
    
    OnGainCharge();
    GetWorld()->GetTimerManager().SetTimer(ChargingTimerHandle, this, &UT3Valkyrie_SkillComponent::ChargingTick, 1.0f, true);
}

void UT3Valkyrie_SkillComponent::ChargingTick()
{
    ChargingLevel++;
    if (ChargingLevel <2)
    {
        OnGainCharge();
    }
    
    if (ChargingLevel>=MaxChargingLevel)
    {
        ChargingLevel = MaxChargingLevel;
        GetWorld()->GetTimerManager().ClearTimer(ChargingTimerHandle);
        
        GetWorld()->GetTimerManager().SetTimer(MaxChargingTimerHandle, this,&UT3Valkyrie_SkillComponent::MaxCharging, 1.0f, true);;
    }
}

void UT3Valkyrie_SkillComponent::EndCharging()
{
    if (!bIsCharging || bHasRelease) return;
    GetWorld()->GetTimerManager().ClearTimer(ChargingTimerHandle);
    GetWorld()->GetTimerManager().ClearTimer(MaxChargingTimerHandle);
    bHasRelease = true;
    
    if (OwnerChar && OwnerChar->GetCharacterMovement())
    {
        OwnerChar->GetCharacterMovement()->bAllowPhysicsRotationDuringAnimRootMotion = false;
        if (!OwnerChar->PlayerInputState.bIsLockOn)
        {
            OwnerChar->bUseControllerRotationYaw = false;
            OwnerChar->GetCharacterMovement()->bOrientRotationToMovement = true;
        }
    }
    OnEndCharging();
}

void UT3Valkyrie_SkillComponent::MaxCharging()
{
    if (!bIsCharging || bHasRelease) return;
    GetWorld()->GetTimerManager().ClearTimer(ChargingTimerHandle);
    GetWorld()->GetTimerManager().ClearTimer(MaxChargingTimerHandle);
    
    bHasRelease = true;
    if (OwnerChar && OwnerChar->GetCharacterMovement())
    {
        OwnerChar->GetCharacterMovement()->bAllowPhysicsRotationDuringAnimRootMotion = false;
        if (!OwnerChar->PlayerInputState.bIsLockOn)
        {
            OwnerChar->bUseControllerRotationYaw = false;
            OwnerChar->GetCharacterMovement()->bOrientRotationToMovement = true;
        }
    }
    OnEndCharging();
}

void UT3Valkyrie_SkillComponent::BasicAttackCount()
{
    CurrentBasicAttackCount++;
    
    if (CurrentBasicAttackCount >= TriggerAttackCount)
    {
        if (OwnerChar)
        {
            float HealAmount = (OwnerChar->GetMaxHP() * 0.1f) + (OwnerChar->GetMaxHP() * PassiveHealBonus / 100);
            float NewHP = FMath::Clamp(OwnerChar->GetCurrentHP() + HealAmount, 0.0f, OwnerChar->GetMaxHP());
            OwnerChar->SetCurrentHP(NewHP);
            CurrentBasicAttackCount = 0;
            
            UE_LOG(LogItem, Log, TEXT("발키리 패시브 발동 : %.1f 회복됨"), HealAmount);
        }
    }
}

void UT3Valkyrie_SkillComponent::EnduranceBegin()
{
    OwnerChar->bIsSuperArmor = true;
    GetWorld()->GetTimerManager().SetTimer(EnduranceTimerHandle, this, &UT3Valkyrie_SkillComponent::EnduranceEnd, 20.0f, true);
}

void UT3Valkyrie_SkillComponent::EnduranceEnd()
{
    GetWorld()->GetTimerManager().ClearTimer(EnduranceTimerHandle);
    OwnerChar->bIsSuperArmor = false;
}

void UT3Valkyrie_SkillComponent::ExecuteSkill(int32 SlotNumber)
{
    // 1. 슬롯 번호(1 or 2)에 따른 ID 추출
    int32 SkillID = (SlotNumber == 1) ? CurrentSkillSlot : NextSkillSlot;

    // 2. ID에 맞는 데이터 가져오기
    FSkillData* TargetData = GetSkillDataByID(SkillID);

    // 3. 마나 & 쿨타임 체크
    if (!TargetData || SkillID == 0) return;
    if (!CanExecuteSkill(*TargetData)) return;

    // 4. 쿨타임 시작 및 스킬 실행
    StartCooldown(SkillID, *TargetData);

    // ID에 따른 분기
    switch (SkillID)
    {
    case 0: // 빈슬롯
        UE_LOG(LogTemp, Warning, TEXT("There is no skill."));    break;
    case 1:
        PowerStrike();
        break;// 첫번째 스킬
    case 2:
        Endurance();
        break;
    case 3:
        LunarSword();
        break;
    case 4:
        OnLunarSlash();
        break;
        //ExecuteSwordWave();    break;

    default:
        UE_LOG(LogTemp, Warning, TEXT("Unknown Skill ID: %d"), SkillID);   break;
    }
}

void UT3Valkyrie_SkillComponent::ExecuteSkill_Completed(int32 SlotNumber)
{    
    int32 SkillID = (SlotNumber == 1) ? CurrentSkillSlot : NextSkillSlot;
    
    switch (SkillID)
    {
    case 1:
        EndCharging();
        break;
        
    default:
        break;
    }
}


void UT3Valkyrie_SkillComponent::ExecuteSkillNotify(int32 Index)
{
    // 공통 노티파이에서 보낸 Index에 따라 분기
    switch (Index)
    {
    case 1: // 

        
        break;
    }
}

void UT3Valkyrie_SkillComponent::CancelCurrentSkill()
{
}

void UT3Valkyrie_SkillComponent::SetPassiveHealBonus(float NewHealBonus)
{
    PassiveHealBonus = NewHealBonus;
}

void UT3Valkyrie_SkillComponent::SetTriggerAttackCount(int32 NewCount)
{
    TriggerAttackCount = NewCount;
}
