// T3BossProjectile.cpp

#include "Monster/T3BossProjectile.h"
#include "Desecration.h"
#include "Components/AudioComponent.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/DamageEvents.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"
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

	// BP에서 변경된 CollisionExtent 반영 — 콜리전 + 디버그 메시 동기화
	if (CollisionBox)
	{
		CollisionBox->SetBoxExtent(CollisionExtent);
		CollisionBox->OnComponentBeginOverlap.AddDynamic(this, &AT3BossProjectile::OnProjectileOverlap);
	}

	if (DebugMesh)
	{
		// 기본 큐브 = 100x100x100, CollisionExtent는 반크기 → Scale = Extent * 2 / 100
		DebugMesh->SetRelativeScale3D(CollisionExtent * 2.f / 100.f);
	}

	// Niagara 에셋이 설정되어 있으면 디버그 메시 숨김 (에디터에서는 항상 표시)
	if (ProjectileEffect && ProjectileEffect->GetAsset())
	{
#if !WITH_EDITOR
		if (DebugMesh)
		{
			DebugMesh->SetVisibility(false);
		}
#endif
	}

	// 비행 루프 사운드 — 투사체에 붙어서 3D 위치 추적
	if (LoopSound)
	{
		LoopAudioComponent = UGameplayStatics::SpawnSoundAttached(
			LoopSound, RootComponent, NAME_None,
			FVector::ZeroVector, EAttachLocation::KeepRelativeOffset,
			true,  // bStopWhenAttachedToDestroyed — 투사체 소멸 시 사운드 정지
			LoopVolumeMultiplier, 1.f, 0.f,
			SoundAttenuation);  // 3D 거리 감쇠
	}
}

void AT3BossProjectile::Destroyed()
{
	// 안전장치 — 어떤 경로로든 소멸 시 루프 사운드 정지
	if (LoopAudioComponent && LoopAudioComponent->IsPlaying())
	{
		LoopAudioComponent->Stop();
	}

	Super::Destroyed();
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

	// 충돌 사운드 재생
	if (ImpactSound)
	{
		UGameplayStatics::PlaySoundAtLocation(
			this, ImpactSound, GetActorLocation(),
			ImpactVolumeMultiplier, 1.f, 0.f,
			SoundAttenuation);
	}

	// 비행 루프 사운드 정지
	if (LoopAudioComponent)
	{
		LoopAudioComponent->Stop();
	}

	// 히트 후 콜리전 비활성화 + 지연 소멸
	if (CollisionBox)
	{
		CollisionBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
	SetLifeSpan(0.5f);
}
