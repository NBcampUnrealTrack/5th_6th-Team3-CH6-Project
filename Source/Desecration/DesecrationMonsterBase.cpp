// DesecrationMonsterBase.cpp


#include "DesecrationMonsterBase.h"
#include "Components/CapsuleComponent.h"
#include "Engine/DamageEvents.h"

// Sets default values
ADesecrationMonsterBase::ADesecrationMonsterBase()
{
	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	CurrentHP = 100.0f;
	bIsDead = false;
}

// Called when the game starts or when spawned
void ADesecrationMonsterBase::BeginPlay()
{
	Super::BeginPlay();

	if (MonsterDataTable && !MonsterRowName.IsNone())
	{
		static const FString ContextString(TEXT("Monster Stats Context"));
		FMonsterStats* Stats = MonsterDataTable->FindRow<FMonsterStats>(MonsterRowName, ContextString, true);

		if (Stats)
		{
			CurrentHP = Stats->MaxHP;
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("Monster Stats not found for RowName: %s"), *MonsterRowName.ToString());
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("MonsterDataTable or MonsterRowName is not set for %s"), *GetName());
	}
}

float ADesecrationMonsterBase::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	if (CurrentHP <= 0.0f || bIsDead)
	{
		return 0.0f;
	}

	float ActualDamage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);

	if (ActualDamage > 0.0f)
	{
		CurrentHP -= ActualDamage;

		if (CurrentHP <= 0.0f)
		{
			OnDeath();
		}
	}

	return ActualDamage;
}

void ADesecrationMonsterBase::OnDeath()
{
	bIsDead = true;

	// Disable collision on capsule
	if (GetCapsuleComponent())
	{
		GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	// Enable ragdoll
	if (GetMesh())
	{
		GetMesh()->SetSimulatePhysics(true);
		GetMesh()->SetCollisionProfileName(TEXT("Ragdoll"));
	}

	// Stop AI Logic
	DetachFromControllerPendingDestroy();

	// Destroy after 3 seconds
	SetLifeSpan(3.0f);
}

// Called every frame
void ADesecrationMonsterBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

// Called to bind functionality to input
void ADesecrationMonsterBase::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}
