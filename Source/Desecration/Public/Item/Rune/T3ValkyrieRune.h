#pragma once

#include "CoreMinimal.h"
#include "Item/Rune/T3RuneBase.h"
#include "T3ValkyrieRune.generated.h"

UCLASS()
class DESECRATION_API UT3ValkyrieRune : public UT3RuneBase
{
	GENERATED_BODY()
	
public:
	virtual void OnSocketed_Implementation(AT3CharacterBase* OwnerChar) override;
	
	virtual void OnUnsocketed_Implementation(AT3CharacterBase* OwnerChar) override;
	
	virtual void SetGrade(ET3RuneGrade InGrade) override;
	
private:
	int32 OriginTriggerAttackCount = 0;
	
	UPROPERTY(EditDefaultsOnly, Category = "Value")
	float MaxHPRecoveryPercentNormal = 2.0f;
	
	UPROPERTY(EditDefaultsOnly, Category = "Value")
	float MaxHPRecoveryPercentEpic = 4.0f;
	
	UPROPERTY(EditDefaultsOnly, Category = "Value")
	float MaxHPRecoveryPercentLegendary = 6.0f;
	
	uint8 bIsLegendary : 1 = false;
};
