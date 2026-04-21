#include "Public/Item/Component/T3InventoryComponent.h"

//#include "IDetailTreeNode.h"
#include "Item/Component/T3ItemUseComponent.h"
#include "Player/T3CharacterBase.h"
#include "Public/Item/Data/T3ConsumableItemData.h"
UT3InventoryComponent::UT3InventoryComponent()
	:
InventorySize(20),
RuneInventorySize(20),
EtcInventorySize(20),
AccessoryInventorySize(20),
Money(0),
WeaponNormalStoneCount(0),
WeaponEpicStoneCount(0),
WeaponLegendaryStoneCount(0),
ArmorNormalStoneCount(0),
ArmorEpicStoneCount(0),
ArmorLegendaryStoneCount(0),
InitialHPPotionAmount(3),
InitialMPPotionAmount(3),
HPPotionCount(0),
MPPotionCount(0),
HPPotionID(NAME_None),
MPPotionID(NAME_None),
CurrentPotionID(NAME_None),
PotionAmountUpgradeLevel(0),
PotionRecoveryUpgradeLevel(0)
{
	PrimaryComponentTick.bCanEverTick = false;
	
	Items.SetNum(InventorySize);
	RuneItems.SetNum(RuneInventorySize);
	EtcItems.SetNum(EtcInventorySize);
	AccessoryItems.SetNum(AccessoryInventorySize);
}

void UT3InventoryComponent::BeginPlay()
{
	Super::BeginPlay();
	
	OwnerCharacter = Cast<AT3CharacterBase>(GetOwner());
	
	if (!IsValid(OwnerCharacter))
	{
		return;
	}
	
	InitializePotionIDs();
	SetHPPotionCount(GetMaxHPPotionCount());
	SetMPPotionCount(GetMaxMPPotionCount());
	
	OnInventoryInitialized.Broadcast();
}

void UT3InventoryComponent::AddItem(const FName& ItemName)
{
	if (ItemName == NAME_None)
	{
		UE_LOG(LogTemp, Error, TEXT("아이템 이름 비었음"));
		return;
	}

	if (!IsValid(OwnerCharacter))
	{
		UE_LOG(LogTemp, Error, TEXT("캐릭터 유효하지않음"));
		return;
	}
	
	UDataTable* ItemDataTable = OwnerCharacter->ItemDataTable;
	
	if (!IsValid(ItemDataTable))
	{
		UE_LOG(LogTemp, Error, TEXT("데이터 테이블 비었음"));
		return;
	}

	FT3ConsumableItemData* ItemRow = ItemDataTable->FindRow<FT3ConsumableItemData>(ItemName, TEXT("AddItem"));

	if (!ItemRow)
	{
		UE_LOG(LogTemp, Error, TEXT("아이템 Row 없음"));
		return;
	}

	for (int32 i = 0; i < Items.Num(); i++) // 슬롯에 추가 할 아이템이 이미 있는지 확인
	{
		if (Items[i].ItemID == ItemName)
		{
			Items[i].ItemStack++;
			
			UE_LOG(LogTemp, Log, TEXT("[%s] 1개 추가, 현재 개수: %d"), *ItemName.ToString(), Items[i].ItemStack)
			
			if (EquippedItemIDs.Contains(ItemName))
			{
				OnEquippedItemChanged.Broadcast();
			}
			
			OnInventoryUpdated.Broadcast();
			return;
		}
	}
	
	for (int32 i = 0; i < Items.Num(); i++)
	{
		if (Items[i].ItemID == NAME_None)
		{
			Items[i].ItemID = ItemName;
			Items[i].ItemStack = 1;
			
			UE_LOG(LogTemp, Log, TEXT("새로운 아이템 [%s] 획득, 현재 개수: %d"), *ItemName.ToString(), Items[i].ItemStack)

			OnInventoryUpdated.Broadcast();
			return;
		}
	}
}

