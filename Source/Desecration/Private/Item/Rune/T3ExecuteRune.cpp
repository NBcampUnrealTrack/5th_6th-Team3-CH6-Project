#include "Item/Rune/T3ExecuteRune.h"

#include "Desecration.h"
#include "Equipment/T3EquipmentTypes.h"
#include "Monster/Interface/T3Monster.h"

void UT3ExecuteRune::OnSocketed_Implementation(AT3CharacterBase* OwnerChar)
{
	if (!IsValid(OwnerChar))
	{
		return;
	}
	
	OwnerChar->OnDamageDealt.AddDynamic(this, &UT3ExecuteRune::CheckExecution);
}

void UT3ExecuteRune::OnUnsocketed_Implementation(AT3CharacterBase* OwnerChar)
{
	if (!IsValid(OwnerChar))
	{
		return;
	}
	
	OwnerChar->OnDamageDealt.RemoveDynamic(this, &UT3ExecuteRune::CheckExecution);
}

void UT3ExecuteRune::SetGrade(ET3RuneGrade InGrade)
{
	switch (InGrade)
	{
	case ET3RuneGrade::Normal:
		
		ValueByGrade = HPThresholdPercentNormal;
		BossAttackBonusPercentByGrade = BossAttackBonusPercentNormal;
		
		break;
		
	case ET3RuneGrade::Epic:
		
		ValueByGrade = HPThresholdPercentEpic;
		BossAttackBonusPercentByGrade = BossAttackBonusPercentEpic;
		
		break;
		
	case ET3RuneGrade::Legendary:
		
		ValueByGrade = HPThresholdPercentLegendary;
		BossAttackBonusPercentByGrade = BossAttackBonusPercentLegendary;
		
		break;
	}
}

void UT3ExecuteRune::CheckExecution(AActor* HitTarget, float DamageDealt)
{
	if (!IsValid(HitTarget))
	{
		return;
	}
	
	IT3Monster* Monster = Cast<IT3Monster>(HitTarget);
	
	if (!Monster)
	{
		return;
	}
	
	if (Monster->GetMonsterType() == ET3MonsterType::Normal)
	{
		if (Monster->GetHPPercent() <= ValueByGrade / 100.0f)
		{
			Monster->ApplyBonusDamage(99999.0f);
				
			UE_LOG(LogItem, Warning, TEXT("잡몹 처형"));
		}
	}
	else if (Monster->GetMonsterType() == ET3MonsterType::MiddleBoss
		|| Monster->GetMonsterType() == ET3MonsterType::Boss)
	{
		if (Monster->GetHPPercent() <= ValueByGrade / 100.0f)
		{
			float Bonus = DamageDealt * BossAttackBonusPercentByGrade / 100.0f;
			
			Monster->ApplyBonusDamage(Bonus);
			
			UE_LOG(LogItem, Log, TEXT("추가 데미지 : %.1f"), Bonus);
		}
		UE_LOG(LogItem, Log, TEXT("보스 호출 테스트"));
	}
}
