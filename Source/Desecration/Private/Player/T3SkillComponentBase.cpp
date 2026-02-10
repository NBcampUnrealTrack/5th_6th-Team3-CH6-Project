// T3SkillComponentBase.cpp


#include "Player/T3SkillComponentBase.h"
#include "Player/T3CharacterBase.h"
#include "Player/T3CombatComponent.h"

void UT3SkillComponentBase::ExecuteSkillNotify(int32 Index)
{
}
void UT3SkillComponentBase::ExecuteSkill(int32 SkillSlot)
{
}

void UT3SkillComponentBase::SetSkillSlot(int32 SlotNumber, int32 NewSkillID)
{
    // 1. 중복 장착 방지 로직
    if (Slot_1_SkillID == NewSkillID || Slot_2_SkillID == NewSkillID)
    {
        // 이미 장착된 스킬이라면 기존 슬롯과 스왑하거나 무시
        UE_LOG(LogTemp, Warning, TEXT("Skill ID %d is already equipped!"), NewSkillID);
    }

    // 2. 슬롯 번호에 따른 할당
    if (SlotNumber == 1)
    {
        Slot_1_SkillID = NewSkillID;
    }
    else if (SlotNumber == 2)
    {
        Slot_2_SkillID = NewSkillID;
    }

    UE_LOG(LogTemp, Log, TEXT("Slot %d updated with Skill ID: %d"), SlotNumber, NewSkillID);

}

void UT3SkillComponentBase::BeginPlay()
{
    Super::BeginPlay();

    // 캐싱
    OwnerChar = Cast<AT3CharacterBase>(GetOwner());

    if (OwnerChar)
    {

        Combat = OwnerChar->GetCombatComponent();

        UE_LOG(LogTemp, Log, TEXT("[SkillBase] Caching Success: %s"), *OwnerChar->GetName());
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("[SkillBase] Owner is not AT3CharacterBase!"));
    }
}

bool UT3SkillComponentBase::CanExecuteSkill(FSkillData& Data)
{
    // 1. 마나 체크
    if (OwnerChar->GetCurrentMana() < Data.ManaCost)
    {
        UE_LOG(LogTemp, Warning, TEXT("Not Enough Mana! Your Mana : %.1f, ManaCost : %.1f"), OwnerChar->GetCurrentMana(), Data.ManaCost);
        return false;
    }

    // 2. 쿨타임 체크
    float CurrentTime = GetWorld()->GetTimeSeconds();
    if (CurrentTime - Data.LastActivatedTime < Data.Cooldown)
    {
        float RemainingTime = Data.Cooldown - (CurrentTime - Data.LastActivatedTime);
        UE_LOG(LogTemp, Warning, TEXT("Skill is on Cooldown! Please wait %1.f second"), RemainingTime);
        return false;
    }

    return true;
}

void UT3SkillComponentBase::StartCooldown(FSkillData& Data)
{
    // 스킬을 사용한 시간 캐싱, 마나 소모
    Data.LastActivatedTime = GetWorld()->GetTimeSeconds();
    OwnerChar->ConsumeMana(Data.ManaCost);
}


