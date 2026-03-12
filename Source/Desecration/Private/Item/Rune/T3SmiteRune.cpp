#include "Item/Rune/T3SmiteRune.h"

void UT3SmiteRune::OnSocketed_Implementation(AT3CharacterBase* OwnerChar)
{
	if (!IsValid(OwnerChar))
	{
		return;
	}
	
	OwnerChar->SetSmiteCounter(0);
	
	OwnerChar->SetSmiteMultiplier(ValueByGrade);
	
	OwnerChar->SetSmiteThreshold(3);
}

void UT3SmiteRune::OnUnsocketed_Implementation(AT3CharacterBase* OwnerChar)
{
	if (!IsValid(OwnerChar))
	{
		return;
	}
	
	OwnerChar->SetSmiteMultiplier(1.0f);
	
	OwnerChar->SetSmiteThreshold(0);
}
