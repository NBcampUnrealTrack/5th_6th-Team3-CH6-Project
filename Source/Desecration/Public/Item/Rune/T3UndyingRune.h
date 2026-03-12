#pragma once

#include "CoreMinimal.h"
#include "Item/Rune/T3RuneBase.h"
#include "T3UndyingRune.generated.h"

UCLASS()
class DESECRATION_API UT3UndyingRune : public UT3RuneBase
{
	GENERATED_BODY()
	
public:
	virtual void OnSocketed_Implementation(AT3CharacterBase* OwnerChar) override;
	
	virtual void OnUnsocketed_Implementation(AT3CharacterBase* OwnerChar) override;
	
private:
	TWeakObjectPtr<AT3CharacterBase> CachedOwner;
	
	uint8 bIsSocketed : 1 = false;
	
	uint8 bIsCooldown : 1 = false;
	
	UPROPERTY(EditDefaultsOnly, Category = "Rune")
	float Cooldown = 0.f;
	
	UPROPERTY(EditDefaultsOnly, Category = "Rune")
	float NormalValue = 0.f;
	
	FTimerHandle CooldownTimerHandle;
	
	UFUNCTION()
	void Activate();
	
	UFUNCTION()
	void OnCooldownFinished();
	
	void RestoreHealthFromUndying(AT3CharacterBase* OwnerChar);
};