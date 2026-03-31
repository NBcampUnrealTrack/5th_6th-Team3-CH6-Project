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
	bIsAlreadyExploded = false;
	
	SphereComponent = CreateDefaultSubobject<USphereComponent>(TEXT("SphereComponent"));
	SphereComponent->InitSphereRadius(500.0f);
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
	float CurrentAttackPower = OwnerChar->GetAttackPower();
	float FinalDamage = CurrentAttackPower * (ExplosionDamage * FMath::Max(1, ChargeLevel));
	
	int32 ActualCharge = FMath::Max(1, ChargeLevel);
	
	TArray<AActor*> OverlappedActors;
	SphereComponent->GetOverlappingActors(OverlappedActors);
	bIsAlreadyExploded = true;

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
	OnLunarExplosion(); 
}

void AT3LunarSlash::BeginPlay()
{
	Super::BeginPlay();
	
	OwnerChar = Cast<AT3CharacterBase>(GetOwner());
	
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
	
	if (OwnerChar)
	{
		TObjectPtr<UT3CombatComponent> Combat = OwnerChar->GetCombatComponent();
		
		for (AActor* Actor : OverlappedActors)
		{
			if (Actor && Actor != OwnerChar)
			{
				float CurrentAttackPower = OwnerChar->GetAttackPower();
				Combat->RequestAttackDamage(Actor, (CurrentAttackPower * DamageRate));
				OnDoTAttack();
			}
		}
	}
}


