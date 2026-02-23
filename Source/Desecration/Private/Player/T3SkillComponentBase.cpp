// T3SkillComponentBase.cpp


#include "Player/T3SkillComponentBase.h"
#include "Player/T3CharacterBase.h"
#include "Player/T3CombatComponent.h"

void UT3SkillComponentBase::CancelCurrentSkill()
{
}

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
        FSkillData* NextData = GetSkillDataByID(NextSkillSlot);
        // 현재 슬롯 갱신
        FSkillData* CurrentData = GetSkillDataByID(CurrentSkillSlot);
        if (CurrentData)
        {
            OnSkillSlotUpdated.Broadcast(1, CurrentSkillSlot, *CurrentData);
        }
        else
        {
            // 데이터가 없을 때 전송할 빈 구조체 정의 혹은 처리 로직
            OnSkillSlotUpdated.Broadcast(1, CurrentSkillSlot, FSkillData());
        }

        // 다음 슬롯 갱신
        if (NextData)
        {
            OnSkillSlotUpdated.Broadcast(2, NextSkillSlot, *NextData);
        }
        else
        {
            // 탈착 시 데이터가 nullptr이면 빈 구조체를 넘겨 UI에서 '비어있음'을 표현하게 함
            OnSkillSlotUpdated.Broadcast(2, NextSkillSlot, FSkillData());
        }
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
   
    // 0. 안전 장치 추가
    if (!OwnerChar)
    {
        // 다시 한 번 캐싱 시도 (만약을 대비)
        OwnerChar = Cast<AT3CharacterBase>(GetOwner());
        if (!OwnerChar)
        {
            UE_LOG(LogTemp, Error, TEXT("CanExecuteSkill Failed: OwnerChar is NULL!"));
            return false;
        }
    }

    // 1. 입력 상태 체크 (OwnerChar가 확실히 있을 때만 접근)
    if (bUsingSkill || !OwnerChar->PlayerInputState.bCanAttack) return false;


    if (bUsingSkill || !OwnerChar->PlayerInputState.bCanAttack) return false;
    
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

void UT3SkillComponentBase::StartCooldown(int32 SkillID, FSkillData& Data)
{
    // 1. 데이터 업데이트
    Data.LastActivatedTime = GetWorld()->GetTimeSeconds();

    if (OwnerChar)
    {
        OwnerChar->ConsumeMana(Data.ManaCost);
    }

    // 2. UI 알림
    if (OnSkillCooldownStarted.IsBound())
    {
        OnSkillCooldownStarted.Broadcast(SkillID, Data.Cooldown);
    }

    UE_LOG(LogTemp, Log, TEXT("SkillID %d Cooldown Started: %.1f seconds"), SkillID, Data.Cooldown);
}

float UT3SkillComponentBase::GetRemainingCooldown(int32 SkillID)
{
    FSkillData* Data = GetSkillDataByID(SkillID);
    if (!Data) return 0.f;

    float ElapsedTime = GetWorld()->GetTimeSeconds() - Data->LastActivatedTime;
    float Remaining = Data->Cooldown - ElapsedTime;

    return (Remaining > 0.f) ? Remaining : 0.f;
}

float UT3SkillComponentBase::GetCooldownRemainingRatio(int32 SkillID)
{
    // 1. ID로 스킬 데이터 찾기
    const FSkillData* Data = GetSkillDataByID(SkillID);

    if (!Data || Data->Cooldown <= 0.f) return 0.f;

    float CurrentTime = GetWorld()->GetTimeSeconds();
    float ElapsedTime = CurrentTime - Data->LastActivatedTime;

    // 2. 남은 시간 계산
    float RemainingTime = Data->Cooldown - ElapsedTime;

    if (RemainingTime <= 0.f) return 0.f;

    // 3. 0~1 사이의 비율 반환 (UI 프로그레스바용)
    return FMath::Clamp(RemainingTime / Data->Cooldown, 0.f, 1.0f);
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

void UT3SkillComponentBase::OnSkillMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{

}


