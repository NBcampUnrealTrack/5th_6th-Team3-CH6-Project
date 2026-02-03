#include "Monster/T3MonsterBase.h"

AT3MonsterBase::AT3MonsterBase()
{
	PrimaryActorTick.bCanEverTick = true;
}

void AT3MonsterBase::BeginPlay()
{
	Super::BeginPlay();
	HealthComponent = FindComponentByClass<UT3HealthComponent>();
}

void AT3MonsterBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AT3MonsterBase::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
}

float AT3MonsterBase::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	float ActualDamage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);

	if (HealthComponent)
	{
		HealthComponent->HandleTakeDamage(ActualDamage, DamageEvent, EventInstigator, DamageCauser);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("T3MonsterBase: HealthComponent is null when taking damage."));
	}

	return ActualDamage;
}

