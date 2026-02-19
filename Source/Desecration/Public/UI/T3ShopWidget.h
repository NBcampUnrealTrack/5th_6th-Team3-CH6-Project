#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "T3ShopWidget.generated.h"

class AT3CharacterBase;
class UT3ShopComponent;
class UT3InventoryComponent;

UCLASS()
class DESECRATION_API UT3ShopWidget : public UUserWidget
{
	GENERATED_BODY()
	
	UPROPERTY(BlueprintReadOnly, meta = (AllowPrivateAccess = true))
	TObjectPtr<UT3InventoryComponent> InventoryComponent;
	
	UPROPERTY(BlueprintReadOnly, meta = (AllowPrivateAccess = true))
	TObjectPtr<UT3ShopComponent> ShopComponent;
	
	UPROPERTY(BlueprintReadOnly, meta = (AllowPrivateAccess = true))
	TObjectPtr<AT3CharacterBase> PlayerCharacter;
	
public:
	void Init(UT3InventoryComponent* InventoryComp, UT3ShopComponent* ShopComp, AT3CharacterBase* Character);
};
