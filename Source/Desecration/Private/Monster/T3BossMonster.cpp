// Fill out your copyright notice in the Description page of Project Settings.


#include "Monster/T3BossMonster.h"
#include "Engine/DamageEvents.h"
#include "GameFramework/Controller.h"

void AT3BossMonster::Damage(float DamageAmount, float StunAmount)
{
	if (DamageAmount > 0) 
	{
		BossStats.CurrentHP -= DamageAmount;

		if (!bBossStun) 
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
