// T3BossProjectile.cpp

#include "Monster/T3BossProjectile.h"
#include "Desecration.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/DamageEvents.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "NiagaraComponent.h"

AT3BossProjectile::AT3BossProjectile()
{
	// 콜리전
	CollisionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("CollisionBox"));
	SetRootComponent(CollisionBox);
	CollisionBox->InitBoxExtent(CollisionExtent);
	CollisionBox->SetGenerateOverlapEvents(true);
	CollisionBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	CollisionBox->SetCollisionObjectType(ECC_WorldDynamic);
	CollisionBox->SetCollisionResponseToAllChannels(ECR_Ignore);
	CollisionBox->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);

	// 투사체 이동
	MovementComp = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("MovementComp"));
	MovementComp->UpdatedComponent = CollisionBox;
	MovementComp->bRotationFollowsVelocity = true;
	MovementComp->ProjectileGravityScale = 0.f;

	// Niagara 이펙트 (BP에서 에셋 설정)
	ProjectileEffect = CreateDefaultSubobject<UNiagaraComponent>(TEXT("ProjectileEffect"));
	ProjectileEffect->SetupAttachment(RootComponent);
	ProjectileEffect->SetAutoActivate(true);

	// 임시 디버그 메시 (큐브)
	DebugMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DebugMesh"));
	DebugMesh->SetupAttachment(RootComponent);
	DebugMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	DebugMesh->SetRelativeScale3D(FVector(0.6f, 1.6f, 0.8f));

	// 기본 큐브 메시 로드
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(
		TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMesh.Succeeded())
	{
		DebugMesh->SetStaticMesh(CubeMesh.Object);
	}

	// 기본 머티리얼 — 반투명 붉은색
	static ConstructorHelpers::FObjectFinder<UMaterial> DefaultMat(
		TEXT("/Engine/EngineMaterials/DefaultMaterial.DefaultMaterial"));
	if (DefaultMat.Succeeded())
	{
		DebugMesh->SetMaterial(0, DefaultMat.Object);
	}

	// 자동 소멸
	InitialLifeSpan = 3.0f;
}

void AT3BossProjectile::BeginPlay()
{
	Super::BeginPlay();

	if (CollisionBox)
	{
		CollisionBox->OnComponentBeginOverlap.AddDynamic(this, &AT3BossProjectile::OnProjectileOverlap);
	}

	// Niagara 에셋이 설정되어 있으면 디버그 메시 숨김
	if (ProjectileEffect && ProjectileEffect->GetAsset())
	{
		if (DebugMesh)
		{
			DebugMesh->SetVisibility(false);
		}
	}
}

void AT3BossProjectile::InitializeProjectile(float InDamage, float InSpeed,
	EHitIntensity InIntensity, TSubclassOf<UT3DamageType_Base> InDamageType)
{
	Damage = InDamage;
	HitIntensity = InIntensity;
	DamageTypeClass = InDamageType;

	if (MovementComp)
	{
		MovementComp->InitialSpeed = InSpeed;
		MovementComp->MaxSpeed = InSpeed;
		MovementComp->Velocity = GetActorForwardVector() * InSpeed;
		MovementComp->UpdateComponentVelocity();
	}

	UE_LOG(LogDesecration, Verbose, TEXT("T3_BossProjectile: 초기화 (데미지:%.0f, 속도:%.0f)"),
		Damage, InSpeed);
}

void AT3BossProjectile::OnProjectileOverlap(UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
	bool bFromSweep, const FHitResult& SweepResult)
{
	// Owner(보스) 및 중복 히트 방지
	if (!OtherActor || OtherActor == GetOwner() || HitActors.Contains(OtherActor))
	{
		return;
	}

	HitActors.Add(OtherActor);

	// DamageType 설정
	TSubclassOf<UDamageType> FinalDamageType = DamageTypeClass
		? DamageTypeClass.Get()
		: UT3DamageType_Base::StaticClass();

	// FT3DamageEvent로 TakeDamage 직접 호출
	FT3DamageEvent DamageEvent(FinalDamageType);
	DamageEvent.HitIntensity = HitIntensity;
	DamageEvent.HitDamageMultiplier = 1.0f;

	OtherActor->TakeDamage(
		Damage,
		DamageEvent,
		GetInstigatorController(),
		GetOwner()  // DamageCauser = 보스
	);

	UE_LOG(LogDesecration, Log, TEXT("T3_BossProjectile: %s에게 TakeDamage (데미지:%.0f, 강도:%s)"),
		*OtherActor->GetName(), Damage, *UEnum::GetValueAsString(HitIntensity));

	// 히트 후 콜리전 비활성화 + 지연 소멸
	if (CollisionBox)
	{
		CollisionBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
	SetLifeSpan(0.5f);
}