void UT3InventoryComponent::UseItem(int32 SlotIndex)
{
	if (!Items.IsValidIndex(SlotIndex))
	{
		return;
	}
    
	FName const ItemIDToUse = Items[SlotIndex].ItemID;
    
	if (ItemIDToUse == NAME_None || Items[SlotIndex].ItemStack <= 0)
	{
		GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Red, TEXT("아이템 없음"));
		return;
	}
	
	if (!IsValid(OwnerCharacter) || !IsValid(OwnerCharacter->ItemDataTable))
	{
		return;
	}
	
	FT3ConsumableItemData* ItemRow =
		OwnerCharacter->ItemDataTable->FindRow<FT3ConsumableItemData>(ItemIDToUse, TEXT("UseItem"));
	
	if (!ItemRow)
	{
		return;
	}
	
	if (!OwnerCharacter->ItemUseComponent->ApplyConsumableItem(*ItemRow))
	{
		return;
	}
	
	ItemCooldownStartTimes.Emplace(ItemIDToUse, GetWorld()->GetTimeSeconds());
	ItemCooldownDurations.Emplace(ItemIDToUse, ItemRow->CoolTime);
	
	if (!GetWorld()->GetTimerManager().IsTimerActive(CooldownUpdateTimerHandle))
	{
		GetWorld()->GetTimerManager().SetTimer(
			CooldownUpdateTimerHandle,
			this,
			&UT3InventoryComponent::UpdateCooldowns,
			0.1f,
			true);
	}
	
	Items[SlotIndex].ItemStack--;

	UE_LOG(LogTemp, Log, TEXT("[%s]를 1개 사용했습니다. 현재 개수: %d"), *Items[SlotIndex].ItemID.ToString(), Items[SlotIndex].ItemStack)

	if (Items[SlotIndex].ItemStack <= 0)
	{
		UE_LOG(LogTemp, Log, TEXT("[%s]를 모두 사용했습니다."), *Items[SlotIndex].ItemID.ToString())
		
		Items[SlotIndex].ItemID = NAME_None;
		Items[SlotIndex].ItemStack = 0;
	}

	OnInventoryUpdated.Broadcast();
}

void UT3InventoryComponent::SwapSlots(int32 SourceSlotIndex, int32 TargetSlotIndex)
{
	if (!Items.IsValidIndex(SourceSlotIndex) || !Items.IsValidIndex(TargetSlotIndex))
	{
		return;
	}

	if (SourceSlotIndex == TargetSlotIndex)
	{
		return;
	}

	FInventorySlot TempSlot = Items[SourceSlotIndex];
	Items[SourceSlotIndex] = Items[TargetSlotIndex];
	Items[TargetSlotIndex] = TempSlot;

	OnInventoryUpdated.Broadcast();
}

void UT3InventoryComponent::SwapEtcSlots(int32 SourceSlotIndex, int32 TargetSlotIndex)
{
	if (!EtcItems.IsValidIndex(SourceSlotIndex) || !EtcItems.IsValidIndex(TargetSlotIndex))
	{
		return;
	}

	if (SourceSlotIndex == TargetSlotIndex)
	{
		return;
	}

	FInventorySlot TempSlot = EtcItems[SourceSlotIndex];
	EtcItems[SourceSlotIndex] = EtcItems[TargetSlotIndex];
	EtcItems[TargetSlotIndex] = TempSlot;

	OnEtcInventoryUpdated.Broadcast();
}

void UT3InventoryComponent::SwapRuneSlots(int32 SourceSlotIndex, int32 TargetSlotIndex)
{
	if (!RuneItems.IsValidIndex(SourceSlotIndex) || !RuneItems.IsValidIndex(TargetSlotIndex))
	{
		return;
	}

	if (SourceSlotIndex == TargetSlotIndex)
	{
		return;
	}

	FInventorySlot TempSlot = RuneItems[SourceSlotIndex];
	RuneItems[SourceSlotIndex] = RuneItems[TargetSlotIndex];
	RuneItems[TargetSlotIndex] = TempSlot;

	OnRuneInventoryUpdated.Broadcast();
}

bool UT3InventoryComponent::RemoveItem(const FName& ItemName)
{
	for (FInventorySlot& Item : Items)
	{
		if (Item.ItemID == ItemName)
		{
			Item.ItemStack--;
			
			if (Item.ItemStack <= 0)
			{
				Item.ItemID = NAME_None;
				Item.ItemStack = 0;
				
				if (EquippedItemIDs.Remove(ItemName) != 0)
				{
					OnEquippedItemChanged.Broadcast();
				}
			}
			
			OnInventoryUpdated.Broadcast();
			
			return true;
		}
	}
	return false;
}

float UT3InventoryComponent::GetCooldownProgressByItemID(const FName& ItemName)
{
	if (ItemName == NAME_None)
	{
		return 1.0f;
	}
    
	float* StartTime = ItemCooldownStartTimes.Find(ItemName);
	float* Duration = ItemCooldownDurations.Find(ItemName);
    
	if (!StartTime || !Duration || *Duration <= 0.0f)
	{
		return 1.0f;
	}
    
	float Elapsed = GetWorld()->GetTimeSeconds() - *StartTime;
	float Progress = FMath::Clamp(Elapsed / *Duration, 0.0f, 1.0f);
	
	return Progress;
}

