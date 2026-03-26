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
	
	virtual void SetGrade(ET3RuneGrade InGrade) override;
	
	virtual float GetCooldownRemaining() const override;
	
	virtual void RestoreCooldown(float RemainingTime) override;
	
private:
	TWeakObjectPtr<AT3CharacterBase> CachedOwner;
	
	uint8 bIsSocketed : 1 = false;
	
	uint8 bIsCooldown : 1 = false;
	
	float CooldownByGrade = 0.0f;
	
	UPROPERTY(EditDefaultsOnly, Category = "Value")
	float CooldownNormal = 900.0f;
	
	UPROPERTY(EditDefaultsOnly, Category = "Value")
	float CooldownEpic = 600.0f;
	
	UPROPERTY(EditDefaultsOnly, Category = "Value")
	float CooldownLegendary = 300.0f;
	
	UPROPERTY(EditDefaultsOnly, Category = "Value")
	float OnDeathHealPercentNormal = 20.f;
	
	UPROPERTY(EditDefaultsOnly, Category = "Value")
	float OnDeathHealPercentEpic = 35.f;
	
	UPROPERTY(EditDefaultsOnly, Category = "Value")
	float OnDeathHealPercentLegendary = 50.f;
	
	FTimerHandle CooldownTimerHandle;
	
	UFUNCTION()
	void Activate();
	
	UFUNCTION()
	void OnCooldownFinished();
	
	void RestoreHealthFromUndying(AT3CharacterBase* OwnerChar);
};