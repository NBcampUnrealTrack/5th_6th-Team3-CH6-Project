#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "T3ShopComponent.generated.h"

class UDataTable;
class UT3InventoryComponent;
class AT3PlayerController;

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

USTRUCT(BlueprintType)
struct FT3ShopItemUIData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	FName ItemID = NAME_None;
	
	UPROPERTY(BlueprintReadOnly)
	FText Name = FText::GetEmpty();
	
	UPROPERTY(BlueprintReadOnly)
	UTexture2D* Icon = nullptr;
	
	UPROPERTY(BlueprintReadOnly)
	int32 BuyPrice = 0;
	
	UPROPERTY(BlueprintReadOnly)
	uint8 bCanBuy : 1 = false;
	
	UPROPERTY(BlueprintReadOnly)
	int32 SellPrice = 0;
	
	UPROPERTY(BlueprintReadOnly)
	uint8 bCanSell : 1 = false;
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
	
	UFUNCTION(BlueprintCallable, Category = "Shop")
	void GetShopItemUIData(TArray<FT3ShopItemUIData>& OutItems) const;
	
	UFUNCTION(BlueprintCallable, Category = "Shop")
	void OpenShop(AT3PlayerController* T3PC);
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Data")
	TObjectPtr<UDataTable> ShopData;
	
};
