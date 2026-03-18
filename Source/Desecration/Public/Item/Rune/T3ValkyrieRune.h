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
	
private:
	UPROPERTY(EditDefaultsOnly, Category = "Value")
	float ValueByGrade = 10.0f;
};
