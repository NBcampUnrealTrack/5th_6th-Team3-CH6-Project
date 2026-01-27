// T3Paladin_Weapon.cpp


#include "Player/T3Paladin_Weapon.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Kismet/GameplayStatics.h"

AT3Paladin_Weapon::AT3Paladin_Weapon()
{
    WeaponMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WeaponMesh"));
    RootComponent = WeaponMesh;

    WeaponCollision = CreateDefaultSubobject<UBoxComponent>(TEXT("WeaponCollision"));
    WeaponCollision->SetupAttachment(RootComponent);

    // 초기 상태는 충돌 무시
    WeaponCollision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    WeaponCollision->SetCollisionProfileName(TEXT("Weapon")); // 전용 프로파일 설정 권장
}

void AT3Paladin_Weapon::BeginPlay()
{
    Super::BeginPlay();
    WeaponCollision->OnComponentBeginOverlap.AddDynamic(this, &AT3Paladin_Weapon::OnWeaponOverlap);
}

void AT3Paladin_Weapon::SetWeaponCollisionEnabled(ECollisionEnabled::Type NewType)
{
    WeaponCollision->SetCollisionEnabled(NewType);
    if (NewType == ECollisionEnabled::NoCollision)
    {
        AlreadyHitActors.Empty(); // 공격 종료 시 타격 리스트 초기화
    }
}

void AT3Paladin_Weapon::OnWeaponOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
    if (OtherActor && !AlreadyHitActors.Contains(OtherActor))
    {
        // 내 자신이나 소유자는 제외
        if (OtherActor == GetOwner()) return;

        AlreadyHitActors.Add(OtherActor);

        // 데미지 처리 (Netmarble 스타일: ApplyDamage 활용)
        UGameplayStatics::ApplyDamage(OtherActor, 10.f, GetInstigatorController(), this, UDamageType::StaticClass());

        UE_LOG(LogTemp, Log, TEXT("Hit: %s"), *OtherActor->GetName());
    }
}