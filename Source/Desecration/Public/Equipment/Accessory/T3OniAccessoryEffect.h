#pragma once

#include "CoreMinimal.h"
#include "T3AccessoryEffectBase.h"
#include "T3OniAccessoryEffect.generated.h"

UCLASS()
class DESECRATION_API UT3OniAccessoryEffect : public UT3AccessoryEffectBase
{
	GENERATED_BODY()
	
public:
	virtual void OnEquipped_Implementation(AT3CharacterBase* OwnerChar) override;
	
	virtual void OnUnequipped_Implementation(AT3CharacterBase* OwnerChar) override;
};