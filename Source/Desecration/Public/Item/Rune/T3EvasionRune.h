#pragma once

#include "CoreMinimal.h"
#include "Item/Rune/T3RuneBase.h"
#include "T3EvasionRune.generated.h"

UCLASS()
class DESECRATION_API UT3EvasionRune : public UT3RuneBase
{
	GENERATED_BODY()
	
public:
	virtual void OnSocketed_Implementation(AT3CharacterBase* OwnerChar) override;
	
	virtual void OnUnsocketed_Implementation(AT3CharacterBase* OwnerChar) override;
	
	virtual void SetGrade(ET3RuneGrade InGrade) override;
	
private:	
	UPROPERTY(EditDefaultsOnly, Category = "Value")
	float RollAnimSpeedBonusPercentNormal = 30.0f;
	
	UPROPERTY(EditDefaultsOnly, Category = "Value")
	float RollAnimSpeedBonusPercentEpic = 50.0f;
	
	UPROPERTY(EditDefaultsOnly, Category = "Value")
	float RollAnimSpeedBonusPercentLegendary = 70.0f;
};
