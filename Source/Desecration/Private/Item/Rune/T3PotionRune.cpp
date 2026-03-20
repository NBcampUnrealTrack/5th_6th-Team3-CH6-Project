#include "Item/Rune/T3PotionRune.h"

#include "Equipment/T3EquipmentTypes.h"

void UT3PotionRune::OnSocketed_Implementation(AT3CharacterBase* OwnerChar)
{
	if (!IsValid(OwnerChar))
	{
		return;
	}
	
	OwnerChar->SetPotionUsePlayRate(1.0f + (ValueByGrade / 100.0f));
}

void UT3PotionRune::OnUnsocketed_Implementation(AT3CharacterBase* OwnerChar)
{
	if (!IsValid(OwnerChar))
	{
		return;
	}
	
	OwnerChar->SetPotionUsePlayRate(1.0f);
}

void UT3PotionRune::SetGrade(ET3RuneGrade InGrade)
{
	switch (InGrade)
	{
	case ET3RuneGrade::Normal:
		
		ValueByGrade = PotionUseAnimSpeedBonusPercentNormal;
		break;
		
	case ET3RuneGrade::Epic:
		
		ValueByGrade = PotionUseAnimSpeedBonusPercentEpic;
		break;
		
	case ET3RuneGrade::Legendary:
		
		ValueByGrade = PotionUseAnimSpeedBonusPercentLegendary;
		break;
		
	default:
		break;
	}
}
