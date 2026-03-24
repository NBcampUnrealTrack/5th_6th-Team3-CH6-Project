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
	virtual void OnSocketed_Implementation(AT3CharacterBase* OwnerChar) override;
	
	virtual void OnUnsocketed_Implementation(AT3CharacterBase* OwnerChar) override;

	virtual void SetGrade(ET3RuneGrade InGrade) override;
	
private:
	TWeakObjectPtr<AT3CharacterBase> CachedOwner;

	UPROPERTY(EditDefaultsOnly, Category = "Value")
	float AttackBonusPerHPTenPercentNormal = 2.0f;
	
	UPROPERTY(EditDefaultsOnly, Category = "Value")
	float AttackBonusPerHPTenPercentEpic = 4.0f;
	
	UPROPERTY(EditDefaultsOnly, Category = "Value")
	float AttackBonusPerHPTenPercentLegendary = 6.0f;
	
	UFUNCTION()
	void OnHPChanged(ET3StatType StatType, float CurrentValue, float MaxValue);

	UFUNCTION()
	void OnEquipmentStatsUpdated(float NewAtk, float NewDef, float WeaponLevel);

	void ApplyAttackBonus();
};