// Fill out your copyright notice in the Description page of Project Settings.


#include "Monster/T3BossMonster.h"
#include "Engine/DamageEvents.h"
#include "GameFramework/Controller.h"
#include "Player/T3DamageTypes.h"

void AT3BossMonster::Damage(float DamageAmount, float StunAmount)
{
	if (DamageAmount > 0 && !bSuperPattern) 
	{
		BossStats.CurrentHP -= DamageAmount;
		OnBossDamaged.Broadcast();

		if (!bBossStun ) 
		{
			BossStats.CurrentStunGauge += StunAmount;
		}
		
		if (BossStats.CurrentHP <= 0) 
		{
			OnBossDeath.Broadcast();
			return;
		}

		if (BossStats.CurrentStunGauge >= BossStats.StunThreshold && !bBossStun) 
		{
			bBossStun = true;
			OnBossStun.Broadcast();
			return;
		}

		OnBossHit.Broadcast();
	}
}

float AT3BossMonster::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	const float ActualDamage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
	const FT3DamageEvent* T3Event = static_cast<const FT3DamageEvent*>(&DamageEvent);

	if (T3Event)
	{
		EHitIntensity Intensity = T3Event->HitIntensity;
	}

	return ActualDamage;
}
