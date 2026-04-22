#include "Equipment/Accessory/T3AngelAccessoryEffect.h"

#include "DrawDebugHelpers.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Monster/Interface/T3Monster.h"
#include "Player/T3CharacterBase.h"

void UT3AngelAccessoryEffect::OnEquipped_Implementation(AT3CharacterBase* OwnerChar)
{
	CachedOwner = OwnerChar;

	OwnerChar->GetWorldTimerManager().SetTimer(
		SlowTimerHandle,
		this,
		&UT3AngelAccessoryEffect::ApplySlowAura,
		SlowInterval,
		true);
}

void UT3AngelAccessoryEffect::ApplySlowAura()
{
	if (!CachedOwner.IsValid())
	{
		return;
	}

	for (int32 i = 0; i < SlowedMonsters.Num(); ++i)
	{
		if (SlowedMonsters[i].IsValid())
		{
			if (IT3Monster* Monster = Cast<IT3Monster>(SlowedMonsters[i].Get()))
			{
				Monster->SetAnimationSpeedMultiplier(1.f, 1.f);
			}

			if (ACharacter* MonsterChar = Cast<ACharacter>(SlowedMonsters[i].Get()))
			{
				MonsterChar->GetCharacterMovement()->MaxWalkSpeed = OriginalMaxWalkSpeeds[i];
			}
		}
	}

	SlowedMonsters.Empty();

	OriginalMaxWalkSpeeds.Empty();

	AT3CharacterBase* Owner = CachedOwner.Get();

	TArray<AActor*> Overlapped;

	TArray<TEnumAsByte<EObjectTypeQuery>> ObjTypes;

	ObjTypes.Add(UEngineTypes::ConvertToObjectType(ECC_Pawn));

	UKismetSystemLibrary::SphereOverlapActors(
		Owner,
		Owner->GetActorLocation(),
		SlowRadius,
		ObjTypes,
		nullptr,
		{Owner},
		Overlapped);

	if (bShowDebugRadius)
	{
		DrawDebugSphere(Owner->GetWorld(), Owner->GetActorLocation(), SlowRadius, 16, FColor::Cyan, false, SlowInterval);
	}

	for (AActor* Actor : Overlapped)
	{
		if (IT3Monster* Monster = Cast<IT3Monster>(Actor))
		{
			Monster->SetAnimationSpeedMultiplier(MoveAnimSlowAmount, AttackAnimSlowAmount);

			float OriginalSpeed = 0.f;

			if (ACharacter* MonsterChar = Cast<ACharacter>(Actor))
			{
				OriginalSpeed = MonsterChar->GetCharacterMovement()->MaxWalkSpeed;

				MonsterChar->GetCharacterMovement()->MaxWalkSpeed *= MoveSpeedSlowAmount;
			}

			SlowedMonsters.Add(Actor);

			OriginalMaxWalkSpeeds.Add(OriginalSpeed);
		}
	}
}

void UT3AngelAccessoryEffect::OnUnequipped_Implementation(AT3CharacterBase* OwnerChar)
{
	OwnerChar->GetWorldTimerManager().ClearTimer(SlowTimerHandle);

	for (int32 i = 0; i < SlowedMonsters.Num(); ++i)
	{
		if (SlowedMonsters[i].IsValid())
		{
			if (IT3Monster* Monster = Cast<IT3Monster>(SlowedMonsters[i].Get()))
			{
				Monster->SetAnimationSpeedMultiplier(1.f, 1.f);
			}

			if (ACharacter* MonsterChar = Cast<ACharacter>(SlowedMonsters[i].Get()))
			{
				MonsterChar->GetCharacterMovement()->MaxWalkSpeed = OriginalMaxWalkSpeeds[i];
			}
		}
	}

	SlowedMonsters.Empty();

	OriginalMaxWalkSpeeds.Empty();
}
