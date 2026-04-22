// Fill out your copyright notice in the Description page of Project Settings.


#include "Monster/T3BossMonster.h"

#include "Desecration.h"
#include "Engine/DamageEvents.h"
#include "GameFramework/Controller.h"
#include "Player/T3DamageTypes.h"
#include "Blueprint/UserWidget.h"
#include "Components/WidgetComponent.h"
#include "Components/CapsuleComponent.h"

void AT3BossMonster::BeginPlay()
{
	Super::BeginPlay();

	OnBossSpawned.Broadcast();


	if (LockOnWidgetComponent && GetMesh())
	{
		// 이미 부착되어 있더라도 안전하게 다시 부착 (KeepRelativeTransform 사용)
		LockOnWidgetComponent->AttachToComponent(GetMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, TEXT("LockOn_Socket"));

		// 위치 초기화 (소켓 정중앙으로)
		LockOnWidgetComponent->SetRelativeLocation(FVector::ZeroVector);

		UE_LOG(LogTemp, Log, TEXT("[Confirmed] LockOnWidget forced to Socket: %s"), *LockOnWidgetComponent->GetAttachSocketName().ToString());
	}
}

void AT3BossMonster::Damage(float DamageAmount, float StunAmount)
{
	if (DamageAmount > 0 && !bSuperPattern)
	{
		BossStats.CurrentHP -= DamageAmount;
		OnBossDamaged.Broadcast();
		
		UE_LOG(LogItem, Log, TEXT("보스의 남은 체력 : %.1f"), BossStats.CurrentHP);

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

		if (BossStats.CurrentHP / BossStats.MaxHP <= 0.25 )
		{
			OnBoss25per.Broadcast();
			return;
		}
		else if (BossStats.CurrentHP / BossStats.MaxHP <= 0.5)
		{
			OnBoss50per.Broadcast();
			return;
		}
		else if (BossStats.CurrentHP / BossStats.MaxHP <= 0.75)
		{
			OnBoss75per.Broadcast();
			return;
		}

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

AT3BossMonster::AT3BossMonster()
{
	// 1. 컴포넌트 생성
	LockOnWidgetComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("LockOnWidget"));

	// 2. 부착 
	LockOnWidgetComponent->SetupAttachment(GetMesh(), TEXT("LockOn_Socket"));

	// 3. 설정
	LockOnWidgetComponent->SetWidgetSpace(EWidgetSpace::World);
	LockOnWidgetComponent->SetVisibility(false);
	LockOnWidgetComponent->SetRelativeLocation(FVector::ZeroVector); // 소켓 위치로 초기화

	if (UCapsuleComponent* Capsule = GetCapsuleComponent())
	{
		float Radius = Capsule->GetScaledCapsuleRadius();
		float TargetScale = Radius * 0.02f;
		LockOnWidgetComponent->SetWorldScale3D(FVector(TargetScale));
	}
}

void AT3BossMonster::SetLockOnWidgetVisible(bool bVisible)
{
	if (LockOnWidgetComponent)
	{
		LockOnWidgetComponent->SetVisibility(bVisible);
	}
}

float AT3BossMonster::GetHPPercent() const
{
	return BossStats.CurrentHP / BossStats.MaxHP;
}

ET3MonsterType AT3BossMonster::GetMonsterType() const
{
	return MonsterType;
}

void AT3BossMonster::ApplyBonusDamage(float BonusDamage)
{
	Damage(BonusDamage, 0.0f);
}

void AT3BossMonster::SetAnimationSpeedMultiplier(float MoveAnimMultiplier, float AttackAnimMultiplier)
{
}
