#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "T3ShopComponent.generated.h"

class UDataTable;
class UT3InventoryComponent;

UENUM(BlueprintType)
enum class EShopBuyResult : uint8
{
	None,
	InvalidData, // ShopData or Inventory 없음
	ItemNotFound, // 데이터테이블에 없음
	CannotBuy, // 구매 불가 아이템
	NotEnoughMoney, // 돈 부족
	Succeeded // 구매 성공
};

UENUM(BlueprintType)
enum class EShopSellResult : uint8
{
	None,
	InvalidData, // ShopData or Inventory 없음
	ItemNotFound, // 인벤토리에 없음
	CannotSell, // 판매 불가 아이템
	Succeeded // 판매 성공
};

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class DESECRATION_API UT3ShopComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UT3ShopComponent();

	UFUNCTION(BlueprintCallable, Category = "Shop")
	EShopBuyResult BuyItem(const FName& ItemName, UT3InventoryComponent* Inventory);
	
	UFUNCTION(BlueprintCallable, Category = "Shop")
	EShopSellResult SellItem(const FName& ItemName, UT3InventoryComponent* Inventory);
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Data")
	TObjectPtr<UDataTable> ShopData;
	
protected:
	virtual void BeginPlay() override;
};
