#include "Shop/Component/T3ShopComponent.h"

#include "Item/Component/T3InventoryComponent.h"
#include "Item/Data/T3ItemBaseData.h"
#include "Player/T3PlayerController.h"
#include "Shop/Data/T3ShopData.h"

UT3ShopComponent::UT3ShopComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

}

EShopBuyResult UT3ShopComponent::BuyItem(EItemType ItemType, const FName& ItemName, UT3InventoryComponent* Inventory, int32 Count)
{
	switch (ItemType)
	{
	case EItemType::Consumable:
		{
			if (!IsValid(ShopData) || !IsValid(Inventory))
			{
				return EShopBuyResult::InvalidData;
			}
	
			if (Count <= 0)
			{
				UE_LOG(LogTemp, Error, TEXT("사는 개수가 0개임"));
				return EShopBuyResult::ZeroCount;
			}
	
			FT3ShopData* ItemRow = ShopData->FindRow<FT3ShopData>(ItemName, TEXT("BuyItem"));
	
			if (!ItemRow)
			{
				return EShopBuyResult::ItemNotFound;
			}
	
			if (!ItemRow->bCanBuy)
			{
				UE_LOG(LogTemp, Error, TEXT("아이템 구매 조건을 충족하지 않았습니다."));
				return EShopBuyResult::CannotBuy;
			}
	
			if (Inventory->GetMoney() < ItemRow->BuyPrice * Count)
			{
				UE_LOG(LogTemp, Error, TEXT("아이템을 구매할 돈이 없습니다."));
				return EShopBuyResult::NotEnoughMoney;
			}
	
			Inventory->AddItemByCount(ItemName, Count);
			Inventory->SetMoney(Inventory->GetMoney() - (ItemRow->BuyPrice * Count));
	
			return EShopBuyResult::Succeeded;
		}
	
	case EItemType::Rune:
		{
			if (!IsValid(ShopRuneData) || !IsValid(Inventory))
			{
				return EShopBuyResult::InvalidData;
			}
	
			if (Count <= 0)
			{
				UE_LOG(LogTemp, Error, TEXT("사는 개수가 0개임"));
				return EShopBuyResult::ZeroCount;
			}
	
			FT3ShopData* ItemRow = ShopRuneData->FindRow<FT3ShopData>(ItemName, TEXT("BuyItem"));
	
			if (!ItemRow)
			{
				return EShopBuyResult::ItemNotFound;
			}
	
			if (!ItemRow->bCanBuy)
			{
				UE_LOG(LogTemp, Error, TEXT("아이템 구매 조건을 충족하지 않았습니다."));
				return EShopBuyResult::CannotBuy;
			}
	
			if (Inventory->GetMoney() < ItemRow->BuyPrice * Count)
			{
				UE_LOG(LogTemp, Error, TEXT("아이템을 구매할 돈이 없습니다."));
				return EShopBuyResult::NotEnoughMoney;
			}
	
			Inventory->AddRuneItemByCount(ItemName, Count);
			Inventory->SetMoney(Inventory->GetMoney() - (ItemRow->BuyPrice * Count));
	
			return EShopBuyResult::Succeeded;
		}
	
	default:
		return EShopBuyResult::None;
	}
}

