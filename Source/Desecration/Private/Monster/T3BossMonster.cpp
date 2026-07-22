// Fill out your copyright notice in the Description page of Project Settings.


#include "Monster/T3BossMonster.h"

#include "Desecration.h"
#include "Engine/DamageEvents.h"
#include "GameFramework/Controller.h"
#include "Player/T3DamageTypes.h"
#include "Blueprint/UserWidget.h"
#include "Components/WidgetComponent.h"
#include "Components/CapsuleComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "Kismet/GameplayStatics.h"

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

void AT3BossMonster::SetAnimationSpeedMultiplier(
	float MoveSpeedMultiplier,
	float /*MoveAnimMultiplier*/,
	float AttackAnimMultiplier)
{
	// BossStats는 의미상 이동 속도/공격 속도를 보관 — MoveAnim(애님 PlayRate)은 보스 스탯 영역 밖이므로 미사용
	BossStats.MoveSpeed = MoveSpeedMultiplier;
	BossStats.AttackSpeed = AttackAnimMultiplier;
	OnBossActionSpeedChanged.Broadcast();
}

// ============================================================
// 독 시스템 (IT3Poisonable 구현)
// ============================================================

void AT3BossMonster::ApplyPoisonStack_Implementation(int32 Stacks)
{
	// 독 활성화 중이거나 사망 시 스택 누적 없음
	if (BossStats.CurrentHP <= 0.f || bIsPoisoned)
	{
		UE_LOG(LogItem, Verbose, TEXT("[독] 보스 %s — 스택 무시 (HP:%.0f, 독활성:%d)"),
			*BossName.ToString(), BossStats.CurrentHP, bIsPoisoned);
		return;
	}

	CurrentPoisonStack = FMath::Min(CurrentPoisonStack + Stacks, MaxPoisonStack);

	UE_LOG(LogItem, Log, TEXT("[독] 보스 %s 스택 +%d → %d/%d"),
		*BossName.ToString(), Stacks, CurrentPoisonStack, MaxPoisonStack);

	if (CurrentPoisonStack >= MaxPoisonStack)
	{
		ActivatePoison();
		CurrentPoisonStack = 0;
	}
}

bool AT3BossMonster::IsPoisoned_Implementation() const
{
	return bIsPoisoned;
}

void AT3BossMonster::ActivatePoison()
{
	bIsPoisoned = true;
	PoisonRemainingTime = PoisonDuration;

	UE_LOG(LogItem, Warning, TEXT("[독] ★ 보스 %s 독 활성화! 지속 %.0f초, 초당 %.0f%% 데미지"),
		*BossName.ToString(), PoisonDuration, PoisonDamagePercent * 100.f);

	if (PoisonFX.ActivateEffect)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			GetWorld(), PoisonFX.ActivateEffect, GetActorLocation());
	}
	if (PoisonFX.ActivateSound)
	{
		UGameplayStatics::SpawnSoundAtLocation(this, PoisonFX.ActivateSound, GetActorLocation());
	}

	GetWorld()->GetTimerManager().SetTimer(
		PoisonTickTimerHandle,
		this,
		&AT3BossMonster::PoisonTick,
		PoisonTickInterval,
		true,
		PoisonTickInterval
	);
}

void AT3BossMonster::DeactivatePoison()
{
	bIsPoisoned = false;
	GetWorld()->GetTimerManager().ClearTimer(PoisonTickTimerHandle);

	UE_LOG(LogItem, Log, TEXT("[독] 보스 %s 독 해제"), *BossName.ToString());
}

void AT3BossMonster::PoisonTick()
{
	if (BossStats.CurrentHP <= 0.f)
	{
		DeactivatePoison();
		return;
	}

	// 독 데미지는 슈퍼아머 패턴 무시 (상태이상이므로 항상 적용)
	const float PoisonDamage = BossStats.MaxHP * PoisonDamagePercent;
	BossStats.CurrentHP = FMath::Max(0.f, BossStats.CurrentHP - PoisonDamage);
	OnBossDamaged.Broadcast();

	if (PoisonFX.TickEffect)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			GetWorld(), PoisonFX.TickEffect, GetActorLocation());
	}
	if (PoisonFX.TickSound)
	{
		UGameplayStatics::SpawnSoundAtLocation(this, PoisonFX.TickSound, GetActorLocation());
	}

	UE_LOG(LogItem, Log, TEXT("[독] 보스 %s 독 데미지 %.1f (HP %.0f/%.0f) 남은시간 %.0f초"),
		*BossName.ToString(), PoisonDamage, BossStats.CurrentHP, BossStats.MaxHP, PoisonRemainingTime);

	if (BossStats.CurrentHP <= 0.f)
	{
		OnBossDeath.Broadcast();
		DeactivatePoison();
		return;
	}

	PoisonRemainingTime -= PoisonTickInterval;
	if (PoisonRemainingTime <= 0.f)
	{
		DeactivatePoison();
	}
}
