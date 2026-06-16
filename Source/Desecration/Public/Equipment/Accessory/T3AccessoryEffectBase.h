#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "NiagaraSystem.h"
#include "T3AccessoryEffectBase.generated.h"

class AT3CharacterBase;
class UNiagaraComponent;

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
	UNiagaraComponent* PlayEquipEffect(AActor* Target, FName SocketName = NAME_None, bool bAutoDestroy = false);

	void StopEquipEffect();

	TWeakObjectPtr<AT3CharacterBase> CachedOwner;

	UPROPERTY(EditDefaultsOnly, Category = "VFX")
	TObjectPtr<UNiagaraSystem> EquipEffect;

	UPROPERTY()
	TObjectPtr<UNiagaraComponent> ActiveEquipEffect = nullptr;
};
