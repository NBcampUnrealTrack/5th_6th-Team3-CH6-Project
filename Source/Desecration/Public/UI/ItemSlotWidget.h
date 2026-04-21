#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ItemSlotWidget.generated.h"

class UT3InventoryComponent;
class UImage;
class UBorder;
struct FInventorySlot;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnInventorySlotClicked, UItemSlotWidget*, SlotWidget, bool, bIsRightClick);

UCLASS()
class DESECRATION_API UItemSlotWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadWrite, Category = "Slot", meta = (FieldNotify, ExposeOnSpawn = "true"))
	int32 SlotIndex = -1;
    
	UPROPERTY(BlueprintReadWrite, Category = "Slot", meta = (ExposeOnSpawn = "true"))
	TObjectPtr<UT3InventoryComponent> InventoryComponent;
	
	UFUNCTION(BlueprintCallable)
	void SetSelected(bool bSelected);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Item Slot")
	bool GetSlotData(FInventorySlot& OutSlotData) const;
	
	UFUNCTION(BlueprintPure)
	bool GetIsRuneSlot() const;

	UFUNCTION(BlueprintCallable)
	bool SetIsRuneSlot(bool IsRuneSlot);

	UFUNCTION(BlueprintPure)
	bool GetIsEtcSlot() const;

	UFUNCTION(BlueprintCallable)
	bool SetIsEtcSlot(bool IsEtcSlot);

	UFUNCTION(BlueprintPure)
	bool GetIsAccSlot() const;

	UFUNCTION(BlueprintCallable)
	bool SetIsAccSlot(bool IsAccSlot);
	
	UPROPERTY(BlueprintAssignable)
	FOnInventorySlotClicked OnSlotClicked;
	
protected:
	UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
	UImage* ItemIcon;

	UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
	UBorder* Border_EquippedOrder;
	
	UPROPERTY(BlueprintReadWrite)
	uint8 bIsRuneSlot : 1 = false;

	UPROPERTY(BlueprintReadWrite)
	uint8 bIsEtcSlot : 1 = false;

	UPROPERTY(BlueprintReadWrite)
	uint8 bIsAccSlot : 1 = false;
	
	uint8 bDragDetected : 1 = false;
	
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

	virtual FReply NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

	virtual void NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent, UDragDropOperation*& OutOperation) override;

	virtual bool NativeOnDragOver(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;

	virtual bool NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;
};
