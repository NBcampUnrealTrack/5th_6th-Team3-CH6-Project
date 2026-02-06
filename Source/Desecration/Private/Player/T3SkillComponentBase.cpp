// T3SkillComponentBase.cpp


#include "Player/T3SkillComponentBase.h"

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

    // 3. UI 업데이트 알림 (나중에 UI팀이 이 델리게이트를 써서 아이콘을 바꿈)
    // OnSkillChanged.Broadcast(SlotNumber, NewSkillID);
}

FSkillAttributes* UT3SkillComponentBase::GetSkillRow(int32 SkillID)
{
    if (!MySkillTable) return nullptr;

    // ID를 문자열 이름으로 변환
    FString RowName = FString::FromInt(SkillID);

    return MySkillTable->FindRow<FSkillAttributes>(FName(*RowName), TEXT("SkillContext"));
}


