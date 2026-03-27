// T3TalismanProjectile.cpp

#include "Player/Taoist/T3TalismanProjectile.h"
#include "Components/BoxComponent.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Player/T3CombatComponent.h"
#include "Player/T3CharacterBase.h"
#include "Player/Taoist/T3Taoist_SkillComponent.h"
#include "Player/Taoist/T3TaoistClone.h"

AT3TalismanProjectile::AT3TalismanProjectile()
{
    PrimaryActorTick.bCanEverTick = true;

    // 1. 충돌 박스 설정
    CollisionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("CollisionBox"));
    RootComponent = CollisionBox;

    // 2. 메시 설정
    TalismanMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TalismanMesh"));
    TalismanMesh->SetupAttachment(RootComponent);
    TalismanMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    TalismanMesh->SetCollisionResponseToAllChannels(ECR_Ignore);

    // 3. 투사체 컴포넌트 (속도, 중력 등 설정)
    ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
    ProjectileMovement->InitialSpeed = 1500.f;
    ProjectileMovement->MaxSpeed = 1500.f;
    ProjectileMovement->ProjectileGravityScale = 0.0f;

    // n초 후 자동 소멸
    InitialLifeSpan = 0.8f;
}

void AT3TalismanProjectile::BeginPlay()
{
    Super::BeginPlay();


    if (CollisionBox)
    {
        CollisionBox->OnComponentBeginOverlap.AddDynamic(this, &AT3TalismanProjectile::OnTalismanOverlap);
    }

    // 초기 상대 위치 저장
    if (TalismanMesh)
    {
        InitialRelativeLocation = TalismanMesh->GetRelativeLocation();
    }
}

void AT3TalismanProjectile::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    RunningTime += DeltaTime;

    // 사인파 계산: y = A * sin(f * t)
    float SideOffset = Amplitude * FMath::Sin(RunningTime * Frequency);
    float UpOffset = (Amplitude * 0.5f) * FMath::Cos(RunningTime * Frequency * 0.5f); // 8자 형태 유도

    if (TalismanMesh)
    {
        // 메시만 살짝 흔들어줌
        FVector NewLocation = InitialRelativeLocation;
        NewLocation.Y += SideOffset;
        NewLocation.Z += UpOffset;
        TalismanMesh->SetRelativeLocation(NewLocation);

        // 진행 방향에 따른 회전값(Roll) 추가 시 더 흐물거려 보임
        FRotator NewRotation = TalismanMesh->GetRelativeRotation();
        NewRotation.Roll = SideOffset * 0.5f;
        TalismanMesh->SetRelativeRotation(NewRotation);
    }
}

void AT3TalismanProjectile::OnTalismanOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
    // 1. 유효성 검사
    if (!OtherActor || OtherActor == this) return; // 기본 유효성 및 자기 자신 제외

    // 부적끼리는 충돌하지 않도록 클래스 체크
    if (OtherActor->IsA(AT3TalismanProjectile::StaticClass())) return;

    // 본인 소유자(분신) 및 본체(CharacterBase) 제외
    APawn* MyInstigator = GetInstigator();
    if (OtherActor == MyInstigator) return;
    if (OtherActor->IsA(AT3CharacterBase::StaticClass())) return;
    if (OtherActor->IsA(AT3TaoistClone::StaticClass())) return;

    // 이미 히트된 액터거나 소유자라면 무시
    if (OtherActor == GetOwner() || HitActors.Contains(OtherActor)) return;

    AT3CharacterBase* OwnerChar = Cast<AT3CharacterBase>(GetOwner());
    if (!OwnerChar) return;

    UT3CombatComponent* Combat = OwnerChar->GetCombatComponent();
    if (!Combat) return;

    UT3Taoist_SkillComponent* TaoistSkill = Cast< UT3Taoist_SkillComponent>(Combat->GetSkillComponent());
    if (!TaoistSkill) return;

    // 2. 히트 리스트 추가
    HitActors.Add(OtherActor);

    // 3. 데미지 전달 
    
    {
        Combat->RequestAttackDamage(OtherActor, Damage * DamageMultiflier, EHitIntensity::Light, 1.f, nullptr, 0.f, bIsBasicAttack);
    }

    UE_LOG(LogTemp, Log, TEXT("[Talisman] Hit: %s, Damage: %f"), *OtherActor->GetName(), Damage);

    // 4. 폭발 연출 (Niagara)
    if (ExplosionEffect)
    {
        FVector EffectScale(1.f, 1.f, 1.f);
        UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), ExplosionEffect, GetActorLocation(), GetActorRotation(), EffectScale, true );
    }

    // 5. 소멸 처리 
    Destroy();
}