int32 UT3InventoryComponent::GetMoney() const
{
	return Money;
}

int32 UT3InventoryComponent::SetMoney(int32 NewMoney)
{
	Money = NewMoney;
	
	OnMoneyUpdated.Broadcast(Money);
	return Money;
}

int32 UT3InventoryComponent::GetItemCountByItemID(const FName& ItemName)
{
	for (FInventorySlot& Item : Items)
	{
		if (Item.ItemID == ItemName)
		{
			return Item.ItemStack;
		}
	}
	return 0;
}

int32 UT3InventoryComponent::GetRuneCountByItemID(const FName& ItemName)
{
	for (FInventorySlot& RuneItem : RuneItems)
	{
		if (RuneItem.ItemID == ItemName)
		{
			return RuneItem.ItemStack;
		}
	}
	return 0;
}

void UT3InventoryComponent::AddItemByCount(const FName& ItemName, int32 Count)
{
	for (FInventorySlot& Item : Items)
	{
		if (Item.ItemID == ItemName)
		{
			Item.ItemStack += Count;
			
			UE_LOG(LogTemp, Log, TEXT("[%s] %d개 추가됨"), *Item.ItemID.ToString(), Count);
			
			if (EquippedItemIDs.Contains(ItemName))
			{
				OnEquippedItemChanged.Broadcast();
			}
			
			OnInventoryUpdated.Broadcast();
			return;
		}
	}
	
	for (FInventorySlot& Item : Items)
	{
		if (Item.ItemID == NAME_None)
		{
			Item.ItemID = ItemName;
			Item.ItemStack = Count;
			
			OnInventoryUpdated.Broadcast();
			return;
		}
	}
}

bool UT3InventoryComponent::RemoveItemByCount(const FName& ItemName, int32 Count)
{
	for (FInventorySlot& Item : Items)
	{
		if (Item.ItemID == ItemName)
		{
			if (Item.ItemStack < Count)
			{
				UE_LOG(LogTemp, Error, TEXT("보유한 아이템 개수보다 제거하는 소모품 아이템이 많음"));
				return false;
			}
			
			Item.ItemStack -= Count;
			
			if (Item.ItemStack <= 0)
			{
				Item.ItemID = NAME_None;
				Item.ItemStack = 0;
				
				if (EquippedItemIDs.Remove(ItemName) != 0)
				{
					OnEquippedItemChanged.Broadcast();
				}	
			}
			OnInventoryUpdated.Broadcast();
			return true;
		}
	}
	
	return false;
}

void UT3InventoryComponent::AddRuneItemByCount(const FName& ItemName, int32 Count)
{
	for (FInventorySlot& RuneItem : RuneItems)
	{
		if (RuneItem.ItemID == ItemName)
		{
			RuneItem.ItemStack += Count;
			
			UE_LOG(LogTemp, Log, TEXT("[%s] %d개 추가됨"), *RuneItem.ItemID.ToString(), Count);
			
			OnRuneInventoryUpdated.Broadcast();
			return;
		}
	}
	
	for (FInventorySlot& RuneItem : RuneItems)
	{
		if (RuneItem.ItemID == NAME_None)
		{
			RuneItem.ItemID = ItemName;
			RuneItem.ItemStack = Count;
			
			OnRuneInventoryUpdated.Broadcast();
			return;
		}
	}
}

bool UT3InventoryComponent::RemoveRuneItemByCount(const FName& ItemName, int32 Count)
{
	for (FInventorySlot& RuneItem : RuneItems)
	{
		if (RuneItem.ItemID == ItemName)
		{
			if (RuneItem.ItemStack < Count)
			{
				UE_LOG(LogTemp, Error, TEXT("보유한 아이템 개수보다 제거하는 룬 아이템이 많음"));
				return false;
			}
			
			RuneItem.ItemStack -= Count;
			
			if (RuneItem.ItemStack <= 0)
			{
				RuneItem.ItemID = NAME_None;
				RuneItem.ItemStack = 0;
			}
			OnRuneInventoryUpdated.Broadcast();
			return true;
		}
	}
	
	return false;
}

