// T3Paladin_SkillComponent.cpp


#include "Player/T3Paladin_SkillComponent.h"
#include "GameFramework/Character.h"
#include "Engine/World.h" 
#include "Player/T3SwordWaveProjectile.h"
#include "Player/T3CharacterBase.h"

UT3Paladin_SkillComponent::UT3Paladin_SkillComponent()
{
}

FSkillData* UT3Paladin_SkillComponent::GetSkillDataByID(int32 SkillID)
{
    switch (SkillID)
    {
    case 1: return &SwordWaveData;
    case 2: return &ShieldStrikeData;
    // case 3: return &Data;
    // case 4: return &Data;
    default: return nullptr;
    }
}

void UT3Paladin_SkillComponent::ExecuteSkill(int32 SlotNumber)
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
        ExecuteSwordWave();    break;
    case 2: // 방패찍기
        ShieldStrike();    break;
    case 3: // 도약찍기
        UE_LOG(LogTemp, Warning, TEXT("Flying Attack"));    break;
    case 4: // 신의심판
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
    UAnimInstance* AnimInstance = OwnerChar->GetMesh()->GetAnimInstance();
    if (OwnerChar && AnimInstance)
    {
        // 1. 스킬 사용 시작 상태 설정
        bUsingSkill = true;

        // 2. 몽타주 재생 (애니메이션 기반 스킬 실행)
        float Duration = OwnerChar->PlayAnimMontage(SwordWaveData.SkillMontage);
        
        if (Duration > 0.f)
        {
            // 3. 몽타주 종료 델리게이트 바인딩
            FOnMontageEnded MontageEndedDelegate;
            MontageEndedDelegate.BindUObject(this, &UT3Paladin_SkillComponent::OnSkillMontageEnded);
            AnimInstance->Montage_SetEndDelegate(MontageEndedDelegate, SwordWaveData.SkillMontage);
        }
        else
        {
            // 재생 실패 시 즉시 상태 초기화
            bUsingSkill = false;
        }
    }
}

void UT3Paladin_SkillComponent::OnSkillMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
    // 스킬 사용 상태 해제
    bUsingSkill = false;

    UE_LOG(LogTemp, Log, TEXT("Skill Montage Ended. bUsingSkill set to false. Interrupted: %s"), bInterrupted ? TEXT("True") : TEXT("False"));
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
        FVector SpawnLocation = GetOwner()->GetActorLocation() + GetOwner()->GetActorForwardVector() * 10.f;
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
