// T3Paladin_SkillComponent.cpp


#include "Player/T3Paladin_SkillComponent.h"
#include "GameFramework/Character.h"
#include "Engine/World.h" 
#include "Player/T3SwordWaveProjectile.h"

UT3Paladin_SkillComponent::UT3Paladin_SkillComponent()
{
}

void UT3Paladin_SkillComponent::ExecuteSwordWave()
{
    AActor* Owner = GetOwner();
    if (!Owner || !SwordWaveData.SkillMontage) return;

    ACharacter* Paladin = Cast<ACharacter>(Owner);
    if (Paladin)
    {
        // 몽타주 재생 (애니메이션 기반 스킬 실행)
        Paladin->PlayAnimMontage(SwordWaveData.SkillMontage);
    }
}

void UT3Paladin_SkillComponent::SpawnSwordWaveProjectile()
{
    if (!SwordWaveData.ProjectileClass)
    {
        UE_LOG(LogTemp, Warning, TEXT("SwordWave ProjectileClass is missing!"));
        return;
    }

    UWorld* World = GetWorld();
    if (World)
    {
        FVector SpawnLocation = GetOwner()->GetActorLocation() + GetOwner()->GetActorForwardVector() * 100.f;
        FRotator SpawnRotation = GetOwner()->GetActorRotation();

        FActorSpawnParameters SpawnParams;
        SpawnParams.Owner = GetOwner();
        SpawnParams.Instigator = Cast<APawn>(GetOwner());

        // 검기 생성
        AT3SwordWaveProjectile* Projectile = World->SpawnActor<AT3SwordWaveProjectile>(
            SwordWaveData.ProjectileClass,
            SpawnLocation,
            SpawnRotation,
            SpawnParams
        );

        if (Projectile)
        {
            // 데이터 전달 (초기화)
            // Projectile->InitializeProjectile(SwordWaveData.Damage, SwordWaveData.Speed);
        }
    }
}