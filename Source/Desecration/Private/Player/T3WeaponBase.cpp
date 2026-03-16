// T3WeaponBase.cpp

#include "Player/T3WeaponBase.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Player/T3CharacterBase.h"
#include "Player/T3CombatComponent.h"
#include "Monster/T3BossMonster.h"


AT3WeaponBase::AT3WeaponBase()

{
    WeaponMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WeaponMesh"));

    WeaponSkeletalMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("WeaponSkeletalMesh"));
    WeaponSkeletalMesh->SetupAttachment(RootComponent);
    RootComponent = WeaponMesh;

    WeaponCollision = CreateDefaultSubobject<UBoxComponent>(TEXT("WeaponCollision"));
    WeaponCollision->SetupAttachment(RootComponent);

    WeaponCollision->SetGenerateOverlapEvents(true);

    // 초기 상태는 충돌 무시
    WeaponCollision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    WeaponCollision->SetCollisionProfileName(TEXT("Weapon")); // 전용 프로파일 설정
}


void AT3WeaponBase::BeginPlay()
{
    Super::BeginPlay();
    OwnerChar = Cast<AT3CharacterBase>(GetOwner());
    Combat = OwnerChar->GetCombatComponent();


    if (IsValid(WeaponCollision))
    {
        WeaponCollision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        WeaponCollision->OnComponentBeginOverlap.AddDynamic(this, &AT3WeaponBase::OnWeaponOverlap);
        UE_LOG(LogTemp, Warning, TEXT("Overlap Delegate Bound Successfully!"));
    }
}

void AT3WeaponBase::SetWeaponCollisionEnabled(bool bEnabled, float InDamageMultiplier, TSubclassOf<UT3DamageType_Base> InType, EHitIntensity InIntensity, float InStunAmount, float StaminaAmount)
{
   
    if (bEnabled && OwnerChar && WeaponCollision)
    {
        CurrentAttackDamage = OwnerChar->GetAttackPower() * InDamageMultiplier;
        CurrentDamageType = InType;
        CurrentIntensity = InIntensity;
        StunAmount = InStunAmount;
        WeaponCollision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);

        // [디버그 로그]
        FString CollisionState = (WeaponCollision->GetCollisionEnabled() == ECollisionEnabled::QueryOnly) ? TEXT("Enabled") : TEXT("Disabled");
       // GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Cyan, FString::Printf(TEXT("Weapon Collision: %s"), *CollisionState));
        UE_LOG(LogTemp, Display, TEXT("Weapon Collision: %s"), *CollisionState);
    }

    else if(IsValid(WeaponCollision))
    {
        WeaponCollision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        AlreadyHitActors.Empty();
        WeaponCollision->SetHiddenInGame(true);
    }
}

void AT3WeaponBase::OnWeaponOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
    if (OtherActor && !AlreadyHitActors.Contains(OtherActor))
    {
        // GetOwner()가 내 캐릭터(CharBase)를 가리키는지 확인
        if (OtherActor == GetOwner())
        {
            // 본인과 닿았을 때는 그냥 무시 (로그도 안 남기게)
            return;
        }
        // 적과 닿았을 때만 실행

        AlreadyHitActors.Add(OtherActor);
        //GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Cyan, TEXT("Overlap Success with Enemy!"));

        AT3BossMonster* HitBoss = Cast<AT3BossMonster>(OtherActor);

        if (IsValid(Combat))
        {
            Combat->RequestAttackDamage(OtherActor, CurrentAttackDamage, CurrentIntensity, 1.f, CurrentDamageType, StunAmount);
            UE_LOG(LogTemp, Warning, TEXT("Hit Monster! Damage: %.1f"), CurrentAttackDamage);
        }
        UE_LOG(LogTemp, Log, TEXT("Hit: %s"), *OtherActor->GetName());
    }

}