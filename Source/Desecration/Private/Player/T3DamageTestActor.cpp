// T3DamageTestActor.cpp

#include "Player/T3DamageTestActor.h"
#include "Components/BoxComponent.h"
#include "Kismet/GameplayStatics.h"

AT3DamageTestActor::AT3DamageTestActor()
{
    InteractionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("InteractionBox"));
    RootComponent = InteractionBox;

    // 겹치기만 해도 데미지를 주도록 설정
    InteractionBox->SetCollisionProfileName(TEXT("Trigger"));
}

void AT3DamageTestActor::BeginPlay()
{
    Super::BeginPlay();
    InteractionBox->OnComponentBeginOverlap.AddDynamic(this, &AT3DamageTestActor::OnOverlapBegin);
}

void AT3DamageTestActor::OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
    UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
    bool bFromSweep, const FHitResult& SweepResult)
{
    if (OtherActor && OtherActor != this)
    {
        // 1번(방향), 3번(타입) 테스트를 위해 ApplyDamage 호출
        // 이 액터의 위치가 가해자 위치가 됨
        UGameplayStatics::ApplyDamage(
            OtherActor,
            DamageAmount,
            GetInstigatorController(),
            this,
            DamageTypeClass
        );

        // 시각적 확인
        DrawDebugString(GetWorld(), GetActorLocation(), TEXT("Damage Applied!"), nullptr, FColor::White, 2.f);
    }
}