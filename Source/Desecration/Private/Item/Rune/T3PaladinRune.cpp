#include "Item/Rune/T3PaladinRune.h"

#include "Equipment/T3EquipmentTypes.h"
#include "Player/T3CombatComponent.h"
#include "Player/Paladin/T3Paladin_SkillComponent.h"

void UT3PaladinRune::OnSocketed_Implementation(AT3CharacterBase* OwnerChar)
{
	if (!IsValid(OwnerChar))
	{
		return;
	}
	
	UT3CombatComponent* Combat = OwnerChar->GetCombatComponent();
	
	OriginalHolyGaugeChargeAmount = Combat->GetHolyGaugeChargeAmount();
	
	Combat->SetHolyGaugeChargeAmount(OriginalHolyGaugeChargeAmount * (1.0f + ValueByGrade / 100.f));
	
	if (bIsLegendary)
	{
		UT3Paladin_SkillComponent* PaladinSkill = Cast<UT3Paladin_SkillComponent>(Combat->GetSkillComponent());
		
		if (IsValid(PaladinSkill))
		{
			OriginalHolyModeAttackSpeedMultiplier = PaladinSkill->GetHolyModeAttackSpeedMultiplier();
			
			PaladinSkill->SetHolyModeAttackSpeedMultiplier(1 + LegendaryAttackSpeedBonusPercent / 100.0f);
		}
	}
}

void UT3PaladinRune::OnUnsocketed_Implementation(AT3CharacterBase* OwnerChar)
{
	if (!IsValid(OwnerChar))
	{
		return;
	}
	
	OwnerChar->GetCombatComponent()->SetHolyGaugeChargeAmount(OriginalHolyGaugeChargeAmount);
	
	if (bIsLegendary)
	{
		UT3Paladin_SkillComponent* PaladinSkill = Cast<UT3Paladin_SkillComponent>(
		OwnerChar->GetCombatComponent()->GetSkillComponent());
	
		if (IsValid(PaladinSkill))
		{
			PaladinSkill->SetHolyModeAttackSpeedMultiplier(OriginalHolyModeAttackSpeedMultiplier);
		}
	}
}

void UT3PaladinRune::SetGrade(ET3RuneGrade InGrade)
{
	switch (InGrade)
	{
	case ET3RuneGrade::Normal:
		
		ValueByGrade = HolyGaugeChargeBonusPercentNormal;
		bIsLegendary = false;
		break;
		
	case ET3RuneGrade::Epic:
		
		ValueByGrade = HolyGaugeChargeBonusPercentEpic;
		bIsLegendary = false;
		break;
		
	case ET3RuneGrade::Legendary:
		
		ValueByGrade = HolyGaugeChargeBonusPercentLegendary;
		bIsLegendary = true;
		break;
		
	default:
		break;
	}
}