EShopSellResult UT3ShopComponent::SellItem(EItemType ItemType, const FName& ItemName, UT3InventoryComponent* Inventory, int32 Count)
{
	switch (ItemType)
	{
	case EItemType::Consumable:
		{
			if (!IsValid(ShopData) || !IsValid(Inventory))
			{
				UE_LOG(LogTemp, Error, TEXT("데이터 또는 인벤토리가 유효하지않음"));
				return EShopSellResult::InvalidData;
			}

			if (Count <= 0)
			{
				UE_LOG(LogTemp, Error, TEXT("파는 개수가 0개임"));
				return EShopSellResult::ZeroCount;
			}

			FT3ShopData* ItemRow = ShopData->FindRow<FT3ShopData>(ItemName, TEXT("BuyItem"));

			if (!ItemRow)
			{
				UE_LOG(LogTemp, Error, TEXT("데이터가 유효하지않음"));
				return EShopSellResult::InvalidData;
			}

			if (!ItemRow->bCanSell)
			{
				UE_LOG(LogTemp, Error, TEXT("[%s]은 팔 수 없는 아이템"), *ItemName.ToString());
				return EShopSellResult::CannotSell;
			}

			if (!Inventory->RemoveItemByCount(ItemName, Count))
			{
				UE_LOG(LogTemp, Error, TEXT("인벤토리에 [%s]이 충분하지 않음"), *ItemName.ToString());
				return EShopSellResult::NotEnoughCount;
			}

			Inventory->SetMoney(Inventory->GetMoney() + (ItemRow->SellPrice * Count));

			return EShopSellResult::Succeeded;
		}
	
	case EItemType::Rune:
		{
			if (!IsValid(ShopRuneData) || !IsValid(Inventory))
			{
				UE_LOG(LogTemp, Error, TEXT("데이터 또는 인벤토리가 유효하지않음"));
				return EShopSellResult::InvalidData;
			}

			if (Count <= 0)
			{
				UE_LOG(LogTemp, Error, TEXT("파는 개수가 0개임"));
				return EShopSellResult::ZeroCount;
			}

			FT3ShopData* ItemRow = ShopRuneData->FindRow<FT3ShopData>(ItemName, TEXT("BuyItem"));

			if (!ItemRow)
			{
				UE_LOG(LogTemp, Error, TEXT("데이터가 유효하지않음"));
				return EShopSellResult::InvalidData;
			}

			if (!ItemRow->bCanSell)
			{
				UE_LOG(LogTemp, Error, TEXT("[%s]은 팔 수 없는 아이템"), *ItemName.ToString());
				return EShopSellResult::CannotSell;
			}

			if (!Inventory->RemoveRuneItemByCount(ItemName, Count))
			{
				UE_LOG(LogTemp, Error, TEXT("인벤토리에 [%s]이 충분하지 않음"), *ItemName.ToString());
				return EShopSellResult::NotEnoughCount;
			}

			Inventory->SetMoney(Inventory->GetMoney() + (ItemRow->SellPrice * Count));

			return EShopSellResult::Succeeded;
		}
	default:
		return EShopSellResult::None;
	}
	
}

void UT3ShopComponent::GetShopItemUIData(TArray<FT3ShopItemUIData>& OutItems) const
{
	if (!IsValid(ShopData))
	{
		return;
	}

	TArray<FName> RowNames = ShopData->GetRowNames();

	for (const FName& RowName : RowNames)
	{
		FT3ShopData* Row = ShopData->FindRow<FT3ShopData>(RowName, TEXT("Shop"));

		if (!Row)
		{
			continue;
		}

		FT3ShopItemUIData UIData;
		
		UIData.ItemID = RowName;
		UIData.Name = Row->Name;
		UIData.Icon = Row->Icon;
		UIData.BuyPrice = Row->BuyPrice;
		UIData.SellPrice = Row->SellPrice;
		UIData.bCanBuy = Row->bCanBuy;
		UIData.bCanSell = Row->bCanSell;
		UIData.ItemInfo = Row->ItemInfo;
		
		OutItems.Add(UIData);
	}
}

void UT3ShopComponent::GetShopRuneUIData(TArray<FT3ShopRuneUIData>& OutItems) const
{
	if (!IsValid(ShopRuneData))
	{
		return;
	}

	TArray<FName> RowNames = ShopRuneData->GetRowNames();

	for (const FName& RowName : RowNames)
	{
		FT3ShopData* Row = ShopRuneData->FindRow<FT3ShopData>(RowName, TEXT("Shop"));

		if (!Row)
		{
			continue;
		}

		FT3ShopRuneUIData UIData;
		
		UIData.ItemID = RowName;
		UIData.Name = Row->Name;
		UIData.Icon = Row->Icon;
		UIData.BuyPrice = Row->BuyPrice;
		UIData.SellPrice = Row->SellPrice;
		UIData.bCanBuy = Row->bCanBuy;
		UIData.bCanSell = Row->bCanSell;
		UIData.ItemInfo = Row->ItemInfo;
		UIData.CharacterClass = Row->CharacterClass;
		
		OutItems.Add(UIData);
	}
}

void UT3ShopComponent::OpenShop(AT3PlayerController* T3PC)
{
	if (!T3PC)
	{
		return;
	}
	
	T3PC->ShowShopUI(this);
}
