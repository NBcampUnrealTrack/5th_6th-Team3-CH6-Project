#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "T3ItemUseComponent.generated.h"

DECLARE_DYNAMIC_DELEGATE(FOnCoolTimeEnded);

struct FT3ConsumableItemData;
class AT3CharacterBase;
class USoundCue;

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
	
	// 다음 틱에 실행될 쿨타임 해제 함수들 (타이머 매니저 컨테이너 변경 방지)
	void ClearHPPotionCoolTime();
	void ClearMPPotionCoolTime();
	void ClearPowerPotionCoolTime();
	void ClearDefensePotionCoolTime();
	void ClearSpeedPotionCoolTime();
	void ClearBerserkPotionCoolTime();
	
	void PlayItemUseEffect(FT3ConsumableItemData ItemData);
	
	float OriginalPowerValue;
	float OriginalDefenseValue;
	float OriginalSpeedValue;
	
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

	UPROPERTY(EditDefaultsOnly, Category = "Sound")
	TObjectPtr<USoundCue> HealSound;
	
public:
	bool ApplyConsumableItem(const FT3ConsumableItemData& ItemData);
	
	bool UseEquipmentItem(const FT3ConsumableItemData& ItemData);
};