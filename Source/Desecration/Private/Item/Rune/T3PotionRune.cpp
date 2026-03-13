#include "Item/Rune/T3PotionRune.h"

void UT3PotionRune::OnSocketed_Implementation(AT3CharacterBase* OwnerChar)
{
	if (!IsValid(OwnerChar))
	{
		return;
	}
	
	OwnerChar->SetPotionUsePlayRate(ValueByGrade);
}

void UT3PotionRune::OnUnsocketed_Implementation(AT3CharacterBase* OwnerChar)
{
	if (!IsValid(OwnerChar))
	{
		return;
	}
	
	OwnerChar->SetPotionUsePlayRate(1.0f);
}
