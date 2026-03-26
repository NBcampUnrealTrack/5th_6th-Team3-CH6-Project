#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "T3SynthesisSlotWidget.generated.h"

class AT3UpgradeStation;
class UImage;

UCLASS()
class DESECRATION_API UT3SynthesisSlotWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadWrite, Category = "Synthesis")
	int32 SlotIndex = -1;

	UPROPERTY(BlueprintReadWrite, Category = "Synthesis")
	TObjectPtr<AT3UpgradeStation> UpgradeStation;

	void SetRuneIcon(UTexture2D* Icon);
	
	void ClearRuneIcon();

protected:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> Img_RuneIcon;

	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

	virtual bool NativeOnDragOver(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;

	virtual bool NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;
};