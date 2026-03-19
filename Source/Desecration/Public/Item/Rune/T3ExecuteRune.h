#pragma once

#include "CoreMinimal.h"
#include "Item/Rune/T3RuneBase.h"
#include "T3ExecuteRune.generated.h"

UCLASS()
class DESECRATION_API UT3ExecuteRune : public UT3RuneBase
{
	GENERATED_BODY()

public:
	virtual void OnSocketed_Implementation(AT3CharacterBase* OwnerChar) override;
	
	virtual void OnUnsocketed_Implementation(AT3CharacterBase* OwnerChar) override;
	
	virtual void SetGrade(ET3RuneGrade InGrade) override;

private:
	UFUNCTION()
	void CheckExecution(AActor* HitTarget, float DamageDealt);
	
	UPROPERTY(EditDefaultsOnly, Category = "Value|HPThreshold")
	float HPThresholdPercentNormal = 10.0f;
	
	UPROPERTY(EditDefaultsOnly, Category = "Value|HPThreshold")
	float HPThresholdPercentEpic = 20.0f;
	
	UPROPERTY(EditDefaultsOnly, Category = "Value|HPThreshold")
	float HPThresholdPercentLegendary = 30.0f;
	
	float BossAttackBonusPercentByGrade = 0.0f;
	
	UPROPERTY(EditDefaultsOnly, Category = "Value|BossAttackBonus")
	float BossAttackBonusPercentNormal = 10.0f;
	
	UPROPERTY(EditDefaultsOnly, Category = "Value|BossAttackBonus")
	float BossAttackBonusPercentEpic = 20.0f;
	
	UPROPERTY(EditDefaultsOnly, Category = "Value|BossAttackBonus")
	float BossAttackBonusPercentLegendary = 30.0f;
};