void UT3InventoryComponent::AddEtcItemByCount(const FName& ItemName, int32 Count)
{
	for (FInventorySlot& EtcItem : EtcItems)
	{
		if (EtcItem.ItemID == ItemName)
		{
			EtcItem.ItemStack += Count;
			
			UE_LOG(LogTemp, Log, TEXT("[%s] %d개 추가됨"), *EtcItem.ItemID.ToString(), Count);
			
			OnEtcInventoryUpdated.Broadcast();
			return;
		}
	}
	
	for (FInventorySlot& EtcItem : EtcItems)
	{
		if (EtcItem.ItemID == NAME_None)
		{
			EtcItem.ItemID = ItemName;
			EtcItem.ItemStack = Count;
			
			OnEtcInventoryUpdated.Broadcast();
			return;
		}
	}
}

bool UT3InventoryComponent::RemoveEtcItemByCount(const FName& ItemName, int32 Count)
{
	for (FInventorySlot& EtcItem : EtcItems)
	{
		if (EtcItem.ItemID == ItemName)
		{
			if (EtcItem.ItemStack < Count)
			{
				UE_LOG(LogTemp, Error, TEXT("보유한 아이템 개수보다 제거하는 기타 아이템이 많음"));
				return false;
			}
			
			EtcItem.ItemStack -= Count;
			
			if (EtcItem.ItemStack <= 0)
			{
				EtcItem.ItemID = NAME_None;
				EtcItem.ItemStack = 0;
			}
			OnEtcInventoryUpdated.Broadcast();
			
			UE_LOG(LogTemp, Error, TEXT("[%s] 제거 완료"), *EtcItem.ItemID.ToString());
			return true;
		}
	}
	
	UE_LOG(LogTemp, Error, TEXT("제거 할 아이템이 없음"));
	return false;
}

int32 UT3InventoryComponent::GetRuneItemCountByRuneID(const FName& RuneID)
{
	for (FInventorySlot& RuneItem : RuneItems)
	{
		if (RuneItem.ItemID == RuneID)
		{
			return RuneItem.ItemStack;
		}
	}
	return 0;
}

void UT3InventoryComponent::AddAccessoryItemByCount(const FName& ItemName, int32 Count)
{
	for (FInventorySlot& AccessoryItem : AccessoryItems)
	{
		if (AccessoryItem.ItemID == ItemName)
		{
			AccessoryItem.ItemStack += Count;

			UE_LOG(LogTemp, Log, TEXT("[%s] %d개 추가됨"), *AccessoryItem.ItemID.ToString(), Count);

			OnAccessoryInventoryUpdated.Broadcast();
			return;
		}
	}

	for (FInventorySlot& AccessoryItem : AccessoryItems)
	{
		if (AccessoryItem.ItemID == NAME_None)
		{
			AccessoryItem.ItemID = ItemName;
			AccessoryItem.ItemStack = Count;

			UE_LOG(LogTemp, Log, TEXT("[%s] %d개 추가됨"), *AccessoryItem.ItemID.ToString(), Count);
			
			OnAccessoryInventoryUpdated.Broadcast();
			return;
		}
	}
}

bool UT3InventoryComponent::RemoveAccessoryItemByCount(const FName& ItemName, int32 Count)
{
	for (FInventorySlot& AccessoryItem : AccessoryItems)
	{
		if (AccessoryItem.ItemID == ItemName)
		{
			if (AccessoryItem.ItemStack < Count)
			{
				UE_LOG(LogTemp, Error, TEXT("보유한 아이템 개수보다 제거하는 악세서리 아이템이 많음"));
				return false;
			}

			AccessoryItem.ItemStack -= Count;

			if (AccessoryItem.ItemStack <= 0)
			{
				AccessoryItem.ItemID = NAME_None;
				AccessoryItem.ItemStack = 0;
			}

			OnAccessoryInventoryUpdated.Broadcast();
			return true;
		}
	}

	UE_LOG(LogTemp, Error, TEXT("제거 할 악세서리 아이템이 없음"));
	return false;
}

void UT3InventoryComponent::SwapAccessorySlots(int32 SourceSlotIndex, int32 TargetSlotIndex)
{
	if (!AccessoryItems.IsValidIndex(SourceSlotIndex) || !AccessoryItems.IsValidIndex(TargetSlotIndex))
	{
		return;
	}

	if (SourceSlotIndex == TargetSlotIndex)
	{
		return;
	}

	FInventorySlot TempSlot = AccessoryItems[SourceSlotIndex];
	AccessoryItems[SourceSlotIndex] = AccessoryItems[TargetSlotIndex];
	AccessoryItems[TargetSlotIndex] = TempSlot;

	OnAccessoryInventoryUpdated.Broadcast();
}

