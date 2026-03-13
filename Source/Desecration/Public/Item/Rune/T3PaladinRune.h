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
	
private:	
	float OriginalHolyGaugeChargeAmount = 0.f;
	
	UPROPERTY(EditDefaultsOnly, Category="Value")
	float ValueByGrade = 1.0f;
};
