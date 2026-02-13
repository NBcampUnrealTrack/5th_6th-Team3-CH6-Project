#include "Public/Item/Component/T3InventoryComponent.h"

#include "IDetailTreeNode.h"
#include "Item/Component/T3ItemUseComponent.h"
#include "Player/T3CharacterBase.h"
#include "Public/Item/Data/T3ConsumableItemData.h"
UT3InventoryComponent::UT3InventoryComponent()
	:
InventorySize(20),
Money(0),
NormalStoneCount(0),
EpicStoneCount(0),
LegendaryStoneCount(0)
{
	PrimaryComponentTick.bCanEverTick = false;
	
	Items.SetNum(InventorySize);
}

void UT3InventoryComponent::BeginPlay()
{
	Super::BeginPlay();
	
	OwnerCharacter = Cast<AT3CharacterBase>(GetOwner());
	
	InitializePotionIDs();
	SetHPPotionCount(3);
	SetMPPotionCount(3);
	
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

void UT3InventoryComponent::DropItem(int32 SlotIndex)
{
}

bool UT3InventoryComponent::RemoveItem(const FName& ItemName)
{
	for (int32 i = 0; i < Items.Num(); i++)
	{
		if (Items[i].ItemID == ItemName)
		{
			Items[i].ItemStack--;
			
			if (Items[i].ItemStack <= 0)
			{
				Items[i].ItemID = NAME_None;
				Items[i].ItemStack = 0;
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
	
	return Money;
}

int32 UT3InventoryComponent::GetNormalStoneCount() const
{
	return NormalStoneCount;
}

int32 UT3InventoryComponent::GetEpicStoneCount() const
{
	return EpicStoneCount;
}

int32 UT3InventoryComponent::GetLegendaryStoneCount() const
{
	return LegendaryStoneCount;
}

int32 UT3InventoryComponent::SetNormalStoneCount(int32 NewCount)
{
	NormalStoneCount = NewCount;
	return NormalStoneCount;
}

int32 UT3InventoryComponent::SetEpicStoneCount(int32 NewCount)
{
	EpicStoneCount = NewCount;
	return EpicStoneCount;
}

int32 UT3InventoryComponent::SetLegendaryStoneCount(int32 NewCount)
{
	LegendaryStoneCount = NewCount;
	return LegendaryStoneCount;
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
		return;
	}
	
	EquippedItemIDs.Emplace(ItemName);
	
	OnToggleItemEquipped.Broadcast(ItemName);
	OnInventoryUpdated.Broadcast();
}

void UT3InventoryComponent::UnequipItem(const FName& ItemName)
{
	if (ItemName == NAME_None)
	{
		return;
	}
	
	if (!EquippedItemIDs.Contains(ItemName))
	{
		return;
	}
	
	int32 Index = GetEquippedItemIndex(ItemName);
	
	if (Index != INDEX_NONE)
	{
		EquippedItemIDs.RemoveAt(Index);
		
		OnToggleItemEquipped.Broadcast(ItemName);
		OnInventoryUpdated.Broadcast();
	}
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
	
	EquippedItemIDs.RemoveAt(0);
	EquippedItemIDs.Emplace(TempName);

	OnToggleItemEquipped.Broadcast(TempName);
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
	
	for (FInventorySlot& Item : Items)
	{
		if (Item.ItemID == ItemIDToUse)
		{
			Item.ItemStack--;
			
			UE_LOG(LogTemp, Log, TEXT("[%s]를 1개 사용했습니다. 현재 개수: %d"), *Item.ItemID.ToString(), Item.ItemStack)

			if (Item.ItemStack <= 0)
			{
				UE_LOG(LogTemp, Log, TEXT("[%s]를 모두 사용했습니다."), *Item.ItemID.ToString())
		
				Item.ItemID = NAME_None;
				Item.ItemStack = 0;
			}
			
			OnInventoryUpdated.Broadcast();
			
			break;
		}
	}
	
	ConsumableItemType = EConsumableItemType::Buff;
	
	FString ConsumableTypeString = StaticEnum<EConsumableItemType>()->GetNameStringByValue(static_cast<int64>(ConsumableItemType));
	UE_LOG(LogTemp, Log, TEXT("ConsumableItemType 설정: %s"), *ConsumableTypeString);
}

int32 UT3InventoryComponent::GetCurrentBuffItemCount() const
{
	if (EquippedItemIDs.Num() <= 0)
	{
		UE_LOG(LogTemp, Error, TEXT("장착한 아이템이 없음"));
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
		UE_LOG(LogTemp, Error, TEXT("장착한 아이템이 없음"));
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
		UE_LOG(LogTemp, Error, TEXT("장착된 버프 아이템이 없습니다"))
		return NAME_None;
	}
	
	return EquippedItemIDs[0];
}

FName UT3InventoryComponent::GetNextBuffItemName() const
{
	if (EquippedItemIDs.Num() <= 1)
	{
		UE_LOG(LogTemp, Error, TEXT("장착된 버프 아이템이 1개 입니다"))
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
	HPPotionCount = Count;
}

void UT3InventoryComponent::SetMPPotionCount(int32 Count)
{
	MPPotionCount = Count;
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
	
	ConsumableItemType = EConsumableItemType::Recover;
	
	FString ConsumableTypeString = StaticEnum<EConsumableItemType>()->GetNameStringByValue(static_cast<int64>(ConsumableItemType));
	UE_LOG(LogTemp, Log, TEXT("ConsumableItemType 설정: %s"), *ConsumableTypeString);
}

void UT3InventoryComponent::SwapHPMPSlot()
{
	if (CurrentPotionID == NAME_None || OwnerCharacter->bIsUsingItem)
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
	
	if (!OwnerCharacter->ItemUseComponent->ApplyConsumableItem(*ItemRow))
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
	
	OnInventoryUpdated.Broadcast();
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
	
	if (!OwnerCharacter->ItemUseComponent->ApplyConsumableItem(*ItemRow))
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
	
	OnInventoryUpdated.Broadcast();
}
