// T3LunarSlash.cpp


#include "Player/Valkyrie/T3LunarSlash.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "TimerManager.h"
#include "Kismet/GameplayStatics.h"
#include "Player/T3CombatComponent.h"
#include "Player/T3CharacterBase.h"

AT3LunarSlash::AT3LunarSlash()
{
	PrimaryActorTick.bCanEverTick = false;
	
	SphereComponent = CreateDefaultSubobject<USphereComponent>(TEXT("SphereComponent"));
	SphereComponent->InitSphereRadius(400.0f);
	SphereComponent->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	RootComponent = SphereComponent;
	
	MoonMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MoonMesh"));
	MoonMesh->SetupAttachment(RootComponent);
	MoonMesh->SetCollisionProfileName(TEXT("NoCollision"));
	
	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
	ProjectileMovement->UpdatedComponent = SphereComponent;
	ProjectileMovement->InitialSpeed = 300.0f;
	ProjectileMovement->MaxSpeed = 300.0f;
	ProjectileMovement->ProjectileGravityScale = 0.0f;
	
	Tags.Add(TEXT("MoonSlash"));
	
	InitialLifeSpan = 8.0f;

}

void AT3LunarSlash::BeginPlay()
{
	Super::BeginPlay();
	
	GetWorldTimerManager().SetTimer(DamageTickTimer, this, &AT3LunarSlash::ApplyDamage, DamageTickRate, true);
	
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
			Combat->RequestAttackDamage(Actor, DamageRate);
		}
	}
}


