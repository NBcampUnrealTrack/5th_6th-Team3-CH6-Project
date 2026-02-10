// T3Paladin_SkillComponent.cpp


#include "Player/T3Paladin_SkillComponent.h"
#include "GameFramework/Character.h"
#include "Engine/World.h" 
#include "Player/T3SwordWaveProjectile.h"
#include "Player/T3CharacterBase.h"

UT3Paladin_SkillComponent::UT3Paladin_SkillComponent()
{
}

void UT3Paladin_SkillComponent::ExecuteSkill(int32 SlotNumber)
{
    // 1. 어떤 스킬 ID가 들어왔는지 확인
    FSkillData* TargetData = (SlotNumber == 1) ? &SwordWaveData : &ShieldStrikeData; // 스킬 데이터 선택
    int32 SkillID = (SlotNumber == 1) ? Slot_1_SkillID : Slot_2_SkillID;

    // 2. 마나 & 쿨타임 체크
    if (!CanExecuteSkill(*TargetData)) return;

    // 3. 쿨타임 시작 및 스킬 실행
    StartCooldown(*TargetData);

    // ID에 따른 분기
    switch (SkillID)
    {
    case 0: // 검격
        ExecuteSwordWave();    break;
    case 1: // 방패찍기
        ShieldStrike();    break;
    case 2: // 도약찍기
        UE_LOG(LogTemp, Warning, TEXT("Flying Attack"));    break;
    case 3: // 신의심판
        UE_LOG(LogTemp, Warning, TEXT("Judge of God"));    break;
    default:
        UE_LOG(LogTemp, Warning, TEXT("Unknown Skill ID: %d"), SkillID);   break;
    }
}

void UT3Paladin_SkillComponent::ExecuteSwordWave()
{
    if (!OwnerChar || !SwordWaveData.SkillMontage)
    {
        UE_LOG(LogTemp, Display, TEXT("no montage"))
         return;
    }
    if (OwnerChar)
    {
        // 몽타주 재생 (애니메이션 기반 스킬 실행)
        OwnerChar->PlayAnimMontage(SwordWaveData.SkillMontage);
        UE_LOG(LogTemp, Display, TEXT("play sword wave"));
    }
}


void UT3Paladin_SkillComponent::ExecuteSkillNotify(int32 Index)
{
    // 공통 노티파이에서 보낸 Index에 따라 분기
    switch (Index)
    {
    case 0: // 검격 (Sword Wave)
        SpawnSwordWaveProjectile();
        break;
    case 1: // 방패찍기
        // UseShieldBash(); // 다음 단계에서 구현
        break;
        // ... 나머지 스킬들
    }
}


void UT3Paladin_SkillComponent::SpawnSwordWaveProjectile()
{
    if (!SwordWaveData.ProjectileClass) return;

    UWorld* World = GetWorld();
    if (World)
    {
        // 위치가 애매하면 소켓을 생성해서 소켓의 위치 가져오기
        FVector SpawnLocation = GetOwner()->GetActorLocation() + GetOwner()->GetActorForwardVector() * 100.f;
        FRotator SpawnRotation = GetOwner()->GetActorRotation();

        FActorSpawnParameters SpawnParams;
        SpawnParams.Owner = GetOwner();
        SpawnParams.Instigator = Cast<APawn>(GetOwner());

        SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

        AT3SwordWaveProjectile* Projectile = World->SpawnActor<AT3SwordWaveProjectile>(
            SwordWaveData.ProjectileClass,
            SpawnLocation,
            SpawnRotation,
            SpawnParams
        );

        if (Projectile)
        {
            //  데미지, 속도 전달
            float FinalDamage = SwordWaveData.DamageMultiflier * OwnerChar->GetAttackPower();
            Projectile->InitializeProjectile(FinalDamage, SwordWaveData.ProjectileSpeed);
            UE_LOG(LogTemp, Log, TEXT("Paladin SwordWave Launched!"));
        }
    }
}
