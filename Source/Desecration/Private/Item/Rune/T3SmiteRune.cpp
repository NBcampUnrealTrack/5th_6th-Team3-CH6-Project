#include "Item/Rune/T3SmiteRune.h"

#include "Equipment/T3EquipmentTypes.h"

void UT3SmiteRune::OnSocketed_Implementation(AT3CharacterBase* OwnerChar)
{
	if (!IsValid(OwnerChar))
	{
		return;
	}
	
	OwnerChar->SetSmiteCounter(0);
	
	OwnerChar->SetSmiteMultiplier(1 + (ValueByGrade / 100.f));
	
	OwnerChar->SetSmiteThreshold(TriggerAttackCount);
}

void UT3SmiteRune::OnUnsocketed_Implementation(AT3CharacterBase* OwnerChar)
{
	if (!IsValid(OwnerChar))
	{
		return;
	}
	
	OwnerChar->SetSmiteMultiplier(1.0f);
	
	OwnerChar->SetSmiteThreshold(0);
	OwnerChar->SetSmiteCounter(0);
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
