#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ItemSlotWidget.generated.h"

class UItemComponent;
class UImage;
class UBorder;

UCLASS()
class DESECRATION_API UItemSlotWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
	UImage* ItemIcon;

	UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
	UBorder* CooldownBorder;
};
