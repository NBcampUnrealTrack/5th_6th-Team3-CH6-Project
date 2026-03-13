#include "Item/Rune/T3ValkyrieRune.h"

#include "Player/T3CombatComponent.h"
#include "Player/Valkyrie/T3Valkyrie_SkillComponent.h"

void UT3ValkyrieRune::OnSocketed_Implementation(AT3CharacterBase* OwnerChar)
{
	if (!IsValid(OwnerChar))
	{
		return;
	}
	
	UT3Taoist_SkillComponent* SkillComp = Cast<UT3Taoist_SkillComponent>(OwnerChar->GetCombatComponent()->GetSkillComponent());
	
	if (!IsValid(SkillComp))
	{
		return;
	}
	
	SkillComp->SetPassiveHealBonus(ValueByGrade);
}

void UT3ValkyrieRune::OnUnsocketed_Implementation(AT3CharacterBase* OwnerChar)
{
	if (!IsValid(OwnerChar))
	{
		return;
	}
	
	UT3Valkyrie_SkillComponent* SkillComp = OwnerChar->GetComponentByClass<UT3Valkyrie_SkillComponent>();
	
	if (!IsValid(SkillComp))
	{
		return;
	}
	
	SkillComp->SetPassiveHealBonus(0.1f);
}
