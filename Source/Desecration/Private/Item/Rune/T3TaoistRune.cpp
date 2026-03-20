#include "Item/Rune/T3TaoistRune.h"

#include "Conditions/MovieSceneScalabilityCondition.h"
#include "Equipment/T3EquipmentTypes.h"
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
	
	if (bIsLegendary)
	{
		OriginalCloneCount = SkillComp->GetCloneCount();
		SkillComp->SetCloneCount(OriginalCloneCount + 1);
	}
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
	
	if (bIsLegendary)
	{
		SkillComp->SetCloneCount(OriginalCloneCount);
		SkillComp->RemoveLastActiveClone();
	}
}

void UT3TaoistRune::SetGrade(ET3RuneGrade InGrade)
{
	switch (InGrade)
	{
	case ET3RuneGrade::Normal:
		
		ValueByGrade = CloneAttackBonusPercentNormal;
		bIsLegendary = false;
		break;
		
	case ET3RuneGrade::Epic:
		
		ValueByGrade = CloneAttackBonusPercentEpic;
		bIsLegendary = false;
		break;
		
	case ET3RuneGrade::Legendary:
		
		ValueByGrade = CloneAttackBonusPercentLegendary;
		bIsLegendary = true;
		break;
	}
}
