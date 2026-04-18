#pragma once

#include "CoreMinimal.h"
#include "T3AccessoryEffectBase.h"
#include "T3BalrogAccessoryEffect.generated.h"

UCLASS()
class DESECRATION_API UT3BalrogAccessoryEffect : public UT3AccessoryEffectBase
{
	GENERATED_BODY()
	
public:
	virtual void OnEquipped_Implementation(AT3CharacterBase* OwnerChar) override;
	
	virtual void OnUnequipped_Implementation(AT3CharacterBase* OwnerChar) override;

private:
	void PerformDamage();

	UPROPERTY(EditDefaultsOnly)
	float DamageRadius = 300.f;
	
	UPROPERTY(EditDefaultsOnly)
	float DamageAmount = 5.f;
	
	UPROPERTY(EditDefaultsOnly)
	float DamageInterval = 3.f;
	
	FTimerHandle DamageTimerHandle;
};
