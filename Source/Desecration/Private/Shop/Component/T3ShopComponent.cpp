#include "Shop/Component/T3ShopComponent.h"

#include "Item/Component/T3InventoryComponent.h"
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

void UT3ShopComponent::BeginPlay()
{
	Super::BeginPlay();
	
}