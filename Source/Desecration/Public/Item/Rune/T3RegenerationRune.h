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
	
	UPROPERTY(EditDefaultsOnly, Category = "Value")
	float HealAmountByGrade = 1.0f;
	
	UPROPERTY(EditDefaultsOnly, Category = "Value")
	float RecoveryInterval = 1.0f;
	
	UPROPERTY(EditDefaultsOnly, Category = "Value")
	float RecoveryTargetHPPercentByGrade= 0.3f;
	
	UFUNCTION()
	void RegenerationHP(ET3StatType StatType, float CurrentHP, float MaxHP);
};
