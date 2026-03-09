// T3LunarSlash.cpp


#include "Player/Valkyrie/T3LunarSlash.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "TimerManager.h"
#include "Kismet/GameplayStatics.h"
#include "Player/T3CombatComponent.h"
#include "Player/T3CharacterBase.h"
#include "Player/T3WeaponBase.h"
#include "Player/Valkyrie/T3Valkyrie_SkillComponent.h"

AT3LunarSlash::AT3LunarSlash()
{
	PrimaryActorTick.bCanEverTick = false;
	
	SphereComponent = CreateDefaultSubobject<USphereComponent>(TEXT("SphereComponent"));
	SphereComponent->InitSphereRadius(300.0f);
	SphereComponent->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	RootComponent = SphereComponent;
	
	MoonMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MoonMesh"));
	MoonMesh->SetupAttachment(RootComponent);
	MoonMesh->SetCollisionProfileName(TEXT("NoCollision"));
	
	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
	ProjectileMovement->UpdatedComponent = SphereComponent;
	ProjectileMovement->InitialSpeed = 100.0f;
	ProjectileMovement->MaxSpeed = 100.0f;
	ProjectileMovement->ProjectileGravityScale = 0.0f;
	
	ColdAuraComponent = CreateDefaultSubobject<UNiagaraComponent>(TEXT("ColdAura"));
	ColdAuraComponent->SetupAttachment(RootComponent);
	
	Tags.Add(TEXT("MoonSlash"));
	
	InitialLifeSpan = 8.0f;

}

void AT3LunarSlash::OnLunarOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherOverlappedComponent, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!OtherActor ||OtherActor == this) return;
	if (OtherActor == GetOwner()) return;
	
	AT3WeaponBase* Weapon = Cast<AT3WeaponBase>(OtherActor);
	if (Weapon)
	{
		AT3CharacterBase* WeaponOwner = Cast<AT3CharacterBase>(Weapon->GetOwner());
		if (WeaponOwner && WeaponOwner == GetOwner())
		{
			UT3CombatComponent* Combat = WeaponOwner->GetCombatComponent();
			if (Combat && Combat->GetIsPowerStrike())
			{
				int32 CurrentCharge = 1;
				
				UT3Valkyrie_SkillComponent* Valkyrie_SkillComponent = WeaponOwner->FindComponentByClass<UT3Valkyrie_SkillComponent>();
				if (Valkyrie_SkillComponent)
				{
					CurrentCharge = Valkyrie_SkillComponent->ChargingLevel;
				}
				
				ExplodeLunarSlash(CurrentCharge);
			}
		}
	}
}

void AT3LunarSlash::ExplodeLunarSlash(int32 ChargeLevel)
{
	float FinalDamage = ExplosionDamage * FMath::Max(1, ChargeLevel);
	
	int32 ActualCharge = FMath::Max(1, ChargeLevel);
	UE_LOG(LogTemp, Warning, TEXT("=================================================="));
	UE_LOG(LogTemp, Warning, TEXT("💥 [달 폭발 시작] 차징 레벨: %d 단계!"), ActualCharge);
	UE_LOG(LogTemp, Warning, TEXT("💥 [데미지 계산] 기본(%.1f) x 차징(%d) = 최종 광역 데미지: %.1f"), ExplosionDamage, ActualCharge, FinalDamage);
	UE_LOG(LogTemp, Warning, TEXT("=================================================="));
	
	TArray<AActor*> OverlappedActors;
	SphereComponent->GetOverlappingActors(OverlappedActors);
	
	AT3CharacterBase* OwnerChar = Cast<AT3CharacterBase>(GetOwner());
	if (OwnerChar)
	{
		UT3CombatComponent* Combat = OwnerChar->GetCombatComponent();
		{
			if (Combat)
			{
				for (AActor* Actor : OverlappedActors)
				{
					if (Actor->IsA<AT3CharacterBase>()) continue;
					if (Actor && Actor != OwnerChar)
					{
						UE_LOG(LogTemp, Warning, TEXT("   -> 피격 대상: [%s] 에게 %.1f 데미지 전달 요청!"), *Actor->GetName(), FinalDamage);
						Combat->RequestAttackDamage(Actor, FinalDamage);
					}
				}
			}
		}
	}
	if (SphereComponent)
	{
		SphereComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
	
	if (MoonMesh)
	{
		MoonMesh->SetHiddenInGame(true);
	}
	
	if (ProjectileMovement)
	{
		ProjectileMovement->StopMovementImmediately();
	}
	
	if (ColdAuraComponent)
	{
		ColdAuraComponent->Deactivate();
	}
	
	SetLifeSpan(2.0f);
}

void AT3LunarSlash::BeginPlay()
{
	Super::BeginPlay();
	
	GetWorldTimerManager().SetTimer(DamageTickTimer, this, &AT3LunarSlash::ApplyDamage, DamageTickRate, true);
	
	if (SphereComponent)
	{
		SphereComponent->OnComponentBeginOverlap.AddDynamic(this, &AT3LunarSlash::OnLunarOverlap);
	}
	
}

void AT3LunarSlash::ApplyDamage()
{
	TArray<AActor*> OverlappedActors;
	SphereComponent->GetOverlappingActors(OverlappedActors);
	
	TObjectPtr<AT3CharacterBase> OwnerChar = Cast<AT3CharacterBase>(GetOwner());
	if (OwnerChar)
	{
		TObjectPtr<UT3CombatComponent> Combat = OwnerChar->GetCombatComponent();
		
		for (AActor* Actor : OverlappedActors)
		{
			if (Actor && Actor != OwnerChar)
			{
				Combat->RequestAttackDamage(Actor, DamageRate);
				OnDoTAttack();
			}
		}
	}
}


