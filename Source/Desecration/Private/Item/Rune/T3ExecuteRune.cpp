#include "Item/Rune/T3ExecuteRune.h"

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

void UT3ExecuteRune::CheckExecution(AActor* HitTarget)
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
	
	switch (Monster->GetMonsterType())
	{
	case ET3MonsterType::Normal:
		
		if (Monster->GetHPPercent() <= ValueByGrade / 100.0f)
		{
			Monster->InstantKill();
		}
		
		UE_LOG(LogTemp, Warning, TEXT("잡몹 처형"));
		return;
		
	case ET3MonsterType::MiddleBoss:
		
		return;
	
	case ET3MonsterType::Boss:
		
		return;
		
	default:
		return;
	}
}
