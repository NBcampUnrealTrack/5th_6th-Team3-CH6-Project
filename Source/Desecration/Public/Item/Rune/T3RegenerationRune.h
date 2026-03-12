#pragma once

#include "CoreMinimal.h"
#include "Item/Rune/T3RuneBase.h"
#include "T3RegenerationRune.generated.h"

UCLASS()
class DESECRATION_API UT3RegenerationRune : public UT3RuneBase
{
	GENERATED_BODY()
	
public:
	virtual void OnSocketed_Implementation(AT3CharacterBase* OwnerChar) override;
	
	virtual void OnUnsocketed_Implementation(AT3CharacterBase* OwnerChar) override;
	
private:
	TWeakObjectPtr<AT3CharacterBase> CachedOwner;

	FTimerHandle RegenerationHPTimerHandle;
	
	UPROPERTY(EditDefaultsOnly)
	float HealAmount = 0.0f;
	
	UPROPERTY(EditDefaultsOnly)
	float RecoveryInterval = 0.0f;
	
	UFUNCTION()
	void RegenerationHP(ET3StatType StatType, float CurrentHP, float MaxHP);
};
