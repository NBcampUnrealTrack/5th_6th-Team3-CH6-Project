#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "NiagaraSystem.h"
#include "Player/T3CharacterBase.h"
#include "T3RuneBase.generated.h"

class UNiagaraComponent;

enum class ET3RuneGrade : uint8;

UCLASS(Abstract, Blueprintable)
class DESECRATION_API UT3RuneBase : public UObject
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, Category = "Rune")
	void OnSocketed(AT3CharacterBase* OwnerChar);

	UFUNCTION(BlueprintNativeEvent, Category = "Rune")
	void OnUnsocketed(AT3CharacterBase* OwnerChar);
	
	virtual void SetGrade(ET3RuneGrade InGrade);
	
	virtual bool CanUnsocket() const;
	
	virtual float GetCooldownRemaining() const;
	
	virtual void RestoreCooldown(float RemainingTime);

	virtual void ResetCooldown() {}

protected:
	float ValueByGrade = 0.0f;

	UPROPERTY(EditDefaultsOnly, Category = "VFX")
	TObjectPtr<UNiagaraSystem> TriggerEffect;

	UNiagaraComponent* PlayTriggerEffect(AActor* Target, FName SocketName = NAME_None, bool bAutoDestroy = true);
};
