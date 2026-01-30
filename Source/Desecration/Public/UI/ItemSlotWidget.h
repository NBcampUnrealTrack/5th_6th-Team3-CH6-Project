#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Widget.h"
#include "ItemSlotWidget.generated.h"

class UT3InventoryComponent;
class UImage;
class UBorder;

UCLASS()
class DESECRATION_API UItemSlotWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadWrite, Category = "Slot", meta = (FieldNotify, ExposeOnSpawn = "true"))
	int32 SlotIndex = -1;
    
	UPROPERTY(BlueprintReadWrite, Category = "Slot", meta = (ExposeOnSpawn = "true"))
	TObjectPtr<class UT3InventoryComponent> InventoryComponent;
	
protected:
	UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
	UImage* ItemIcon;

	UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
	UBorder* CooldownBorder;
	
	// 마우스 버튼이 눌렸을 때 호출
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
    
	// 드래그가 감지되었을 때 호출 (드래그 시작)
	virtual void NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent, UDragDropOperation*& OutOperation) override;
    
	// 드래그 중 다른 위젯 위로 올라갔을 때 호출 (드롭 가능 여부 확인)
	virtual bool NativeOnDragOver(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;
    
	// 드롭되었을 때 호출 (실제 드롭 처리)
	virtual bool NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;
};
