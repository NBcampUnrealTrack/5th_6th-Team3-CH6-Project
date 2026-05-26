#include "Item/Rune/T3RegenerationRune.h"

#include "Equipment/T3EquipmentTypes.h"
#include "NiagaraFunctionLibrary.h"

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

	if (!OwnerChar->bIsDead && CurrentHP / MaxHP < RecoveryTargetHPPercentByGrade / 100.0f)
	{
		FTimerDelegate TimerDelegate;
		TimerDelegate.BindUObject(OwnerChar, &AT3CharacterBase::RestoreHP, ValueByGrade);

		OwnerChar->GetWorldTimerManager().SetTimer(RegenerationHPTimerHandle,
			TimerDelegate,
			RecoveryInterval,
			true);

		ActiveEffect = PlayTriggerEffect(OwnerChar, NAME_None, false);
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

	if (IsValid(ActiveEffect))
	{
		ActiveEffect->DeactivateImmediate();
		ActiveEffect = nullptr;
	}

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

	if (Owner->bIsDead || CurrentHP <= 0.f)
	{
		Owner->GetWorldTimerManager().ClearTimer(RegenerationHPTimerHandle);

		if (IsValid(ActiveEffect))
		{
			ActiveEffect->DeactivateImmediate();
			ActiveEffect = nullptr;
		}

		return;
	}

	if (CurrentHP / MaxHP >= RecoveryTargetHPPercentByGrade / 100.0f)
	{
		Owner->GetWorldTimerManager().ClearTimer(RegenerationHPTimerHandle);

		if (IsValid(ActiveEffect))
		{
			ActiveEffect->DeactivateImmediate();
			ActiveEffect = nullptr;
		}

		return;
	}
	
	if (Owner->GetWorldTimerManager().IsTimerActive(RegenerationHPTimerHandle))
	{
		return;
	}
	
	FTimerDelegate TimerDelegate;
	TimerDelegate.BindUObject(Owner, &AT3CharacterBase::RestoreHP, ValueByGrade);

	Owner->GetWorldTimerManager().SetTimer(
		RegenerationHPTimerHandle,
		TimerDelegate,
		RecoveryInterval,
		true);

	ActiveEffect = PlayTriggerEffect(Owner, NAME_None, false);
}

void UT3RegenerationRune::SetGrade(ET3RuneGrade InGrade)
{
	switch (InGrade)
	{
	case ET3RuneGrade::Normal:
		
		ValueByGrade = HealAmountNormal;
		RecoveryTargetHPPercentByGrade = RecoveryTargetHPPercentNormal;
		break;
		
	case ET3RuneGrade::Epic:
		
		ValueByGrade = HealAmountEpic;
		RecoveryTargetHPPercentByGrade = RecoveryTargetHPPercentEpic;
		break;
		
	case ET3RuneGrade::Legendary:
		
		ValueByGrade = HealAmountLegendary;
		RecoveryTargetHPPercentByGrade = RecoveryTargetHPPercentLegendary;
		break;
		
	default:
		break;
	}
}
