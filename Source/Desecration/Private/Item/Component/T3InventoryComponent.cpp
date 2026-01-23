#include "Public/Item/Component/T3InventoryComponent.h"

#include "Item/Component/T3ItemUseComponent.h"
#include "Player/T3CharacterBase.h"
#include "Public/Item/Data/T3ConsumableItemData.h"
UT3InventoryComponent::UT3InventoryComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UT3InventoryComponent::BeginPlay()
{
	Super::BeginPlay();
	
	Items.SetNum(InventorySize);
	
	OwnerCharacter = Cast<AT3CharacterBase>(GetOwner());
}

void UT3InventoryComponent::AddItem(FName ItemName)
{
	if (ItemName == NAME_None)
	{
		return;
	}

	if (!IsValid(OwnerCharacter))
	{
		return;
	}
	
	UDataTable* ItemDataTable = OwnerCharacter->ItemDataTable;
	
	if (!IsValid(ItemDataTable))
	{
		return;
	}

	FT3ConsumableItemData* ItemRow = ItemDataTable->FindRow<FT3ConsumableItemData>(ItemName, TEXT("AddItem"));

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
			
			UE_LOG(LogTemp, Log, TEXT("[%s] 1개 추가, 현재 개수: %d"), *ItemName.ToString(), Items[i].ItemStack)
			
			OnInventoryUpdated.Broadcast();
			return;
		}

		if (Items[i].ItemID == NAME_None)
		{
			Items[i].ItemID = ItemName;
			Items[i].ItemStack = 1;
			
			UE_LOG(LogTemp, Log, TEXT("새로운 아이템 [%s] 획득, 현재 개수: %d"), *ItemName.ToString(), Items[i].ItemStack)

			OnInventoryUpdated.Broadcast();
			return;
		}
	}

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Red, TEXT("인벤토리 꽉 참"));
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
	
	// bool bUsed = OwnerCharacter->ItemUseComponent->ApplyConsumableItem(*ItemRow);
	//
	// if (!bUsed)
	// {
	// 	return;
	// }
	
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

void UT3InventoryComponent::DropItem(int32 SlotIndex)
{
}