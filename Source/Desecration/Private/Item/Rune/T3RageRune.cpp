#include "Item/Rune/T3RageRune.h"
#include "Equipment/T3PlayerEquipmentComponent.h"
#include "NiagaraComponent.h"

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

	ActiveEffect = PlayTriggerEffect(OwnerChar, NAME_None, false);

	ApplyAttackBonus();
}

void UT3RageRune::OnUnsocketed_Implementation(AT3CharacterBase* OwnerChar)
{
	if (!IsValid(OwnerChar))SS
	{
		return;
	}

	OwnerChar->OnStatChanged.RemoveDynamic(this, &UT3RageRune::OnHPChanged);

	if (IsValid(OwnerChar->EquipComp))
	{
		OwnerChar->EquipComp->OnEquipmentStatsChanged.RemoveDynamic(this, &UT3RageRune::OnEquipmentStatsUpdated);
	}

	OwnerChar->RemoveRuneAttackBonus(this);

	if (IsValid(ActiveEffect))
	{
		ActiveEffect->DeactivateImmediate();
		ActiveEffect = nullptr;
	}

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

void UT3RageRune::OnEquipmentStatsUpdated(float NewAtk, float NewDef, float WeaponLevel)
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
	float HPPercent = (MaxHP > 0.f) ? FMath::Clamp((Owner->GetCurrentHP() / MaxHP), 0.5f, 1.f) : 1.f;
	float Bonus = FMath::RoundToFloat((Owner->EquipComp->GetCurrentAttackPower() * (1.f - HPPercent) * (ValueByGrade / 10) * 10.0f)) / 10.0f;

	Owner->SetRuneAttackBonus(this, Bonus);

	if (IsValid(ActiveEffect))
	{
		float RageIntensity = FMath::Clamp((1.0f - HPPercent) * 2.0f, 0.0f, 1.0f);

		ActiveEffect->SetVariableFloat(TEXT("RageIntensity"), RageIntensity);
	}
}

void UT3RageRune::SetGrade(ET3RuneGrade InGrade)
{
	switch (InGrade)
	{
	case ET3RuneGrade::Normal:
		
		ValueByGrade = AttackBonusPerHPTenPercentNormal;
		break;
		
	case ET3RuneGrade::Epic:
		
		ValueByGrade = AttackBonusPerHPTenPercentEpic;
		break;
		
	case ET3RuneGrade::Legendary:
		
		ValueByGrade = AttackBonusPerHPTenPercentLegendary;
		break;
		
	default:
		break;
	}
}