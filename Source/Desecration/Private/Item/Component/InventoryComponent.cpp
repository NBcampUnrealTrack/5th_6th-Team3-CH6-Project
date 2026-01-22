#include "Public/Item/Component/InventoryComponent.h"

#include "Player/T3CharacterBase.h"
#include "Public/Item/Data/T3ComsumableItemData.h"
UInventoryComponent::UInventoryComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UInventoryComponent::BeginPlay()
{
	Super::BeginPlay();
	
	Items.SetNum(InventorySize);
}

void UInventoryComponent::AddItem(FName ItemName)
{
	if (ItemName == NAME_None)
	{
		return;
	}

	AT3CharacterBase* OwnerCharacter = Cast<AT3CharacterBase>(GetOwner());
	
	if (!IsValid(OwnerCharacter))
	{
		return;
	}
	
	UDataTable* ItemDataTable = OwnerCharacter->ItemDataTable;
	
	if (!IsValid(ItemDataTable))
	{
		return;
	}

	FT3ComsumableItemData* ItemRow = ItemDataTable->FindRow<FT3ComsumableItemData>(ItemName, TEXT("AddItem"));

	if (!ItemRow) // 데이터 테이블에 없는 아이템을 넣으면 출력됨
	{
		if (GEngine)
		{
			FString Msg = FString::Printf(TEXT("[%s]는 존재하지 않는 아이템"), *ItemName.ToString());
			GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Red, Msg);
		}
		return;
	}

	for (int32 i = 0; i < Items.Num(); i++)
	{
		if (Items[i].ItemID == ItemName)
		{
			Items[i].ItemStack++;
			OnInventoryUpdated.Broadcast();
			return;
		}

		if (Items[i].ItemID == NAME_None)
		{
			Items[i].ItemID = ItemName;
			Items[i].ItemStack = 1;
			OnInventoryUpdated.Broadcast();
			return;
		}
	}

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Red, TEXT("인벤토리 꽉 참"));
	}
}

void UInventoryComponent::UseItem(int32 SlotIndex)
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
	
	AT3CharacterBase* OwnerCharacter = Cast<AT3CharacterBase>(GetOwner());
	
	if (!IsValid(OwnerCharacter) || !IsValid(OwnerCharacter->ItemDataTable))
	{
		return;
	}
	
	FT3ComsumableItemData* ItemRow =
		OwnerCharacter->ItemDataTable->FindRow<FT3ComsumableItemData>(ItemIDToUse, TEXT("UseItem"));
	
	if (!ItemRow)
	{
		return;
	}
	
	// bool bUsed = OwnerCharacter->ApplyConsumableItem(*ItemRow);
	//
	// if (!bUsed)
	// {
	// 	return;
	// }
	
	Items[SlotIndex].ItemStack--;

	if (Items[SlotIndex].ItemStack <= 0)
	{
		Items[SlotIndex].ItemID = NAME_None;
		Items[SlotIndex].ItemStack = 0;
	}

	OnInventoryUpdated.Broadcast();
}

void UInventoryComponent::DropItem(int32 SlotIndex)
{
}