int32 UT3InventoryComponent::GetAccessoryCountByItemID(const FName& ItemName)
{
	for (FInventorySlot& AccessoryItem : AccessoryItems)
	{
		if (AccessoryItem.ItemID == ItemName)
		{
			return AccessoryItem.ItemStack;
		}
	}
	return 0;
}

void UT3InventoryComponent::SetAccessoryLevel(const FName& ItemName, int32 Level)
{
	if (ItemName == NAME_None)
	{
		return;
	}

	AccessoryLevelMap.Add(ItemName, Level);
}

int32 UT3InventoryComponent::GetAccessoryLevel(const FName& ItemName) const
{
	if (ItemName == NAME_None)
	{
		return 0;
	}

	const int32* Found = AccessoryLevelMap.Find(ItemName);

	return Found ? *Found : 0;
}

// 무기 강화석
int32 UT3InventoryComponent::GetWeaponNormalStoneCount() const
{
	return WeaponNormalStoneCount;
}
void UT3InventoryComponent::SetWeaponNormalStoneCount(int32 NewCount)
{
	WeaponNormalStoneCount = NewCount;
}

int32 UT3InventoryComponent::GetWeaponEpicStoneCount() const
{
	return WeaponEpicStoneCount;
}
void UT3InventoryComponent::SetWeaponEpicStoneCount(int32 NewCount)
{
	WeaponEpicStoneCount = NewCount;
}

int32 UT3InventoryComponent::GetWeaponLegendaryStoneCount() const
{
	return WeaponLegendaryStoneCount;
}
void UT3InventoryComponent::SetWeaponLegendaryStoneCount(int32 NewCount)
{
	WeaponLegendaryStoneCount = NewCount;
}

// 방어구 강화석
int32 UT3InventoryComponent::GetArmorNormalStoneCount() const
{
	return ArmorNormalStoneCount;
}
void UT3InventoryComponent::SetArmorNormalStoneCount(int32 NewCount)
{
	ArmorNormalStoneCount = NewCount;
}

int32 UT3InventoryComponent::GetArmorEpicStoneCount() const
{
	return ArmorEpicStoneCount;
}
void UT3InventoryComponent::SetArmorEpicStoneCount(int32 NewCount)
{
	ArmorEpicStoneCount = NewCount;
}

int32 UT3InventoryComponent::GetArmorLegendaryStoneCount() const
{
	return ArmorLegendaryStoneCount;
}
void UT3InventoryComponent::SetArmorLegendaryStoneCount(int32 NewCount)
{
	ArmorLegendaryStoneCount = NewCount;
}

void UT3InventoryComponent::UpdateCooldowns()
{
	bool bHasActiveCooldowns = false;
	TArray<FName> ItemsToRemove; // 제거할 항목들을 저장할 배열
    
	for (auto& Pair : ItemCooldownStartTimes)
	{
		FName ItemID = Pair.Key;
		float* Duration = ItemCooldownDurations.Find(ItemID);
        
		if (!Duration)
		{
			continue;
		}
        
		float Elapsed = GetWorld()->GetTimeSeconds() - Pair.Value;
		float Remaining = FMath::Max(0.0f, *Duration - Elapsed);
        
		float Progress = FMath::Clamp(Elapsed / *Duration, 0.0f, 1.0f);
		
		OnCooldownUpdated.Broadcast(ItemID, Remaining);
		OnCooldownProgressUpdated.Broadcast(ItemID, Progress);
        
		if (Remaining > 0.0f)
		{
			bHasActiveCooldowns = true;
		}
		else
		{
			ItemsToRemove.Add(ItemID); // 제거 목록에 추가만 함
		}
	}
	
	// 순회가 끝난 후 제거
	for (FName ItemID : ItemsToRemove)
	{
		ItemCooldownStartTimes.Remove(ItemID);
		ItemCooldownDurations.Remove(ItemID);
	}
    
	if (!bHasActiveCooldowns)
	{
		GetWorld()->GetTimerManager().ClearTimer(CooldownUpdateTimerHandle);
	}
}

void UT3InventoryComponent::ToggleEquipItem(const FName& ItemName)
{
	if (ItemName == NAME_None)
	{
		return;
	}
	
	if (EquippedItemIDs.Contains(ItemName))
	{
		UnequipItem(ItemName);
		
		OnEquippedItemChanged.Broadcast();
		OnInventoryUpdated.Broadcast();
		return;
	}
	
	EquippedItemIDs.Emplace(ItemName);
	
	OnEquippedItemChanged.Broadcast();
	OnInventoryUpdated.Broadcast();
}

