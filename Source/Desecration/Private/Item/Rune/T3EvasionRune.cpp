#include "Item/Rune/T3EvasionRune.h"

#include "Equipment/T3EquipmentTypes.h"

void UT3EvasionRune::OnSocketed_Implementation(AT3CharacterBase* OwnerChar)
{
	if (!IsValid(OwnerChar))
	{
		return;
	}
	
	OwnerChar->SetEvasionPlayRate(1.0f + (ValueByGrade / 100.0f));
}

void UT3EvasionRune::OnUnsocketed_Implementation(AT3CharacterBase* OwnerChar)
{
	if (!IsValid(OwnerChar))
	{
		return;
	}
	
	OwnerChar->SetEvasionPlayRate(1.0f);
}

void UT3EvasionRune::SetGrade(ET3RuneGrade InGrade)
{
	switch (InGrade)
	{
	case ET3RuneGrade::Normal:
		
		ValueByGrade = RollAnimSpeedBonusPercentNormal;
		break;
		
	case ET3RuneGrade::Epic:
		
		ValueByGrade = RollAnimSpeedBonusPercentEpic;
		break;
		
	case ET3RuneGrade::Legendary:
		
		ValueByGrade = RollAnimSpeedBonusPercentLegendary;
		break;
		
	default:
		break;
	}
}