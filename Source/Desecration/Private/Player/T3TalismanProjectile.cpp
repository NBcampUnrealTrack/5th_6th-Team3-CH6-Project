// T3TalismanProjectile.cpp

#include "Player/T3TalismanProjectile.h"
#include "Components/BoxComponent.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Player/T3CombatComponent.h"
#include "Player/T3CharacterBase.h"

AT3TalismanProjectile::AT3TalismanProjectile()
{
    PrimaryActorTick.bCanEverTick = true;

    // 1. 충돌 박스 설정
    CollisionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("CollisionBox"));
    RootComponent = CollisionBox;

    // 2. 메시 설정
    TalismanMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TalismanMesh"));
    TalismanMesh->SetupAttachment(RootComponent);

    // 3. 투사체 컴포넌트 (속도, 중력 등 설정)
    ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
    ProjectileMovement->InitialSpeed = 1500.f;
    ProjectileMovement->MaxSpeed = 1500.f;
    ProjectileMovement->ProjectileGravityScale = 0.0f;
}

void AT3TalismanProjectile::BeginPlay()
{
    Super::BeginPlay();
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
