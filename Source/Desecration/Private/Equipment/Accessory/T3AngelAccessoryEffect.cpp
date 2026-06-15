#include "Equipment/Accessory/T3AngelAccessoryEffect.h"

#include "Components/SphereComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Monster/Interface/T3Monster.h"
#include "Player/T3CharacterBase.h"

void UT3AngelAccessoryEffect::OnEquipped_Implementation(AT3CharacterBase* OwnerChar)
{
	CachedOwner = OwnerChar;

	AuraSphere = NewObject<USphereComponent>(OwnerChar);
	AuraSphere->InitSphereRadius(SlowRadius);
	AuraSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	AuraSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	AuraSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	AuraSphere->SetGenerateOverlapEvents(true);
	AuraSphere->SetHiddenInGame(!bShowDebugRadius);
	AuraSphere->SetupAttachment(OwnerChar->GetRootComponent());
	AuraSphere->RegisterComponent();

	AuraSphere->OnComponentBeginOverlap.AddDynamic(this, &UT3AngelAccessoryEffect::OnOverlapBegin);
	AuraSphere->OnComponentEndOverlap.AddDynamic(this, &UT3AngelAccessoryEffect::OnOverlapEnd);
}

void UT3AngelAccessoryEffect::OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	IT3Monster* Monster = Cast<IT3Monster>(OtherActor);

	if (!Monster)
	{
		return;
	}

	// IT3Monster 인터페이스가 3-arg(MoveSpeed, MoveAnim, AttackAnim)로 변경됨.
	// MoveSpeed는 아래 MaxWalkSpeed 직접 곱셈 로직이 담당하므로 1.f(노옵) 전달.
	Monster->SetAnimationSpeedMultiplier(1.f, MoveAnimSlowAmount, AttackAnimSlowAmount);

	float OriginalSpeed = 0.f;

	if (ACharacter* MonsterChar = Cast<ACharacter>(OtherActor))
	{
		OriginalSpeed = MonsterChar->GetCharacterMovement()->MaxWalkSpeed;

		MonsterChar->GetCharacterMovement()->MaxWalkSpeed *= MoveSpeedSlowAmount;
	}

	SlowedMonsters.Add(OtherActor);

	OriginalMaxWalkSpeeds.Add(OriginalSpeed);
}

void UT3AngelAccessoryEffect::OnOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	int32 Index = SlowedMonsters.IndexOfByPredicate([OtherActor](const TWeakObjectPtr<AActor>& Ptr)
	{
		return Ptr.Get() == OtherActor;
	});

	if (Index == INDEX_NONE)
	{
		return;
	}

	RestoreMonster(Index);
}

void UT3AngelAccessoryEffect::RestoreMonster(int32 Index)
{
	if (SlowedMonsters[Index].IsValid())
	{
		if (IT3Monster* Monster = Cast<IT3Monster>(SlowedMonsters[Index].Get()))
		{
			Monster->SetAnimationSpeedMultiplier(1.f, 1.f, 1.f);
		}

		if (ACharacter* MonsterChar = Cast<ACharacter>(SlowedMonsters[Index].Get()))
		{
			MonsterChar->GetCharacterMovement()->MaxWalkSpeed = OriginalMaxWalkSpeeds[Index];
		}
	}

	SlowedMonsters.RemoveAt(Index);

	OriginalMaxWalkSpeeds.RemoveAt(Index);
}

void UT3AngelAccessoryEffect::OnUnequipped_Implementation(AT3CharacterBase* OwnerChar)
{
	if (IsValid(AuraSphere))
	{
		AuraSphere->DestroyComponent();

		AuraSphere = nullptr;
	}

	for (int32 i = SlowedMonsters.Num() - 1; i >= 0; --i)
	{
		RestoreMonster(i);
	}
}
