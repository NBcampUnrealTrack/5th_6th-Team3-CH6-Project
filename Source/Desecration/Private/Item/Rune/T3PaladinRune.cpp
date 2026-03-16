#include "Item/Rune/T3PaladinRune.h"

#include "Player/T3CombatComponent.h"

void UT3PaladinRune::OnSocketed_Implementation(AT3CharacterBase* OwnerChar)
{
	if (!IsValid(OwnerChar))
	{
		return;
	}
	
	OriginalHolyGaugeChargeAmount = OwnerChar->GetCombatComponent()->GetHolyGaugeChargeAmount();
	
	OwnerChar->GetCombatComponent()->SetHolyGaugeChargeAmount(OriginalHolyGaugeChargeAmount * ValueByGrade);
}

void UT3PaladinRune::OnUnsocketed_Implementation(AT3CharacterBase* OwnerChar)
{
	if (!IsValid(OwnerChar))
	{
		return;
	}
	
	OwnerChar->GetCombatComponent()->SetHolyGaugeChargeAmount(OriginalHolyGaugeChargeAmount);
}