#pragma once

#include "CoreMinimal.h"
#include "T3AccessoryEffectBase.h"
#include "T3AngelAccessoryEffect.generated.h"

UCLASS()
class DESECRATION_API UT3AngelAccessoryEffect : public UT3AccessoryEffectBase
{
	GENERATED_BODY()
	
public:
	virtual void OnEquipped_Implementation(AT3CharacterBase* OwnerChar) override;
	
	virtual void OnUnequipped_Implementation(AT3CharacterBase* OwnerChar) override;

private:
	void ApplySlowAura();

	UPROPERTY(EditDefaultsOnly)
	float SlowRadius = 400.f;

	UPROPERTY(EditDefaultsOnly)
	float MoveAnimSlowAmount = 0.5f;

	UPROPERTY(EditDefaultsOnly)
	float AttackAnimSlowAmount = 0.7f;

	UPROPERTY(EditDefaultsOnly)
	float SlowInterval = 1.0f;

	UPROPERTY(EditDefaultsOnly)
	bool bShowDebugRadius = false;

	FTimerHandle SlowTimerHandle;

	TArray<TWeakObjectPtr<AActor>> SlowedMonsters;
};