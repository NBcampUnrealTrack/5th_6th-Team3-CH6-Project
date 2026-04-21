#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "T3AccessoryEffectBase.generated.h"

class AT3CharacterBase;

UCLASS(Abstract, Blueprintable)
class DESECRATION_API UT3AccessoryEffectBase : public UObject
{
	GENERATED_BODY()
	
public:
	UFUNCTION(BlueprintNativeEvent)
	void OnEquipped(AT3CharacterBase* OwnerChar);

	UFUNCTION(BlueprintNativeEvent)
	void OnUnequipped(AT3CharacterBase* OwnerChar);

protected:
	TWeakObjectPtr<AT3CharacterBase> CachedOwner;
};