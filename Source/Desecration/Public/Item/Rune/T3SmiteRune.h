#pragma once

#include "CoreMinimal.h"
#include "Item/Rune/T3RuneBase.h"
#include "T3SmiteRune.generated.h"

UCLASS()
class DESECRATION_API UT3SmiteRune : public UT3RuneBase
{
	GENERATED_BODY()

public:
	virtual void OnSocketed_Implementation(AT3CharacterBase* OwnerChar) override;

	virtual void OnUnsocketed_Implementation(AT3CharacterBase* OwnerChar) override;

	virtual void SetGrade(ET3RuneGrade InGrade) override;

private:
	void OnSmiteTriggeredHandler(FVector HitLocation);

	TWeakObjectPtr<AT3CharacterBase> CachedOwner;

	FDelegateHandle SmiteTriggeredHandle;

	UPROPERTY(EditDefaultsOnly, Category = "VFX")
	TObjectPtr<UNiagaraSystem> WeaponHitEffect;

	UPROPERTY(EditDefaultsOnly, Category = "Value|Count")
	int32 TriggerAttackCount = 3;

	UPROPERTY(EditDefaultsOnly, Category = "Value")
	float AttackBonusDamagePercentNormal = 30.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Value")
	float AttackBonusDamagePercentEpic = 50.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Value")
	float AttackBonusDamagePercentLegendary = 70.0f;
};
