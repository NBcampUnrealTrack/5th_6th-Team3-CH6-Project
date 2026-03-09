// T3StrongWind.cpp


#include "Player/Taoist/T3StrongWind.h"
#include "Components/BoxComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystemComponent.h"
#include "Player/T3CharacterBase.h"
#include "Player/T3CombatComponent.h"



AT3StrongWind::AT3StrongWind()
{
    PrimaryActorTick.bCanEverTick = false; 

    // 박스 콜리젼
    AttackArea = CreateDefaultSubobject<UBoxComponent>(TEXT("AttackArea"));
    RootComponent = AttackArea;
    AttackArea->SetBoxExtent(FVector(500.f, 50.f, 50.f));
    AttackArea->SetCollisionProfileName(TEXT("OverlapAllDynamic"));

    ParticleComp = CreateDefaultSubobject<UParticleSystemComponent>(TEXT("ParticleComp"));
    ParticleComp->SetupAttachment(RootComponent);

    InitialLifeSpan = SpawnTime;
}

void AT3StrongWind::BeginPlay()
{
    Super::BeginPlay();
    ImmediateDamageCheck();
    AttackArea->OnComponentBeginOverlap.AddDynamic(this, &AT3StrongWind::OnOverlapBegin);

}

void AT3StrongWind::SetDamage(float InDamage)
{
    Damage = InDamage;
}

void AT3StrongWind::ImmediateDamageCheck()
{
    TArray<AActor*> OverlappingActors;
    AttackArea->GetOverlappingActors(OverlappingActors);

    for (AActor* Actor : OverlappingActors)
    {
        ProcessHit(Actor, TEXT("Initial Hit"));
    }
}

void AT3StrongWind::OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
    UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
    ProcessHit(OtherActor, TEXT("Late Hit"));
}

// 중복 코드를 방지하고 가독성을 높이기 위한 타격 처리 함수 분리
void AT3StrongWind::ProcessHit(AActor* TargetActor, const FString& HitType)
{
    if (TargetActor && TargetActor != GetOwner() && !AlreadyHitActors.Contains(TargetActor))
    {
        
        AT3CharacterBase* OwnerChar = Cast<AT3CharacterBase>(GetOwner());
        if (!OwnerChar) return;

        UT3CombatComponent* Combat = OwnerChar->GetCombatComponent();
        if (!Combat) return;

        Combat->RequestAttackDamage(TargetActor, Damage);

        AlreadyHitActors.Add(TargetActor); // 중복 히트 리스트에 추가

        UE_LOG(LogTemp, Display, TEXT("%s: %s"), *HitType, *TargetActor->GetName());
        UE_LOG(LogTemp, Warning, TEXT("Attempting to Apply Damage: %.1f to %s"), Damage, *TargetActor->GetName());
    }
}