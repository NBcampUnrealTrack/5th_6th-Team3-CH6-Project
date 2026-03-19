#include "Item/Rune/T3BerserkRune.h"

#include "Equipment/T3PlayerEquipmentComponent.h"
#include "Player/T3CombatComponent.h"

void UT3BerserkRune::OnSocketed_Implementation(AT3CharacterBase* OwnerChar)
{
	if (!IsValid(OwnerChar) || !IsValid(OwnerChar->GetCombatComponent()))
	{
		return;
	}
	
	CachedOwner = OwnerChar;
	
	OwnerChar->GetCombatComponent()->OnTakeDamage.AddDynamic(this, &UT3BerserkRune::Activate);
}

void UT3BerserkRune::OnUnsocketed_Implementation(AT3CharacterBase* OwnerChar)
{
	if (!IsValid(OwnerChar) || !IsValid(OwnerChar->GetCombatComponent()))
	{
		return;
	}
	
	if (OwnerChar->GetCombatComponent()->OnTakeDamage.IsBound())
	{
		OwnerChar->GetCombatComponent()->OnTakeDamage.RemoveDynamic(this, &UT3BerserkRune::Activate);
	}
	
	OwnerChar->GetWorldTimerManager().ClearTimer(ActiveTimerHandle);
	
	OwnerChar->RemoveRuneAttackBonus(this);
	
	CachedOwner = nullptr;
}

void UT3BerserkRune::SetGrade(ET3RuneGrade InGrade)
{
	switch (InGrade)
	{
	case ET3RuneGrade::Normal:
		
		ValueByGrade = OnHitAttackBonusPercentNormal;
		break;
	
	case ET3RuneGrade::Epic:
		
		ValueByGrade = OnHitAttackBonusPercentEpic;
		break;
		
	case ET3RuneGrade::Legendary:
		
		ValueByGrade = OnHitAttackBonusPercentLegendary;
		break;
		
	default:
		break;
	}
}

void UT3BerserkRune::Activate()
{
	AT3CharacterBase* Owner = CachedOwner.Get();

	if (!IsValid(Owner))
	{
		return;
	}
	
	if (bIsCooldown)
	{
		return;
	}
	else
	{
		bIsCooldown = true;
		
		float Bonus = FMath::RoundToFloat(Owner->EquipComp->GetCurrentAttackPower() * (ValueByGrade / 100.0f) * 10.0f) / 10.0f;
		
		Owner->SetRuneAttackBonus(this, Bonus);
		
		Owner->GetWorldTimerManager().SetTimer(
			ActiveTimerHandle,
			this,
			&UT3BerserkRune::Deactivate,
			ActiveDuration,
			false);
		
		Owner->GetWorldTimerManager().SetTimer(
			CooldownTimerHandle,
			this,
			&UT3BerserkRune::OnCooldownFinished,
			Cooldown,
			false);
	}
}

void UT3BerserkRune::Deactivate()
{
	AT3CharacterBase* Owner = CachedOwner.Get();

	if (!IsValid(Owner))
	{
		return;
	}
	
	Owner->SetRuneAttackBonus(this, 0.0);
	
	UE_LOG(LogTemp, Warning, TEXT("광폭 룬 효과 끝남"));
}

void UT3BerserkRune::OnCooldownFinished()
{
	bIsCooldown = false;
	
	UE_LOG(LogTemp, Warning, TEXT("광폭 룬 쿨타임 돌았음"));
}

bool UT3BerserkRune::CanUnsocket() const
{
	return !bIsCooldown;
}