void UT3InventoryComponent::UnequipItem(const FName& ItemName)
{
	if (ItemName == NAME_None)
	{
		return;
	}

	EquippedItemIDs.Remove(ItemName);
}

int32 UT3InventoryComponent::GetEquippedItemIndex(const FName& ItemName) const
{
	return EquippedItemIDs.IndexOfByKey(ItemName);
}

bool UT3InventoryComponent::IsItemEquipped(const FName& ItemName) const
{
	return EquippedItemIDs.Contains(ItemName);
}

void UT3InventoryComponent::SwapEquippedItem()
{
	if (EquippedItemIDs.Num() <= 1)
	{
		return;
	}
	
	FName TempName = EquippedItemIDs[0];
	
	EquippedItemIDs.Remove(TempName);
	EquippedItemIDs.Emplace(TempName);

	OnChangedBuffItemSlot.Broadcast();
	OnInventoryUpdated.Broadcast();
}

void UT3InventoryComponent::UseEquippedItem()
{
	if (EquippedItemIDs.Num() <= 0)
	{
		UE_LOG(LogTemp, Error, TEXT("장착한 아이템이 없음"));
		return;
	}
	
	if (!IsValid(OwnerCharacter) || !IsValid(OwnerCharacter->ItemDataTable))
	{
		UE_LOG(LogTemp, Error, TEXT("캐릭터 또는 아이템 데이터테이블이 유효하지않음"));
		return;
	}
	
	FName ItemIDToUse = EquippedItemIDs[0];
	
	FT3ConsumableItemData* ItemRow =
		OwnerCharacter->ItemDataTable->FindRow<FT3ConsumableItemData>(ItemIDToUse, TEXT("UseEquipped"));
	
	if (!ItemRow)
	{
		return;
	}
	
	if (!OwnerCharacter->ItemUseComponent->ApplyConsumableItem(*ItemRow))
	{
		return;
	}
	
	ItemCooldownStartTimes.Emplace(ItemIDToUse, GetWorld()->GetTimeSeconds());
	ItemCooldownDurations.Emplace(ItemIDToUse, ItemRow->CoolTime);
	
	if (!GetWorld()->GetTimerManager().IsTimerActive(CooldownUpdateTimerHandle))
	{
		GetWorld()->GetTimerManager().SetTimer(
			CooldownUpdateTimerHandle,
			this,
			&UT3InventoryComponent::UpdateCooldowns,
			0.1f,
			true);
	}
	
	RemoveItem(ItemIDToUse);
	
	OnBuffItemUsed.Broadcast();
}

int32 UT3InventoryComponent::GetCurrentBuffItemCount() const
{
	if (EquippedItemIDs.Num() <= 0)
	{
		return 0;
	}
	
	FName CurrentItemName = EquippedItemIDs[0];
	
	if (CurrentItemName == NAME_None)
	{
		return 0;
	}
	
	for (const FInventorySlot& Item : Items)
	{
		if (Item.ItemID == CurrentItemName)
		{
			return Item.ItemStack;
		}
	}
	
	return 0;
}

int32 UT3InventoryComponent::GetNextBuffItemCount() const
{
	if (EquippedItemIDs.Num() <= 1)
	{
		return 0;
	}
	
	FName NextItemName = EquippedItemIDs[1];
	
	if (NextItemName == NAME_None)
	{
		return 0;
	}
	
	for (const FInventorySlot& Item : Items)
	{
		if (Item.ItemID == NextItemName)
		{
			return Item.ItemStack;
		}
	}
	
	return 0;
}

FName UT3InventoryComponent::GetCurrentBuffItemName() const
{
	if (EquippedItemIDs.Num() <= 0)
	{
		return NAME_None;
	}
	
	return EquippedItemIDs[0];
}

FName UT3InventoryComponent::GetNextBuffItemName() const
{
	if (EquippedItemIDs.Num() <= 1)
	{
		return NAME_None;
	}
	
	return EquippedItemIDs[1];
}

