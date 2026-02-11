// T3HealthComponent.cpp

#include "Monster/T3HealthComponent.h"
#include "Engine/Engine.h"
#include "Perception/AISense_Damage.h"

UT3HealthComponent::UT3HealthComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UT3HealthComponent::BeginPlay()
{
	Super::BeginPlay();
	ResetCurrentHP();
}

void UT3HealthComponent::ResetCurrentHP()
{
	CurrentHP = MaxHP;
}

void UT3HealthComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

}

void UT3HealthComponent::HandleTakeDamage(float DamageAmount, const FDamageEvent& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	const FT3DamageEvent* T3Event = static_cast<const FT3DamageEvent*>(&DamageEvent);

	if (T3Event)
	{
		EHitIntensity Intensity = T3Event->HitIntensity;
	}

	CurrentHP = FMath::Clamp(CurrentHP - DamageAmount, 0.f, MaxHP);
	UE_LOG(LogTemp, Log, TEXT("T3HealthComponent: Took %.2f damage. CurrentHP = %.2f"), DamageAmount, CurrentHP);

	if (DamageAmount > 0.f && DamageCauser)
	{
		UAISense_Damage::ReportDamageEvent(
			GetWorld(),
			GetOwner(),      // 피해자
			DamageCauser,    // 가해자
			DamageAmount,
			DamageCauser->GetActorLocation(),
			FVector::ZeroVector
		);
	}

	if (CurrentHP <= 0.f)
	{
		if (OnDeath.IsBound())
		{
			OnDeath.Broadcast();
		}
	}
	else
	{
		if (OnDamaged.IsBound())
		{
			OnDamaged.Broadcast(DamageAmount, DamageEvent.DamageTypeClass ? DamageEvent.DamageTypeClass->GetDefaultObject<UDamageType>() : nullptr, EventInstigator, DamageCauser);
		}
	}
}

