#include "Item/Rune/T3UndyingRune.h"

#include "Equipment/T3EquipmentTypes.h"
#include "NiagaraFunctionLibrary.h"

void UT3UndyingRune::OnSocketed_Implementation(AT3CharacterBase* OwnerChar)
{
	if (!IsValid(OwnerChar))
	{
		return;
	}
	
	CachedOwner = OwnerChar;
	
	bIsSocketed = true;
	
	if (!bIsCooldown)
	{
		OwnerChar->SetIsUndyingState(true);
	}
	
	OwnerChar->OnUndyingTriggered.AddDynamic(this, &UT3UndyingRune::Activate);
}

void UT3UndyingRune::OnUnsocketed_Implementation(AT3CharacterBase* OwnerChar)
{
	if (!IsValid(OwnerChar))
	{
		return;
	}
	
	OwnerChar->SetIsUndyingState(false);
	
	OwnerChar->OnUndyingTriggered.RemoveDynamic(this, &UT3UndyingRune::Activate);
	
	bIsSocketed = false;
	
	CachedOwner = nullptr;
}

void UT3UndyingRune::Activate()
{
	if (bIsCooldown)
	{
		return;
	}
	
	AT3CharacterBase* Owner = CachedOwner.Get();
	
	if (!IsValid(Owner))
	{
		return;
	}

	bIsCooldown = true;

	Owner->SetIsUndyingState(false);
	
	RestoreHealthFromUndying(Owner);

	PlayTriggerEffect(Owner);

	Owner->GetWorldTimerManager().SetTimer(
		CooldownTimerHandle,
		this,
		&UT3UndyingRune::OnCooldownFinished,
		CooldownByGrade,
		false);
	
}

void UT3UndyingRune::OnCooldownFinished()
{
	bIsCooldown = false;

	AT3CharacterBase* Owner = CachedOwner.Get();
	if (!IsValid(Owner))
	{
		return;
	}
	
	UE_LOG(LogTemp, Warning, TEXT("불굴 룬 쿨타임 돌았음"));
	
	if(bIsSocketed)
	{
		Owner->SetIsUndyingState(true);
	}
}

void UT3UndyingRune::RestoreHealthFromUndying(AT3CharacterBase* OwnerChar)
{
	UE_LOG(LogTemp, Log, TEXT("회복 함수 호출됨"));
	
	if (!IsValid(OwnerChar))
	{
		return;
	}
	
	OwnerChar->SetCurrentHP(OwnerChar->GetMaxHP() * (ValueByGrade / 100));

	UE_LOG(LogTemp, Log, TEXT("회복 완료"));
}

void UT3UndyingRune::SetGrade(ET3RuneGrade InGrade)
{
	switch (InGrade)
	{
	case ET3RuneGrade::Normal:
		
		ValueByGrade = OnDeathHealPercentNormal;
		CooldownByGrade = CooldownNormal;
		break;
		
	case ET3RuneGrade::Epic:
		
		ValueByGrade = OnDeathHealPercentEpic;
		CooldownByGrade = CooldownEpic;
		break;
		
	case ET3RuneGrade::Legendary:
		
		ValueByGrade = OnDeathHealPercentLegendary;
		CooldownByGrade = CooldownLegendary;
		break;
		
	default:
		break;
	}
}

float UT3UndyingRune::GetCooldownRemaining() const
{
	if (!bIsCooldown || !CachedOwner.IsValid())
	{
		return 0.f;
	}
	
	return CachedOwner->GetWorldTimerManager().GetTimerRemaining(CooldownTimerHandle);
}

void UT3UndyingRune::ResetCooldown()
{
	if (CachedOwner.IsValid())
	{
		CachedOwner->GetWorldTimerManager().ClearTimer(CooldownTimerHandle);
	}

	bIsCooldown = false;

	if (bIsSocketed && CachedOwner.IsValid())
	{
		CachedOwner->SetIsUndyingState(true);
	}
}

void UT3UndyingRune::RestoreCooldown(float RemainingTime)
{
	if (RemainingTime <= 0.f || !CachedOwner.IsValid())
	{
		return;
	}
	
	bIsCooldown = true;
	
	CachedOwner->SetIsUndyingState(false);
	
	CachedOwner->GetWorldTimerManager().SetTimer(
		CooldownTimerHandle,
		this,
		&UT3UndyingRune::OnCooldownFinished,
		RemainingTime,
		false);
}
