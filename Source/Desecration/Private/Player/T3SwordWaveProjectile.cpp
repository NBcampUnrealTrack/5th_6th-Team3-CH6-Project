// T3SwordWaveProjectile.cpp

#include "Player/T3SwordWaveProjectile.h"
#include "Components/BoxComponent.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Player/T3CharacterBase.h"
#include "Player/T3CombatComponent.h"


AT3SwordWaveProjectile::AT3SwordWaveProjectile()
{
    // 1. 충돌체 설정
    BoxCollision = CreateDefaultSubobject<UBoxComponent>(TEXT("BoxComp"));
    SetRootComponent(BoxCollision);

    BoxCollision->InitBoxExtent(FVector(20.f, 100.f, 50.f));
    BoxCollision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    BoxCollision->SetHiddenInGame(false);

    // 충돌 이벤트 바인딩
    BoxCollision->OnComponentBeginOverlap.AddDynamic(this, &AT3SwordWaveProjectile::OnOverlapBegin);

    // 2. 무브먼트 설정
    MovementComp = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
    MovementComp->UpdatedComponent = BoxCollision;
    MovementComp->bRotationFollowsVelocity = true;
    MovementComp->ProjectileGravityScale = 0.f;

    SkillEffect = CreateDefaultSubobject<UNiagaraComponent>(TEXT("SkillEffect"));
    SkillEffect->SetupAttachment(RootComponent);
    SkillEffect->SetActive(true);
    SkillEffect->SetAutoActivate(true);

    // 1초뒤에 자동으로 사라짐
    InitialLifeSpan = 1.0f;

}

void AT3SwordWaveProjectile::InitializeProjectile(float InDamage, float InSpeed)
{
    Damage = InDamage;
    if (MovementComp)
    {
        MovementComp->InitialSpeed = InSpeed;
        MovementComp->MaxSpeed = InSpeed;

        MovementComp->Velocity = GetActorForwardVector() * InSpeed;
        MovementComp->UpdateComponentVelocity();
    }
}

void AT3SwordWaveProjectile::PostInitializeComponents()
{
    Super::PostInitializeComponents();

    // 런타임 스폰 시 컴포넌트들이 루트에 붙어있는지 재확인 및 활성화
    if (SkillEffect)
    {
        SkillEffect->AttachToComponent(RootComponent, FAttachmentTransformRules::KeepRelativeTransform);
        SkillEffect->Activate(true); // 강제 활성화
    }
}

void AT3SwordWaveProjectile::OnOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
    TObjectPtr<AT3CharacterBase> OwnerChar = Cast<AT3CharacterBase>(GetOwner());
    TObjectPtr<UT3CombatComponent> Combat = OwnerChar->GetCombatComponent();
    if (!OwnerChar) return;
    // 자기 자신이나 생성자(Instigator)는 무시
    if (OtherActor && (OtherActor != this) && (OtherActor != GetInstigator()))
    {
        Combat->RequestAttackDamage(OtherActor, Damage);

    }
}