#pragma once

#include "CoreMinimal.h"
#include "Item/Rune/T3RuneBase.h"
#include "Player/T3CharacterBase.h"
#include "T3RageRune.generated.h"

UCLASS()
class DESECRATION_API UT3RageRune : public UT3RuneBase
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, Category = "Rune|Rage")
	float AttackBonusMultiplier = 0.2f;

	virtual void OnSocketed_Implementation(AT3CharacterBase* OwnerChar) override;
	
	virtual void OnUnsocketed_Implementation(AT3CharacterBase* OwnerChar) override;

private:
	TWeakObjectPtr<AT3CharacterBase> CachedOwner;

	UFUNCTION()
	void OnHPChanged(ET3StatType StatType, float CurrentValue, float MaxValue);

	UFUNCTION()
	void OnEquipmentStatsUpdated(float NewAtk, float NewDef);

	void ApplyAttackBonus();
};