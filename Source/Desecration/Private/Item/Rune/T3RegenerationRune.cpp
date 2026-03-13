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

	if (CurrentHP / MaxHP < RecoveryTargetHPPercentByGrade)
	{
		FTimerDelegate TimerDelegate;
		TimerDelegate.BindUObject(OwnerChar, &AT3CharacterBase::RestoreHP, HealAmountByGrade);
		
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
	if (StatType != ET3StatType::HP)
	{
		return;
	}
	
	AT3CharacterBase* Owner = CachedOwner.Get();
	if (!IsValid(Owner))
	{
		return;
	}
	
	if (CurrentHP / MaxHP >= RecoveryTargetHPPercentByGrade)
	{
		Owner->GetWorldTimerManager().ClearTimer(RegenerationHPTimerHandle);
		return;
	}
	
	if (Owner->GetWorldTimerManager().IsTimerActive(RegenerationHPTimerHandle))
	{
		return;
	}
	
	FTimerDelegate TimerDelegate;
	TimerDelegate.BindUObject(Owner, &AT3CharacterBase::RestoreHP, HealAmountByGrade);

	Owner->GetWorldTimerManager().SetTimer(
		RegenerationHPTimerHandle,
		TimerDelegate,
		RecoveryInterval,
		true);
}
