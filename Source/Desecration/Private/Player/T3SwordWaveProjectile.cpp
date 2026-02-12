// T3SwordWaveProjectile.cpp

#include "Player/T3SwordWaveProjectile.h"
#include "Components/BoxComponent.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Player/T3CombatComponent.h"
#include "Player/T3CharacterBase.h"


AT3SwordWaveProjectile::AT3SwordWaveProjectile()
{
    // 1. 충돌체 설정
    BoxCollision = CreateDefaultSubobject<UBoxComponent>(TEXT("BoxComp"));
    SetRootComponent(BoxCollision);

    BoxCollision->InitBoxExtent(FVector(20.f, 100.f, 50.f));
    BoxCollision->SetGenerateOverlapEvents(true);
    BoxCollision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
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

    // 1초뒤에 자동으로 사라짐
    InitialLifeSpan = 1.0f;

}

void AT3SwordWaveProjectile::BeginPlay()
{
    Super::BeginPlay();

    if (BoxCollision)
    {
        BoxCollision->OnComponentBeginOverlap.AddDynamic(this, &AT3SwordWaveProjectile::OnProjectileOverlap);
    }
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

void AT3SwordWaveProjectile::OnProjectileOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
    // 1. 유효성 검사 및 방어적 코드
    if (!OtherActor || OtherActor == GetOwner() || HitActors.Contains(OtherActor)) return;

    TObjectPtr<AT3CharacterBase> OwnerChar = Cast<AT3CharacterBase>(GetOwner());
    if (!OwnerChar) return;

    TObjectPtr<UT3CombatComponent> Combat = OwnerChar->GetCombatComponent();
    if (!Combat) return;

    // 2. 중복 히트 방지 리스트 추가 및 로그
    HitActors.Add(OtherActor);
    UE_LOG(LogTemp, Warning, TEXT("[Projectile] Overlap with: %s"), *OtherActor->GetName());

    // 3. 데미지 전달
    Combat->RequestAttackDamage(OtherActor, Damage);

    // -----------------------------------------------------------
    // [감속 및 지연 소멸 연출]
    // -----------------------------------------------------------

    // A. 충돌 비활성화 (추가 히트 방지)
    if (BoxCollision)
    {
        BoxCollision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    }

    // B. 속도 감속 연출 
    //if (MovementComp)
    //{
    //    // 투사체만 슬로우 모션 (0.2배속)
    //    CustomTimeDilation = 0.2f;

    //    // 속도를 10%로 감속
    //    MovementComp->Velocity *= 0.1f;
    //    MovementComp->UpdateComponentVelocity(); 
    //}

    // C. 0.5초 뒤 소멸
    SetLifeSpan(0.5f);
}