void UT3InventoryComponent::InitializePotionIDs()
{
	if (!IsValid(OwnerCharacter) || !IsValid(OwnerCharacter->ItemDataTable))
	{
		UE_LOG(LogTemp, Error, TEXT("캐릭터 또는 아이템 데이터테이블이 유효하지않음"));
		return;
	}
	
	TArray<FName> RowNames = OwnerCharacter->ItemDataTable->GetRowNames();
	
	for (const FName& RowName : RowNames)
	{
		FT3ConsumableItemData* ItemRow =
			OwnerCharacter->ItemDataTable->FindRow<FT3ConsumableItemData>(RowName, TEXT("InitializePotionIDs"));
		
		if (!ItemRow)
		{
			continue;
		}
		
		if (ItemRow->EffectType == EEffectType::HP)
		{
			HPPotionID = RowName;
		}
		else if (ItemRow->EffectType == EEffectType::MP)
		{
			MPPotionID = RowName;
		}
		
		if (HPPotionID != NAME_None && MPPotionID != NAME_None)
		{
			break;
		}
	}
	
	if (CurrentPotionID == NAME_None && HPPotionID != NAME_None)
	{
		CurrentPotionID = HPPotionID;
	}
}

void UT3InventoryComponent::SetHPPotionCount(int32 Count)
{
	HPPotionCount = FMath::Clamp(Count, 0, GetMaxHPPotionCount());
	OnRecoverItemUsed.Broadcast();
}

void UT3InventoryComponent::SetMPPotionCount(int32 Count)
{
	MPPotionCount = FMath::Clamp(Count, 0, GetMaxMPPotionCount());
	OnRecoverItemUsed.Broadcast();
}

void UT3InventoryComponent::UseCurrentPotion()
{
	if (CurrentPotionID == NAME_None)
	{
		return;
	}
	
	if (CurrentPotionID == HPPotionID)
	{
		UseHPPotion();
	}
	else if (CurrentPotionID == MPPotionID)
	{
		UseMPPotion();
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("회복 포션 사용 실패"));
		return;
	}
}

void UT3InventoryComponent::SwapHPMPSlot()
{
	if (CurrentPotionID == NAME_None)
	{
		return;
	}
	
	if (CurrentPotionID == HPPotionID)
	{
		CurrentPotionID = MPPotionID;
	}
	else if (CurrentPotionID == MPPotionID)
	{
		CurrentPotionID = HPPotionID;
	}
	
	OnSwapRecoverSlot.Broadcast();
}

FName UT3InventoryComponent::GetCurrentPotionID() const
{
	return CurrentPotionID;
}

FName UT3InventoryComponent::GetNextPotionID() const
{
	if (CurrentPotionID == NAME_None)
	{
		return NAME_None;
	}
	
	if (CurrentPotionID == HPPotionID)
	{
		return MPPotionID;
	}
	else if (CurrentPotionID == MPPotionID)
	{
		return HPPotionID;
	}
	
	return NAME_None;
}

int32 UT3InventoryComponent::GetHPPotionCount() const
{
	return HPPotionCount;
}

int32 UT3InventoryComponent::GetMPPotionCount() const
{
	return MPPotionCount;
}

int32 UT3InventoryComponent::GetCurrentPotionCount() const
{
	if (CurrentPotionID == NAME_None)
	{
		return 0;
	}
	
	if (CurrentPotionID == HPPotionID)
	{
		return HPPotionCount;
	}
	else if (CurrentPotionID == MPPotionID)
	{
		return MPPotionCount;
	}
	
	return 0;
}

void UT3InventoryComponent::UseHPPotion()
{
	if (!IsValid(OwnerCharacter) || !IsValid(OwnerCharacter->ItemDataTable))
	{
		UE_LOG(LogTemp, Error, TEXT("캐릭터 또는 아이템 데이터테이블이 유효하지않음"));
		return;
	}
	
	if (HPPotionID == NAME_None || HPPotionCount <= 0)
	{
		UE_LOG(LogTemp, Error, TEXT("HPPotion 이름이 비었거나 0개 이하"));
		return;
	}
	
	FT3ConsumableItemData* ItemRow =
		OwnerCharacter->ItemDataTable->FindRow<FT3ConsumableItemData>(HPPotionID, TEXT("UseHPPotion"));
	
	if (!ItemRow)
	{
		return;
	}
	
	if (!OwnerCharacter->ItemUseComponent->ApplyConsumableItem(*ItemRow, GetPotionRecoveryBonus()))
	{
		return;
	}
	
	ItemCooldownStartTimes.Emplace(HPPotionID, GetWorld()->GetTimeSeconds());
	ItemCooldownDurations.Emplace(HPPotionID, ItemRow->CoolTime);
	
	if (!GetWorld()->GetTimerManager().IsTimerActive(CooldownUpdateTimerHandle))
	{
		GetWorld()->GetTimerManager().SetTimer(
			CooldownUpdateTimerHandle,
			this,
			&UT3InventoryComponent::UpdateCooldowns,
			0.1f,
			true);
	}
	
	HPPotionCount--;

	OnRecoverItemUsed.Broadcast();
}

