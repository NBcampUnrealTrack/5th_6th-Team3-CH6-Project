#include "Monster/T3MonsterBase.h"

AT3MonsterBase::AT3MonsterBase()
{
	PrimaryActorTick.bCanEverTick = true;
	CurrentHP = 0.0f;
	bIsDead = false;
}

void AT3MonsterBase::BeginPlay()
{
	Super::BeginPlay();

	if (MonsterDataTable)
	{
		static const FString ContextString(TEXT("MonsterData"));
		if (FMonsterStats* Stats = MonsterDataTable->FindRow<FMonsterStats>(MonsterRowName, ContextString))
		{
			CurrentHP = Stats->MaxHP;
		}
	}
}

float AT3MonsterBase::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	const float ActualDamage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
	CurrentHP -= ActualDamage;

	if (CurrentHP <= 0.0f && !bIsDead)
	{
		bIsDead = true;
		OnDeath();
	}

	return ActualDamage;
}

void AT3MonsterBase::OnDeath()
{
	// 필요하면 추가 처리
	Destroy();
}

void AT3MonsterBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	// Tick 로직
}

void AT3MonsterBase::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	// 입력 바인딩
}