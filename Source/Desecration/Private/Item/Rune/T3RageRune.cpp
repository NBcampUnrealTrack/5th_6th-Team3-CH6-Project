#include "Item/Rune/T3RageRune.h"
#include "Equipment/T3PlayerEquipmentComponent.h"

void UT3RageRune::OnSocketed_Implementation(AT3CharacterBase* OwnerChar)
{
	if (!IsValid(OwnerChar))
	{
		return;
	}

	CachedOwner = OwnerChar;

	OwnerChar->OnStatChanged.AddDynamic(this, &UT3RageRune::OnHPChanged);

	if (IsValid(OwnerChar->EquipComp))
	{
		OwnerChar->EquipComp->OnEquipmentStatsChanged.AddDynamic(this, &UT3RageRune::OnEquipmentStatsUpdated);
	}

	ApplyAttackBonus();
}

void UT3RageRune::OnUnsocketed_Implementation(AT3CharacterBase* OwnerChar)
{
	if (!IsValid(OwnerChar))
	{
		return;
	}

	OwnerChar->OnStatChanged.RemoveDynamic(this, &UT3RageRune::OnHPChanged);

	if (IsValid(OwnerChar->EquipComp))
	{
		OwnerChar->EquipComp->OnEquipmentStatsChanged.RemoveDynamic(this, &UT3RageRune::OnEquipmentStatsUpdated);
	}

	OwnerChar->RemoveRuneAttackBonus(this);
	
	CachedOwner = nullptr;
}

void UT3RageRune::OnHPChanged(ET3StatType StatType, float CurrentValue, float MaxValue)
{
	if (StatType != ET3StatType::HP)
	{
		return;
	}

	ApplyAttackBonus();
}

void UT3RageRune::OnEquipmentStatsUpdated(float NewAtk, float NewDef)
{
	ApplyAttackBonus();
}

void UT3RageRune::ApplyAttackBonus()
{
	AT3CharacterBase* Owner = CachedOwner.Get();
	if (!IsValid(Owner) || !IsValid(Owner->EquipComp))
	{
		return;
	}

	float MaxHP = Owner->GetMaxHP();
	float HPPercent = (MaxHP > 0.f) ? FMath::Clamp((Owner->GetCurrentHP() / MaxHP), 0.5, 1) : 1.f;
	float Bonus = (FMath::RoundToFloat(Owner->EquipComp->GetCurrentAttackPower() * (1.f - HPPercent) * AttackBonusMultiplier) * 10.0f) / 10.0f;

	Owner->SetRuneAttackBonus(this, Bonus);
}
