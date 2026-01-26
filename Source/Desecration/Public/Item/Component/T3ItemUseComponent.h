#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "T3ItemUseComponent.generated.h"

struct FT3ConsumableItemData;
class AT3CharacterBase;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class DESECRATION_API UT3ItemUseComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UT3ItemUseComponent();
	
protected:
	virtual void BeginPlay() override;
	
private:
	void RecoverHPTick();
	void RecoverMPTick();
	void EndPowerPotionEffect();
	void EndDefensePotionEffect();
	void EndSpeedPotionEffect();
	void EndBerserkPotionEffect();
	
	void EndHPPotionCoolTime();
	void EndMPPotionCoolTime();
	void EndPowerPotionCoolTime();
	void EndDefensePotionCoolTime();
	void EndSpeedPotionCoolTime();
	void EndBerserkPotionCoolTime();
	
	float PendingPowerValue;
	float PendingDefenseValue;
	float PendingSpeedValue;
	float PendingBerserkPowerValue;
	float PendingBerserkDefenseValue;
	
	uint8 bIsHPPotionActive : 1;
	uint8 bIsMPPotionActive : 1;
	uint8 bIsPowerPotionActive : 1;
	uint8 bIsDefensePotionActive : 1;
	uint8 bIsSpeedPotionActive : 1;
	uint8 bIsBerserkPotionActive : 1;
	
	UPROPERTY(EditDefaultsOnly, Category = "RecoverInterval")
	float RecoverHPInterval;
	float RecoverHPTickCount;
	float RecoverHPPerTick;
	float RecoverHPAmount;
	float AccumulatedRecoverHP;
	
	UPROPERTY(EditDefaultsOnly, Category = "RecoverInterval")
	float RecoverMPInterval;
	float RecoverMPTickCount;
	float RecoverMPPerTick;
	float RecoverMPAmount;
	float AccumulatedRecoverMP;
	
	FTimerHandle RecoverHPTimerHandle;
	FTimerHandle RecoverMPTimerHandle;
	FTimerHandle PowerPotionActiveTimerHandle;
	FTimerHandle DefensePotionActiveTimerHandle;
	FTimerHandle SpeedPotionActiveTimerHandle;
	FTimerHandle BerserkPotionActiveTimerHandle;
	
	FTimerHandle HPPotionCoolTimerHandle;
	FTimerHandle MPPotionCoolTimerHandle;
	FTimerHandle PowerPotionCoolTimerHandle;
	FTimerHandle DefensePotionCoolTimerHandle;
	FTimerHandle SpeedPotionCoolTimerHandle;
	FTimerHandle BerserkPotionCoolTimerHandle;
	
	UPROPERTY()
	AT3CharacterBase* OwnerCharacter;

public:
	bool ApplyConsumableItem(const FT3ConsumableItemData& ItemData);
};
