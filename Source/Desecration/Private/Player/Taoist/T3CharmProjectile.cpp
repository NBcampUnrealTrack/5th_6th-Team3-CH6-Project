// T3CharmProjectile.cpp


#include "Player/Taoist/T3CharmProjectile.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "Kismet/KismetSystemLibrary.h"


AT3CharmProjectile::AT3CharmProjectile()
{
    PrimaryActorTick.bCanEverTick = true;

    // 컬리젼 설정
    CollisionComp = CreateDefaultSubobject<USphereComponent>(TEXT("SphereComp"));
    CollisionComp->InitSphereRadius(15.0f);
    CollisionComp->SetCollisionProfileName(TEXT("Projectile"));
    RootComponent = CollisionComp;

    // 메시 설정
    MeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComp"));
    MeshComp->SetupAttachment(RootComponent);
    MeshComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void AT3CharmProjectile::BeginPlay()
{
    AActor::BeginPlay();
}

void AT3CharmProjectile::LaunchCharm(FVector LaunchDirection, float Power)
{
    Velocity = LaunchDirection * Power;
    bIsActive = true;

    // 2초 뒤에는 바닥에 안 닿아도 무조건 터지게 예약
    GetWorldTimerManager().SetTimer(ExplosionTimerHandle, this, &AT3CharmProjectile::HandleExplosion, 1.0f, false);
}

void AT3CharmProjectile::Tick(float DeltaTime)
{
    AActor::Tick(DeltaTime);

    if (!bIsActive) return;

    // 1. 물리 계산 (중력 등)
    float Gravity = GetWorld()->GetGravityZ();
    Velocity.Z += Gravity * GravityScale * DeltaTime;
    Velocity.X *= HorizontalDamping;
    Velocity.Y *= HorizontalDamping;

    FVector NewLocation = GetActorLocation() + (Velocity * DeltaTime);

    // 2. 나풀거리는 좌우 흔들림 (Sway)
    float Time = GetWorld()->GetTimeSeconds();
    float Sway = FMath::Sin(Time * SwaySpeed) * SwayIntensity;
    NewLocation += GetActorRightVector() * Sway * DeltaTime;

    SetActorLocation(NewLocation);

    // 3. 회전 연출 (Roll과 Pitch를 섞어서 출렁이게)
    FRotator NewRot = GetActorRotation();

    // Pitch는 위아래로 끄덕끄덕
    NewRot.Pitch += FMath::Sin(Time * SwaySpeed * 1.2f) * 3.0f;

    // Roll은 좌우로 까닥까닥 (이게 들어가야 빳빳함이 사라짐)
    NewRot.Roll = FMath::Cos(Time * SwaySpeed) * (SwayIntensity * 0.5f);

    // 속도에 따라 살짝 기울어지게 추가 (선택 사항)
    NewRot.Yaw += Sway * 0.1f;

    SetActorRotation(NewRot);
}


void AT3CharmProjectile::HandleExplosion()
{
    if (!bIsActive) return;
    bIsActive = false;

    FVector SpawnLocation = GetActorLocation();

    // [LineTrace] 바닥 위치 정확히 찾기
    FHitResult HitResult;
    FVector Start = GetActorLocation();
    FVector End = Start + (FVector::DownVector * 500.0f); // 아래로 5m 레이저 발사
    FCollisionQueryParams Params;
    Params.AddIgnoredActor(this);

    if (GetWorld()->LineTraceSingleByChannel(HitResult, Start, End, ECC_Visibility, Params))
    {
        // 바닥을 찾았다면 해당 위치로 스폰 지점 변경
        SpawnLocation = HitResult.Location;
    }

    if (OnCharmExploded.IsBound())
    {
        OnCharmExploded.Execute(GetActorLocation(), GetOwner());
    }

    // 1. 나이아가라 이펙트 재생 (펑!)
    if (SpawnEffect)
    {
        UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), SpawnEffect, SpawnLocation+FVector(0.f, 0.f, 200.f));
    }

    // 2. 실제 액터(분신/호랑이) 소환
    if (ActorToSpawn)
    {
        FActorSpawnParameters SpawnParams;
        SpawnParams.Owner = GetOwner();
        SpawnParams.Instigator = GetInstigator();

        GetWorld()->SpawnActor<AActor>(ActorToSpawn, SpawnLocation, FRotator::ZeroRotator, SpawnParams);
    }

    // 3. 부적 파괴
    Destroy();
}