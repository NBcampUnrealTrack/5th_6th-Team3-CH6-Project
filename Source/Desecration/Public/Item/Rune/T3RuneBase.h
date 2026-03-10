#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Player/T3CharacterBase.h"
#include "T3RuneBase.generated.h"

UCLASS(Abstract, Blueprintable)
class DESECRATION_API UT3RuneBase : public UObject
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, Category = "Rune")
	void OnSocketed(AT3CharacterBase* OwnerChar);

	UFUNCTION(BlueprintNativeEvent, Category = "Rune")
	void OnUnsocketed(AT3CharacterBase* OwnerChar);
};
