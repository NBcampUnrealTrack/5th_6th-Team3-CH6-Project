#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/DataTable.h"
#include "T3InventoryComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnInventoryInitialized);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnInventoryUpdated);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnToggleItemEquipped);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnChangedBuffItemSlot);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnBuffItemUsed);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnSwapRecoverSlot);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnRecoverItemUsed);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnCooldownUpdated, FName, ItemID, float, RemainingTime);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnCooldownProgressUpdated, FName, ItemID, float, Progress);

UENUM(BlueprintType)
enum class EConsumableItemType : uint8
{
	None,
	Recover,
	Buff
};

class AT3CharacterBase;

USTRUCT(BlueprintType)
struct FInventorySlot
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inventory Slot")
	FName ItemID = NAME_None;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inventory Slot")
	int32 ItemStack = 0;
};

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class DESECRATION_API UT3InventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UT3InventoryComponent();

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void AddItem(const FName& ItemName);
	
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void UseItem(int32 SlotIndex);
	
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool RemoveItem(const FName& ItemName);
	
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void SwapSlots(int32 SourceSlotIndex, int32 TargetSlotIndex);
	
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	float GetCooldownProgressByItemID(const FName& ItemName);

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	int32 GetMoney() const;
	
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	int32 SetMoney(int32 NewMoney);
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inventory")
	TArray<FInventorySlot> Items;
	
	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	int32 InventorySize;
	
	UPROPERTY(BlueprintAssignable, Category = "Inventory")
	FOnInventoryUpdated OnInventoryUpdated;
	
	UPROPERTY(BlueprintAssignable, Category = "Inventory")
	FOnInventoryInitialized OnInventoryInitialized;
	
	UPROPERTY(BlueprintAssignable, Category = "Cooldown")
	FOnCooldownUpdated OnCooldownUpdated;
	
	UPROPERTY(BlueprintAssignable, Category = "Cooldown")
	FOnCooldownProgressUpdated OnCooldownProgressUpdated;
	
protected:
	virtual void BeginPlay() override;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Money")
	int32 Money;
	
	UPROPERTY(BlueprintReadOnly)
	EConsumableItemType ConsumableItemType = EConsumableItemType::None;
	
#pragma region Stone // 강화석
	int32 NormalStoneCount;
	
	int32 EpicStoneCount;
	
	int32 LegendaryStoneCount;
	
public:
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	int32 GetNormalStoneCount() const;
	
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	int32 GetEpicStoneCount() const;
	
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	int32 GetLegendaryStoneCount() const;
	
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	int32 SetNormalStoneCount(int32 NewCount);
	
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	int32 SetEpicStoneCount(int32 NewCount);
	
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	int32 SetLegendaryStoneCount(int32 NewCount);
#pragma endregion
private:
	UPROPERTY()
	AT3CharacterBase* OwnerCharacter;
	
	UPROPERTY()
	TMap<FName, float> ItemCooldownStartTimes;
    
	UPROPERTY()
	TMap<FName, float> ItemCooldownDurations;
	
	FTimerHandle CooldownUpdateTimerHandle;
	
	void UpdateCooldowns();
	
#pragma region Buff Potion
public:
	UPROPERTY(BlueprintReadOnly, Category = "Equipment")
	TArray<FName> EquippedItemIDs;
	
	UFUNCTION(BlueprintCallable, Category = "Equipment")
	void ToggleEquipItem(const FName& ItemName);
	
	UFUNCTION(BlueprintCallable, Category = "Equipment")
	void UnequipItem(const FName& ItemName);
	
	UFUNCTION(BlueprintCallable, Category = "Equipment")
	bool IsItemEquipped(const FName& ItemName) const;
	
	UFUNCTION(BlueprintCallable, Category = "Equipment")
	int32 GetEquippedItemIndex(const FName& ItemName) const;
	
	UFUNCTION(BlueprintCallable, Category = "Equipment")
	void SwapEquippedItem();
	
	UFUNCTION(BlueprintCallable, Category = "Equipment")
	void UseEquippedItem();
	
	UFUNCTION(BlueprintCallable, Category = "Equipment")
	int32 GetCurrentBuffItemCount() const;
	
	UFUNCTION(BlueprintCallable, Category = "Equipment")
	int32 GetNextBuffItemCount() const;
	
	UFUNCTION(BlueprintCallable, Category = "Equipment")
	FName GetCurrentBuffItemName() const;
	
	UFUNCTION(BlueprintCallable, Category = "Equipment")
	FName GetNextBuffItemName() const;
	
	UPROPERTY(BlueprintAssignable, Category = "Equipment")
	FOnToggleItemEquipped OnToggleItemEquipped;
	
	UPROPERTY(BlueprintAssignable, Category = "Equipment")
	FOnChangedBuffItemSlot OnChangedBuffItemSlot;
	
	UPROPERTY(BlueprintAssignable, Category = "Equipment")
	FOnBuffItemUsed OnBuffItemUsed;
#pragma endregion
	
#pragma region Recover Potion
protected:
	UPROPERTY(BlueprintReadOnly, Category = "Recover")
	int32 HPPotionCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Recover")
	int32 MPPotionCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Recover")
	FName HPPotionID = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "Recover")
	FName MPPotionID = NAME_None;
	
	UPROPERTY(BlueprintReadOnly, Category = "Recover")
	FName CurrentPotionID = NAME_None;
	
public:
	UFUNCTION(BlueprintCallable, Category = "Recover")
	void InitializePotionIDs();

	UFUNCTION(BlueprintCallable, Category = "Recover")
	void SetHPPotionCount(int32 Count);

	UFUNCTION(BlueprintCallable, Category = "Recover")
	void SetMPPotionCount(int32 Count);

	UFUNCTION(BlueprintCallable, Category = "Recover")
	void UseCurrentPotion();

	UFUNCTION(BlueprintCallable, Category = "Recover")
	void SwapHPMPSlot();

	UFUNCTION(BlueprintCallable, Category = "Recover")
	FName GetCurrentPotionID() const;

	UFUNCTION(BlueprintCallable, Category = "Recover")
	FName GetNextPotionID() const;

	UFUNCTION(BlueprintCallable, Category = "Recover")
	int32 GetHPPotionCount() const;

	UFUNCTION(BlueprintCallable, Category = "Recover")
	int32 GetMPPotionCount() const;
	
	UFUNCTION(BlueprintCallable, Category = "Recover")
	int32 GetCurrentPotionCount() const;

	UPROPERTY(BlueprintAssignable, Category = "Recover")
	FOnSwapRecoverSlot OnSwapRecoverSlot;
	
	UPROPERTY(BlueprintAssignable, Category = "Recover")
	FOnRecoverItemUsed OnRecoverItemUsed;
private:
	void UseHPPotion();
	
	void UseMPPotion();
#pragma endregion
};