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


void UT3SkillComponentBase::SetSkillSlot(int32 NewSkillID, bool bIsEquip)
{
    if (bIsEquip)
    {
        // [장착 로직] 중복 검사 후 빈 곳에 우선 할당
        if (CurrentSkillSlot == NewSkillID || NextSkillSlot == NewSkillID) return;

        if (CurrentSkillSlot == 0) { CurrentSkillSlot = NewSkillID; }
        else if (NextSkillSlot == 0) { NextSkillSlot = NewSkillID; }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("모든 스킬 슬롯이 가득 찼습니다."));
            return;
        }
    }
    else
    {
        // [탈착 로직]
        if (CurrentSkillSlot == NewSkillID)
        {
            // 메인 슬롯을 비울 때, 서브 슬롯에 스킬이 있다면 메인으로 당겨옴
            CurrentSkillSlot = NextSkillSlot;
            NextSkillSlot = 0;
        }
        else if (NextSkillSlot == NewSkillID)
        {
            NextSkillSlot = 0;
        }
        else { return; }
    }

    //UI팀 : 1번은 큰 슬롯(Current), 2번은 작은 슬롯(Next)
    if (OnSkillSlotUpdated.IsBound())
    {
        // 현재 슬롯(1번) 정보 갱신
        OnSkillSlotUpdated.Broadcast(1, CurrentSkillSlot, *GetSkillDataByID(CurrentSkillSlot));
        // 다음 슬롯(2번) 정보 갱신
        OnSkillSlotUpdated.Broadcast(2, NextSkillSlot, *GetSkillDataByID(NextSkillSlot));
    }

    UE_LOG(LogTemp, Log, TEXT("슬롯 상태 - CurrentSlot: %d, NextSlot: %d"), CurrentSkillSlot, NextSkillSlot);
}

int32 UT3SkillComponentBase::GetSkillIDBySlotIndex(int32 Index) const
{
    if (Index == 1) return CurrentSkillSlot;
    if (Index == 2) return NextSkillSlot;
    return 0;
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

void UT3SkillComponentBase::SwapSkills()
{
    // 넥스트 슬롯이 비어있으면 스왑 안 함
    if (NextSkillSlot == 0) return;

    int32 TempID = CurrentSkillSlot;
    CurrentSkillSlot = NextSkillSlot;
    NextSkillSlot = TempID;

    UE_LOG(LogTemp, Log, TEXT("Skills Swapped! Current: %d, Next: %d"), CurrentSkillSlot, NextSkillSlot);

    // UI팀에게 알림: 전체 슬롯 정보 브로드캐스트
    if (OnSkillSlotUpdated.IsBound())
    {
        OnSkillSlotUpdated.Broadcast(1, CurrentSkillSlot, *GetSkillDataByID(CurrentSkillSlot));
        OnSkillSlotUpdated.Broadcast(2, NextSkillSlot, *GetSkillDataByID(NextSkillSlot));
    }
}


