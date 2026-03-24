#pragma once

#include "CoreMinimal.h"
#include "Item/Rune/T3RuneBase.h"
#include "T3PaladinRune.generated.h"

UCLASS()
class DESECRATION_API UT3PaladinRune : public UT3RuneBase
{
	GENERATED_BODY()
	
public:
	virtual void OnSocketed_Implementation(AT3CharacterBase* OwnerChar) override;
	
	virtual void OnUnsocketed_Implementation(AT3CharacterBase* OwnerChar) override;
	
	virtual void SetGrade(ET3RuneGrade InGrade) override;
	
private:
	float OriginalHolyGaugeChargeAmount = 0.0f;
	
	float OriginalHolyModeAttackSpeedMultiplier = 1.0f;
	
	UPROPERTY(EditDefaultsOnly, Category="Value")
	float HolyGaugeChargeBonusPercentNormal = 20.0f;
	
	UPROPERTY(EditDefaultsOnly, Category="Value")
	float HolyGaugeChargeBonusPercentEpic = 35.0f;
	
	UPROPERTY(EditDefaultsOnly, Category="Value")
	float HolyGaugeChargeBonusPercentLegendary = 50.0f;
	
	UPROPERTY(EditDefaultsOnly, Category="Value|Legendary")
	float LegendaryAttackSpeedBonusPercent = 50.0f;
	
	uint8 bIsLegendary : 1 = false;
};
