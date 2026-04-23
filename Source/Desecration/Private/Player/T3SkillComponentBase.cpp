// T3SkillComponentBase.cpp


#include "Player/T3SkillComponentBase.h"
#include "Player/T3CharacterBase.h"
#include "Player/T3CombatComponent.h"
#include "Kismet/GameplayStatics.h"

void UT3SkillComponentBase::CancelCurrentSkill()
{
}

void UT3SkillComponentBase::ExecuteSkillNotify(int32 Index)
{
}
void UT3SkillComponentBase::ExecuteSkill(int32 SkillSlot)
{
}

void UT3SkillComponentBase::ExecuteSkill_Completed(int32 SkillSlot)
{
}

void UT3SkillComponentBase::SetSkillSlot(int32 NewSkillID, bool bIsEquip)
{
    // 1. 장착/탈착 로직
    if (bIsEquip)
    {
        // 이미 장착된 스킬이면 무시
        if (EquippedSkillIDs.Contains(NewSkillID)) return;

        // 슬롯 꽉 참 (최대 4개)
        if (EquippedSkillIDs.Num() >= MAX_SKILL_SLOTS)
        {
            UE_LOG(LogTemp, Warning, TEXT("슬롯이 꽉 찼습니다. (최대 %d개)"), MAX_SKILL_SLOTS);
            return;
        }

        EquippedSkillIDs.Add(NewSkillID);
    }
    else
    {
        // 탈착: 배열에서 제거 (뒤 원소들이 앞으로 당겨짐)
        if (!EquippedSkillIDs.Remove(NewSkillID))
        {
            return; // 장착 안 된 스킬이면 무시
        }
    }

    // 2. 장착 상태 맵 업데이트
    SkillEquipStates.FindOrAdd(NewSkillID) = bIsEquip;

    // 3. UI 알림
    BroadcastSlotUpdated();

    // 4. 인벤토리 아이콘 갱신용 알림
    if (OnSkillEquipStateChanged.IsBound())
    {
        OnSkillEquipStateChanged.Broadcast(NewSkillID, bIsEquip);
    }

    UE_LOG(LogTemp, Log, TEXT("Slot Update Complete - Equipped: %d개, Current: %d, Next: %d"),
        EquippedSkillIDs.Num(), GetCurrentSkillSlot(), GetNextSkillSlot());
}

int32 UT3SkillComponentBase::GetSkillIDBySlotIndex(int32 Index) const
{
    // 1-based: Index 1 → EquippedSkillIDs[0] (현재), Index 2 → [1] (다음), ...
    int32 ArrayIndex = Index - 1;
    return EquippedSkillIDs.IsValidIndex(ArrayIndex) ? EquippedSkillIDs[ArrayIndex] : 0;
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

    // 스킬 상태 초기화

    // 기본적으로 1번 스킬은 해금 상태로 초기화
    if (!SkillUnlockStates.Contains(1))
    {
        SkillUnlockStates.Add(1, true);

        // UI 팀에게 알림
        if (OnSkillUnlockStateChanged.IsBound())
        {
            OnSkillUnlockStateChanged.Broadcast(1, true);
        }
    }

    // 나머지 2, 3, 4번이 없다면 false로 초기화
    for (int32 i = 2; i <= 4; ++i)
    {
        if (!SkillUnlockStates.Contains(i))
        {
            SkillUnlockStates.Add(i, false);

            if (OnSkillUnlockStateChanged.IsBound())
            {
                OnSkillUnlockStateChanged.Broadcast(i, false);
            }
        }
    }

    // 초기 스킬슬롯 설정 (1번 스킬만 장착, 나머지는 0)
    InitializeDefaultSlots();
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

    // 입력 상태 체크 (OwnerChar가 확실히 있을 때만 접근)
    if (OwnerChar->bUsingSkill || !OwnerChar->PlayerInputState.bCanAttack || Combat->GetCurrentState() == ECharacterCombatState::Dead) return false;
    
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

void UT3SkillComponentBase::PlaySkillEffectSound(USoundBase* Sound, float Volume)
{
    if (Sound && GetWorld())
    {
        UGameplayStatics::PlaySoundAtLocation(this, Sound, GetOwner()->GetActorLocation(), Volume);
    }
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
    // 장착 스킬이 1개 이하면 순환 불필요
    if (EquippedSkillIDs.Num() <= 1) return;

    // [0]을 뒤로 보내는 순환: [A,B,C,D] → [B,C,D,A]
    int32 TempID = EquippedSkillIDs[0];
    EquippedSkillIDs.RemoveAt(0);
    EquippedSkillIDs.Add(TempID);

    BroadcastSlotUpdated();

    UE_LOG(LogTemp, Log, TEXT("Skills Rotated! Current: %d, Next: %d"),
        GetCurrentSkillSlot(), GetNextSkillSlot());
}

void UT3SkillComponentBase::OnSkillMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{

}

void UT3SkillComponentBase::BasicAttackCount()
{
}

bool UT3SkillComponentBase::IsSkillUnlocked(int32 SkillID) const
{
    if (const bool* bUnlocked = SkillUnlockStates.Find(SkillID))
    {
        return *bUnlocked;
    }
    return false;
}

void UT3SkillComponentBase::BroadcastCurrentUnlockStates()
{
    for (auto& Pair : SkillUnlockStates)
    {
        OnSkillUnlockStateChanged.Broadcast(Pair.Key, Pair.Value);
    }
}

void UT3SkillComponentBase::SetSkillUnlockState(int32 SkillID, bool bUnlock)
{
    SkillUnlockStates.FindOrAdd(SkillID) = bUnlock;

    // UI 팀에게 알림
    if (OnSkillUnlockStateChanged.IsBound())
    {
        OnSkillUnlockStateChanged.Broadcast(SkillID, bUnlock);
    }

    UE_LOG(LogTemp, Log, TEXT("Skill %d Unlock State Changed: %s"), SkillID, bUnlock ? TEXT("Unlocked") : TEXT("Locked"));
}

void UT3SkillComponentBase::InitializeDefaultSlots()
{
    // 초기화: 1번 스킬만 장착
    EquippedSkillIDs.Empty();
    EquippedSkillIDs.Add(1);

    SkillEquipStates.Empty();
    SkillEquipStates.Add(1, true);

    if (OnSkillEquipStateChanged.IsBound())
    {
        OnSkillEquipStateChanged.Broadcast(1, true);
    }
}

void UT3SkillComponentBase::BroadcastSlotUpdated()
{
    if (!OnSkillSlotUpdated.IsBound()) return;

    // HUD는 슬롯 2개 고정: 1=현재([0]), 2=다음([1])
    int32 ID1 = GetCurrentSkillSlot();
    int32 ID2 = GetNextSkillSlot();

    FSkillData* D1 = GetSkillDataByID(ID1);
    FSkillData SafeD1 = D1 ? *D1 : FSkillData();
    OnSkillSlotUpdated.Broadcast(1, ID1, SafeD1);

    FSkillData* D2 = GetSkillDataByID(ID2);
    FSkillData SafeD2 = D2 ? *D2 : FSkillData();
    OnSkillSlotUpdated.Broadcast(2, ID2, SafeD2);
}

bool UT3SkillComponentBase::IsSkillEquipped(int32 SkillID) const
{
    // 맵에서 찾아서 반환 (없으면 false)
    if (const bool* bEquipped = SkillEquipStates.Find(SkillID))
    {
        return *bEquipped;
    }
    return false;
}