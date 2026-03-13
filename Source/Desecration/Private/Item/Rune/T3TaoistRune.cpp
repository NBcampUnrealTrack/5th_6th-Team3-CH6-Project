#include "Item/Rune/T3TaoistRune.h"

#include "Player/T3CombatComponent.h"
#include "Player/Taoist/T3Taoist_SkillComponent.h"

void UT3TaoistRune::OnSocketed_Implementation(AT3CharacterBase* OwnerChar)
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
	
	SkillComp->SetCloneAttackBonus(ValueByGrade);
}

void UT3TaoistRune::OnUnsocketed_Implementation(AT3CharacterBase* OwnerChar)
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
	
	SkillComp->SetCloneAttackBonus(0.0f);
}
