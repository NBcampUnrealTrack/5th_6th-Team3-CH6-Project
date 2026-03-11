#include "Item/Rune/T3RegenerationRune.h"

void UT3RegenerationRune::OnSocketed_Implementation(AT3CharacterBase* OwnerChar)
{
	if (!IsValid(OwnerChar))
	{
		return;
	}
	
	CachedOwner = OwnerChar;
	
	OwnerChar->OnStatChanged.AddDynamic(this, &UT3RegenerationRune::RegenerationHP);
	
	float CurrentHP = OwnerChar->GetCurrentHP();
	float MaxHP = OwnerChar->GetMaxHP();

	if (CurrentHP / MaxHP < 0.3f)
	{
		FTimerDelegate TimerDelegate;
		TimerDelegate.BindUObject(OwnerChar, &AT3CharacterBase::RestoreHP, HealAmount);
		
		OwnerChar->GetWorldTimerManager().SetTimer(RegenerationHPTimerHandle,
			TimerDelegate,
			RecoveryInterval,
			true);
	}
}

void UT3RegenerationRune::OnUnsocketed_Implementation(AT3CharacterBase* OwnerChar)
{
	if (!IsValid(OwnerChar))
	{
		return;
	}
	
	OwnerChar->OnStatChanged.RemoveDynamic(this, &UT3RegenerationRune::RegenerationHP);

	OwnerChar->GetWorldTimerManager().ClearTimer(RegenerationHPTimerHandle);
	
	CachedOwner = nullptr;
}

void UT3RegenerationRune::RegenerationHP(ET3StatType StatType, float CurrentHP, float MaxHP)
{
	AT3CharacterBase* Owner = CachedOwner.Get();
	if (!IsValid(Owner))
	{
		return;
	}
	
	if (CurrentHP / MaxHP >= 0.3f)
	{
		Owner->GetWorldTimerManager().ClearTimer(RegenerationHPTimerHandle);
		return;
	}
	
	if (Owner->GetWorldTimerManager().IsTimerActive(RegenerationHPTimerHandle))
	{
		return;
	}
	
	FTimerDelegate TimerDelegate;
	TimerDelegate.BindUObject(Owner, &AT3CharacterBase::RestoreHP, HealAmount);

	Owner->GetWorldTimerManager().SetTimer(
		RegenerationHPTimerHandle,
		TimerDelegate,
		RecoveryInterval,
		true);
}