void UT3InventoryComponent::UseMPPotion()
{
	if (!IsValid(OwnerCharacter) || !IsValid(OwnerCharacter->ItemDataTable))
	{
		UE_LOG(LogTemp, Error, TEXT("캐릭터 또는 아이템 데이터테이블이 유효하지않음"));
		return;
	}
	
	if (MPPotionID == NAME_None || MPPotionCount <= 0)
	{
		UE_LOG(LogTemp, Error, TEXT("MPPotion 이름이 비었거나 0개 이하"));
		return;
	}
	
	FT3ConsumableItemData* ItemRow =
		OwnerCharacter->ItemDataTable->FindRow<FT3ConsumableItemData>(MPPotionID, TEXT("UseMPPotion"));
	
	if (!ItemRow)
	{
		return;
	}
	
	if (!OwnerCharacter->ItemUseComponent->ApplyConsumableItem(*ItemRow, GetPotionRecoveryBonus()))
	{
		return;
	}
	
	ItemCooldownStartTimes.Emplace(MPPotionID, GetWorld()->GetTimeSeconds());
	ItemCooldownDurations.Emplace(MPPotionID, ItemRow->CoolTime);
	
	if (!GetWorld()->GetTimerManager().IsTimerActive(CooldownUpdateTimerHandle))
	{
		GetWorld()->GetTimerManager().SetTimer(
			CooldownUpdateTimerHandle,
			this,
			&UT3InventoryComponent::UpdateCooldowns,
			0.1f,
			true);
	}
	
	MPPotionCount--;
	
	OnRecoverItemUsed.Broadcast();
}

bool UT3InventoryComponent::IsRuneEquipped(const FName& ItemName)
{
	return false;
}

void UT3InventoryComponent::UpgradePotionAmount()
{
	PotionAmountUpgradeLevel++;
	
	OnPotionUpgraded.Broadcast();
}

void UT3InventoryComponent::UpgradePotionRecovery()
{
	PotionRecoveryUpgradeLevel++;
	
	OnPotionUpgraded.Broadcast();
}

int32 UT3InventoryComponent::GetMaxHPPotionCount() const
{
	return InitialHPPotionAmount + PotionAmountUpgradeLevel;
}

int32 UT3InventoryComponent::GetMaxMPPotionCount() const
{
	return InitialMPPotionAmount + PotionAmountUpgradeLevel;
}

int32 UT3InventoryComponent::GetPotionRecoveryBonus() const
{
	return PotionRecoveryUpgradeLevel * 10;
}

int32 UT3InventoryComponent::GetCurrentHPPotionRecovery() const
{
	if (!IsValid(OwnerCharacter) || !IsValid(OwnerCharacter->ItemDataTable))
	{
		return -1;
	}
	
	FT3ConsumableItemData* ItemRow =
		OwnerCharacter->ItemDataTable->FindRow<FT3ConsumableItemData>(HPPotionID, TEXT("GetCurrentHPPotionRecovery"));
	
	return ItemRow->BuffValue + GetPotionRecoveryBonus();
}

int32 UT3InventoryComponent::GetCurrentMPPotionRecovery() const
{
	if (!IsValid(OwnerCharacter) || !IsValid(OwnerCharacter->ItemDataTable))
	{
		return -1;
	}
	
	FT3ConsumableItemData* ItemRow =
		OwnerCharacter->ItemDataTable->FindRow<FT3ConsumableItemData>(MPPotionID, TEXT("GetCurrentMPPotionRecovery"));
	
	return ItemRow->BuffValue + GetPotionRecoveryBonus();
}

int32 UT3InventoryComponent::GetPotionAmountUpgradeLevel() const
{
	return PotionAmountUpgradeLevel;
}

int32 UT3InventoryComponent::GetPotionRecoveryUpgradeLevel() const
{
	return PotionRecoveryUpgradeLevel;
}

void UT3InventoryComponent::LoadPotionUpgradeLevel(int32 AmountLevel, int32 RecoveryLevel)
{
	PotionAmountUpgradeLevel = AmountLevel;
	PotionRecoveryUpgradeLevel = RecoveryLevel;
	
	OnPotionUpgraded.Broadcast();
}
