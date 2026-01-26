// T3DamageTestActor.cpp

#include "Player/T3DamageTestActor.h"
#include "Components/BoxComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/DamageEvents.h"
#include "Components/WidgetComponent.h"
#include "Player/T3CharacterBase.h"

AT3DamageTestActor::AT3DamageTestActor()
{

    CurrentHP = MaxHP;
    SetCanBeDamaged(true); // 데미지를 받을 수 있도록 설정

    LockOnWidgetComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("LockOnWidget"));
    LockOnWidgetComponent->SetupAttachment(RootComponent);
    LockOnWidgetComponent->SetWidgetSpace(EWidgetSpace::Screen); // 위젯이 항상 카메라를 바라보게 설정
    LockOnWidgetComponent->SetVisibility(false); // 처음엔 숨김

    WeaponMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("WeaponMesh"));
    WeaponMesh->SetupAttachment(RootComponent);

    DamageTypeClass = UDamageType::StaticClass();
}

void AT3DamageTestActor::BeginPlay()
{
    Super::BeginPlay();
}


float AT3DamageTestActor::TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser)
{
    // 부모 클래스의 기본 로직 수행
    float ActualDamage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);

    if (ActualDamage <= 0.f || CurrentHP <= 0.f) return 0.f;

    // 체력 차감
    CurrentHP = FMath::Max(0.f, CurrentHP - ActualDamage);

    // 로그 출력용 정보 추출
    FString DamageTypeName = TEXT("None");
    if (DamageEvent.DamageTypeClass)
    {
        DamageTypeName = DamageEvent.DamageTypeClass->GetName();
    }

    FString CauserName = DamageCauser ? DamageCauser->GetName() : TEXT("Unknown");

    // 상세 로그 출력 (화면 및 로그창)
    FString DebugInfo = FString::Printf(TEXT("=== HIT REPORT ===\nCauser: %s\nDamage: %.1f\nType: %s\nHP: %.1f / %.1f"),
        *CauserName, ActualDamage, *DamageTypeName, CurrentHP, MaxHP);

    // 화면에 띄우기
    GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Cyan, DebugInfo);

    // 로그창에 남기기 (냉정한 피드백: 실무에선 UE_LOG가 필수입니다)
    UE_LOG(LogTemp, Warning, TEXT("%s"), *DebugInfo);

    // 시각적 피드백: 맞을 때마다 구체 그리기
    DrawDebugSphere(GetWorld(), GetActorLocation(), 50.f, 12, FColor::Yellow, false, 0.5f);

    if (CurrentHP <= 0.f)
    {
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("TestActor Destroyed (HP 0)"));
    }

    return ActualDamage;
}

void AT3DamageTestActor::SetLockOnWidgetVisible(bool bVisible)
{
    if (LockOnWidgetComponent)
    {
        LockOnWidgetComponent->SetVisibility(bVisible);
    }
}

void AT3DamageTestActor::ExecuteTestAttack()
{

    if (!WeaponMesh) return;

    // 1. 이미지에 설정했던 소켓 위치 가져오기
    FVector Start = WeaponMesh->GetSocketLocation(TEXT("Start_Socket"));
    FVector End = WeaponMesh->GetSocketLocation(TEXT("End_Socket"));



    TArray<FHitResult> HitResults;
    FCollisionQueryParams Params;
    Params.AddIgnoredActor(this);

    // 2. 플레이어를 찾기 위한 스윕 (ECC_Pawn 채널 사용)
    bool bHit = GetWorld()->SweepMultiByChannel(
        HitResults, Start, End, FQuat::Identity, ECC_Pawn,
        FCollisionShape::MakeSphere(AttackRadius), Params
    );

    // 디버그 라인 
    DrawDebugCapsule(GetWorld(), (Start + End) * 0.5f, (End - Start).Size() * 0.5f + AttackRadius,
        AttackRadius, FRotationMatrix::MakeFromZ(End - Start).ToQuat(), FColor::Yellow, false, 0.5f);

    if (bHit)
    {
        // 핵심: 이번 공격 프레임에서 이미 맞은 액터를 저장할 바구니
        TSet<AActor*> HitActors;

        for (auto& Hit : HitResults)
        {
            AActor* Target = Hit.GetActor();

            // 1. 타겟 유효성 확인 
            // 2. T3CharacterBase인지 확인 
            // 3. ★이미 이 바구니(Set)에 들어있는지 확인★
            if (IsValid(Target) && Target->IsA(AT3CharacterBase::StaticClass()) && !HitActors.Contains(Target))
            {
                // 바구니에 추가해서 중복 타격 방지
                HitActors.Add(Target);

                UGameplayStatics::ApplyDamage(
                    Target,
                    TestDamageAmount,
                    GetInstigatorController(),
                    this,
                    DamageTypeClass
                );

                DrawDebugString(GetWorld(), Hit.ImpactPoint, TEXT("HIT PLAYER!"), nullptr, FColor::Red, 1.0f);
            }
        }
    }
}