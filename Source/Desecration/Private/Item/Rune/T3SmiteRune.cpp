#include "Item/Rune/T3SmiteRune.h"

#include "Equipment/T3EquipmentTypes.h"
#include "Player/T3CombatComponent.h"
#include "NiagaraFunctionLibrary.h"

void UT3SmiteRune::OnSocketed_Implementation(AT3CharacterBase* OwnerChar)
{
	if (!IsValid(OwnerChar))
	{
		return;
	}

	CachedOwner = OwnerChar;

	OwnerChar->SetSmiteCounter(0);

	OwnerChar->SetSmiteMultiplier(1 + (ValueByGrade / 100.f));

	OwnerChar->SetSmiteThreshold(TriggerAttackCount);

	SmiteTriggeredHandle = OwnerChar->OnSmiteTriggered.AddUObject(this, &UT3SmiteRune::OnSmiteTriggeredHandler);
}

void UT3SmiteRune::OnUnsocketed_Implementation(AT3CharacterBase* OwnerChar)
{
	if (!IsValid(OwnerChar))
	{
		return;
	}

	OwnerChar->OnSmiteTriggered.Remove(SmiteTriggeredHandle);

	SmiteTriggeredHandle.Reset();

	CachedOwner = nullptr;

	OwnerChar->SetSmiteMultiplier(1.0f);

	OwnerChar->SetSmiteThreshold(0);
	OwnerChar->SetSmiteCounter(0);
}

void UT3SmiteRune::OnSmiteTriggeredHandler(FVector HitLocation)
{
	AT3CharacterBase* Owner = CachedOwner.Get();

	if (!IsValid(Owner) || !IsValid(WeaponHitEffect))
	{
		return;
	}

	UWorld* World = Owner->GetWorld();

	if (!IsValid(World))
	{
		return;
	}

	UNiagaraFunctionLibrary::SpawnSystemAtLocation(
		World,
		WeaponHitEffect,
		HitLocation);
}

void UT3SmiteRune::SetGrade(ET3RuneGrade InGrade)
{
	switch (InGrade)
	{
	case ET3RuneGrade::Normal:
		
		ValueByGrade = AttackBonusDamagePercentNormal;
		break;
		
	case ET3RuneGrade::Epic:
		
		ValueByGrade = AttackBonusDamagePercentEpic;
		break;
		
	case ET3RuneGrade::Legendary:
		
		ValueByGrade = AttackBonusDamagePercentLegendary;
		break;
		
	default:
		break;
	}
}
