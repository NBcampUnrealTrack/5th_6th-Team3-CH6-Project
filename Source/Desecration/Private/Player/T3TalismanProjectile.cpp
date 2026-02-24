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
    // 1. 충돌 박스 설정
    CollisionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("CollisionBox"));
    RootComponent = CollisionBox;

    // 2. 메시 설정
    TalismanMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TalismanMesh"));
    TalismanMesh->SetupAttachment(RootComponent);

    // 3. 투사체 컴포넌트 (속도, 중력 등 설정)
    ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
    ProjectileMovement->InitialSpeed = 2000.f;
    ProjectileMovement->MaxSpeed = 2000.f;
    ProjectileMovement->ProjectileGravityScale = 0.1f; // 약간 둥실 떠가는 느낌
}
