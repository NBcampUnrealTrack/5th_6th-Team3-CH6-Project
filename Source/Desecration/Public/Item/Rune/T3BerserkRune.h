#pragma once

#include "CoreMinimal.h"
#include "Item/Rune/T3RuneBase.h"
#include "T3BerserkRune.generated.h"

UCLASS()
class DESECRATION_API UT3BerserkRune : public UT3RuneBase
{
	GENERATED_BODY()
	
public:
	virtual void OnSocketed_Implementation(AT3CharacterBase* OwnerChar) override;
	
	virtual void OnUnsocketed_Implementation(AT3CharacterBase* OwnerChar) override;
	
	virtual void SetGrade(ET3RuneGrade InGrade) override;

	virtual bool CanUnsocket() const override;

private:
	TWeakObjectPtr<AT3CharacterBase> CachedOwner;

	UPROPERTY(EditDefaultsOnly, Category = "Duration")
	float ActiveDuration = 0.f;
	
	UPROPERTY(EditDefaultsOnly, Category = "Cooldown")
	float Cooldown = 0.f;
	
	UPROPERTY(EditDefaultsOnly, Category = "Value")
	float OnHitAttackBonusPercentNormal = 10.0f;
	
	UPROPERTY(EditDefaultsOnly, Category = "Value")
	float OnHitAttackBonusPercentEpic = 20.0f;
	
	UPROPERTY(EditDefaultsOnly, Category = "Value")
	float OnHitAttackBonusPercentLegendary = 30.0f;
	
	uint8 bIsCooldown : 1 = false;
	
	FTimerHandle ActiveTimerHandle;
	FTimerHandle CooldownTimerHandle;
	
	UFUNCTION()
	void Activate();
	
	UFUNCTION()
	void Deactivate();
	
	UFUNCTION()
	void OnCooldownFinished();
};
