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
	
private:
	TWeakObjectPtr<AT3CharacterBase> CachedOwner;

	UPROPERTY(EditDefaultsOnly, Category = "Rune")
	float ActiveDuration = 0.f;
	
	UPROPERTY(EditDefaultsOnly, Category = "Rune")
	float Cooldown = 0.f;
	
	UPROPERTY(EditDefaultsOnly, Category = "Rune")
	float AttackBonus = 0.1f;
	
	uint8 bIsCooldown = false;
	
	FTimerHandle ActiveTimerHandle;
	FTimerHandle CooldownTimerHandle;
	
	UFUNCTION()
	void Activate();
	
	UFUNCTION()
	void Deactivate();
	
	UFUNCTION()
	void OnCooldownFinished();
};
