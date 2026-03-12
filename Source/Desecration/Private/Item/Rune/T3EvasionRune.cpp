#include "Item/Rune/T3EvasionRune.h"

void UT3EvasionRune::OnSocketed_Implementation(AT3CharacterBase* OwnerChar)
{
	if (!IsValid(OwnerChar))
	{
		return;
	}
	
	OwnerChar->SetEvasionPlayRate(NormalValue);
}

void UT3EvasionRune::OnUnsocketed_Implementation(AT3CharacterBase* OwnerChar)
{
	if (!IsValid(OwnerChar))
	{
		return;
	}
	
	OwnerChar->SetEvasionPlayRate(1.0f);
}
