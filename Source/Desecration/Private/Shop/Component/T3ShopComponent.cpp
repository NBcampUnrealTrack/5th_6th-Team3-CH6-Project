#include "Shop/Component/T3ShopComponent.h"

#include "Item/Component/T3InventoryComponent.h"
#include "Player/T3PlayerController.h"
#include "Shop/Data/T3ShopData.h"

UT3ShopComponent::UT3ShopComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

}

EShopBuyResult UT3ShopComponent::BuyItem(const FName& ItemName, UT3InventoryComponent* Inventory)
{
	if (!IsValid(ShopData) || !IsValid(Inventory))
	{
		return EShopBuyResult::InvalidData;
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
	
	if (Inventory->GetMoney() < ItemRow->BuyPrice)
	{
		UE_LOG(LogTemp, Error, TEXT("아이템을 구매할 돈이 없습니다."));
		return EShopBuyResult::NotEnoughMoney;
	}
	
	Inventory->AddItem(ItemName);
	Inventory->SetMoney(Inventory->GetMoney() - ItemRow->BuyPrice);
	
	return EShopBuyResult::Succeeded;
}

EShopSellResult UT3ShopComponent::SellItem(const FName& ItemName, UT3InventoryComponent* Inventory)
{
	if (!IsValid(ShopData) || !IsValid(Inventory))
	{
		UE_LOG(LogTemp, Error, TEXT("데이터 또는 인벤토리가 유효하지않음"));
		return EShopSellResult::InvalidData;
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
	
	if (!Inventory->RemoveItem(ItemName))
	{
		UE_LOG(LogTemp, Error, TEXT("인벤토리에 [%s]이 없음"), *ItemName.ToString());
		return EShopSellResult::ItemNotFound;
	}
	
	Inventory->SetMoney(Inventory->GetMoney() + ItemRow->SellPrice);
	
	return EShopSellResult::Succeeded;
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
