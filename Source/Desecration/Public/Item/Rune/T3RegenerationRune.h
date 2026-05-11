#pragma once

#include "CoreMinimal.h"
#include "Item/Rune/T3RuneBase.h"
#include "NiagaraComponent.h"
#include "T3RegenerationRune.generated.h"

UCLASS()
class DESECRATION_API UT3RegenerationRune : public UT3RuneBase
{
	GENERATED_BODY()
	
public:
	virtual void OnSocketed_Implementation(AT3CharacterBase* OwnerChar) override;
	
	virtual void OnUnsocketed_Implementation(AT3CharacterBase* OwnerChar) override;
	
	virtual void SetGrade(ET3RuneGrade InGrade) override;
	
private:
	TWeakObjectPtr<AT3CharacterBase> CachedOwner;

	FTimerHandle RegenerationHPTimerHandle;

	UPROPERTY()
	TObjectPtr<UNiagaraComponent> ActiveEffect;
	
	UPROPERTY(EditDefaultsOnly, Category = "Value")
	float HealAmountNormal = 1.0f;
	
	UPROPERTY(EditDefaultsOnly, Category = "Value")
	float HealAmountEpic = 1.5f;
	
	UPROPERTY(EditDefaultsOnly, Category = "Value")
	float HealAmountLegendary = 2.0f;
	
	float RecoveryTargetHPPercentByGrade = 0.0f;
	
	UPROPERTY(EditDefaultsOnly, Category = "Value")
	float RecoveryTargetHPPercentNormal= 30.0f;
	
	UPROPERTY(EditDefaultsOnly, Category = "Value")
	float RecoveryTargetHPPercentEpic= 40.0f;
	
	UPROPERTY(EditDefaultsOnly, Category = "Value")
	float RecoveryTargetHPPercentLegendary = 50.0f;
	
	UPROPERTY(EditDefaultsOnly, Category = "Value")
	float RecoveryInterval = 1.0f;
	
	UFUNCTION()
	void RegenerationHP(ET3StatType StatType, float CurrentHP, float MaxHP);
};
