#pragma once

#include "CoreMinimal.h"
#include "Item/Rune/T3RuneBase.h"
#include "T3TaoistRune.generated.h"

UCLASS()
class DESECRATION_API UT3TaoistRune : public UT3RuneBase
{
	GENERATED_BODY()
	
public:
	virtual void OnSocketed_Implementation(AT3CharacterBase* OwnerChar) override;
	
	virtual void OnUnsocketed_Implementation(AT3CharacterBase* OwnerChar) override;
	
	virtual void SetGrade(ET3RuneGrade InGrade) override;
	
private:
	UPROPERTY(EditDefaultsOnly, Category = "Value")
	float CloneAttackBonusPercentNormal = 10.0f;
	
	UPROPERTY(EditDefaultsOnly, Category = "Value")
	float CloneAttackBonusPercentEpic = 20.0f;
	
	UPROPERTY(EditDefaultsOnly, Category = "Value")
	float CloneAttackBonusPercentLegendary = 30.0f;
	
	int32 OriginalCloneCount = 0;
	
	uint8 bIsLegendary : 1 = false;
};
