#include "Item/Rune/T3ValkyrieRune.h"

#include "Equipment/T3EquipmentTypes.h"
#include "Player/T3CombatComponent.h"
#include "Player/Valkyrie/T3Valkyrie_SkillComponent.h"

void UT3ValkyrieRune::OnSocketed_Implementation(AT3CharacterBase* OwnerChar)
{
	if (!IsValid(OwnerChar))
	{
		return;
	}
	
	UT3Valkyrie_SkillComponent* SkillComp = Cast<UT3Valkyrie_SkillComponent>(OwnerChar->GetCombatComponent()->GetSkillComponent());
	
	if (!IsValid(SkillComp))
	{
		return;
	}
	
	SkillComp->SetPassiveHealBonus(ValueByGrade);
	
	if (bIsLegendary)
	{
		OriginTriggerAttackCount = SkillComp->GetTriggerAttackCount();
		
		SkillComp->SetTriggerAttackCount(OriginTriggerAttackCount - 1);
	}
}

void UT3ValkyrieRune::OnUnsocketed_Implementation(AT3CharacterBase* OwnerChar)
{
	if (!IsValid(OwnerChar))
	{
		return;
	}
	
	UT3Valkyrie_SkillComponent* SkillComp = Cast<UT3Valkyrie_SkillComponent>(OwnerChar->GetCombatComponent()->GetSkillComponent());
	
	if (!IsValid(SkillComp))
	{
		return;
	}
	
	SkillComp->SetPassiveHealBonus(0.0f);
	
	if (bIsLegendary)
	{
		SkillComp->SetTriggerAttackCount(OriginTriggerAttackCount);
	}
}

void UT3ValkyrieRune::SetGrade(ET3RuneGrade InGrade)
{
	switch (InGrade)
	{
	case ET3RuneGrade::Normal:

		ValueByGrade = MaxHPRecoveryPercentNormal;
		bIsLegendary = false;
		break;
		
	case ET3RuneGrade::Epic:
		
		ValueByGrade = MaxHPRecoveryPercentEpic;
		bIsLegendary = false;
		break;
		
	case ET3RuneGrade::Legendary:
		
		ValueByGrade = MaxHPRecoveryPercentLegendary;
		bIsLegendary = true;
		break;
		
	default:
		break;
	}
}
