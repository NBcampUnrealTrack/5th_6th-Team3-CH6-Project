// T3Taoist_SkillComponent.cpp


#include "Player/Taoist/T3Taoist_SkillComponent.h"
#include "GameFramework/Character.h"
#include "Engine/World.h" 
#include "Player/Taoist/T3TalismanProjectile.h"
#include "Player/T3CharacterBase.h"
#include "Player/T3CombatComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/EngineTypes.h"
#include "Engine/OverlapResult.h"
#include "DrawDebugHelpers.h"
#include "Components/CapsuleComponent.h"

UT3Taoist_SkillComponent::UT3Taoist_SkillComponent()
{ }

FSkillData* UT3Taoist_SkillComponent::GetSkillDataByID(int32 SkillID)
{
    switch (SkillID)
    {
    case 1: return 0;
    default: return nullptr;
    }
}

void UT3Taoist_SkillComponent::ExecuteSkill(int32 SlotNumber)
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
    case 1: // 검격
        //ExecuteSwordWave();    break;

    default:
        UE_LOG(LogTemp, Warning, TEXT("Unknown Skill ID: %d"), SkillID);   break;
    }
}

void UT3Taoist_SkillComponent::OnSkillMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
    // 스킬 사용 상태 해제
    bUsingSkill = false;

    UE_LOG(LogTemp, Log, TEXT("Skill Montage Ended. bUsingSkill set to false. Interrupted: %s"), bInterrupted ? TEXT("True") : TEXT("False"));
}

void UT3Taoist_SkillComponent::ExecuteSkillNotify(int32 Index)
{
    // 공통 노티파이에서 보낸 Index에 따라 분기
    switch (Index)
    {
    case 5: // 기본 공격 (부적 날리기)
        
        SpawnTalisman();
        break;
    }
}

void UT3Taoist_SkillComponent::CancelCurrentSkill()
{
    
}

void UT3Taoist_SkillComponent::SpawnTalisman()
{
    if (!TalismanClass)
    {
        UE_LOG(LogTemp, Warning, TEXT("TalismanClass가 설정되지 않았습니다!"));
        return;
    }

    AT3CharacterBase* OwnerCharacter = Cast<AT3CharacterBase>(GetOwner());
    if (!OwnerCharacter) return;

    // 1. 스폰 위치 및 회전 설정
    // 캐릭터 전방 약 100cm 지점에서 스폰
    FVector SpawnLocation = OwnerCharacter->GetActorLocation() + OwnerCharacter->GetActorForwardVector() * 100.f;
    FRotator SpawnRotation = OwnerCharacter->GetActorRotation();

    // 2. 스폰 파라미터 설정 (충돌 방지 및 소유자 설정)
    FActorSpawnParameters SpawnParams;
    SpawnParams.Owner = OwnerCharacter;
    SpawnParams.Instigator = OwnerCharacter;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    // 3. 월드에 스폰
    UWorld* World = GetWorld();
    if (World)
    {
        AT3TalismanProjectile* Talisman = World->SpawnActor<AT3TalismanProjectile>(
            TalismanClass,
            SpawnLocation,
            SpawnRotation,
            SpawnParams
        );

        if (Talisman)
        {

        }
    }
}