// T3SwordWaveProjectile.cpp

#include "Player/T3SwordWaveProjectile.h"
#include "Components/BoxComponent.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "GameFramework/ProjectileMovementComponent.h"


AT3SwordWaveProjectile::AT3SwordWaveProjectile()
{
    // 1. 충돌체 설정
    BoxCollision = CreateDefaultSubobject<UBoxComponent>(TEXT("BoxComp"));
    SetRootComponent(BoxCollision);

    BoxCollision->InitBoxExtent(FVector(20.f, 100.f, 50.f));
    BoxCollision->SetCollisionEnabled(ECollisionEnabled::NoCollision); // 일단 테스트용
    BoxCollision->SetHiddenInGame(false);


    // 2. 무브먼트 설정
    MovementComp = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
    MovementComp->UpdatedComponent = BoxCollision;
    MovementComp->bRotationFollowsVelocity = true;
    MovementComp->ProjectileGravityScale = 0.f;

    SkillEffect = CreateDefaultSubobject<UNiagaraComponent>(TEXT("SkillEffect"));
    SkillEffect->SetupAttachment(RootComponent);
    SkillEffect->SetActive(true);
    SkillEffect->SetAutoActivate(true);

}

void AT3SwordWaveProjectile::InitializeProjectile(float InDamage, float InSpeed)
{
    Damage = InDamage;
    if (MovementComp)
    {
        MovementComp->InitialSpeed = InSpeed;
        MovementComp->MaxSpeed = InSpeed;

        // 🚨 핵심: 속도를 변경했으니 컴포넌트를 다시 활성화하고 속도를 강제로 업데이트합니다.
        MovementComp->Velocity = GetActorForwardVector() * InSpeed;
        MovementComp->UpdateComponentVelocity();
    }
}