#include "Item/Rune/T3UndyingRune.h"

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
	else
	{
		AT3CharacterBase* Owner = CachedOwner.Get();
		if (!IsValid(Owner))
		{
			return;
		}
	
		bIsCooldown = true;
	
		Owner->SetIsUndyingState(false);
		
		RestoreHealthFromUndying(Owner);
		
		Owner->GetWorldTimerManager().SetTimer(
			CooldownTimerHandle,
			this,
			&UT3UndyingRune::OnCooldownFinished,
			Cooldown,
			false);
	}
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
	
	OwnerChar->SetCurrentHP(OwnerChar->GetMaxHP() * (NormalValue / 100));

	UE_LOG(LogTemp, Log, TEXT("회복 완료"));
}
