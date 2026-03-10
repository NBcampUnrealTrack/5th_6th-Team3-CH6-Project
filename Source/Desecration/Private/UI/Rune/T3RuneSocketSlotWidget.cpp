#include "UI/Rune/T3RuneSocketSlotWidget.h"
#include "UI/ItemDragDropOperation.h"
#include "Equipment/T3PlayerEquipmentComponent.h"

FReply UT3RuneSocketSlotWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

void UT3RuneSocketSlotWidget::NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent,
	UDragDropOperation*& OutOperation)
{
	Super::NativeOnDragDetected(InGeometry, InMouseEvent, OutOperation);
}

bool UT3RuneSocketSlotWidget::NativeOnDragOver(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent,
	UDragDropOperation* InOperation)
{
	return Super::NativeOnDragOver(InGeometry, InDragDropEvent, InOperation);
}

bool UT3RuneSocketSlotWidget::NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent,
                                           UDragDropOperation* InOperation)
{
	if (!IsValid(EquipmentComponent))
	{
		return false;
	}

	UItemDragDropOperation* ItemDragOp = Cast<UItemDragDropOperation>(InOperation);

	if (!IsValid(ItemDragOp) || ItemDragOp->DraggedItemID == NAME_None)
	{
		return false;
	}

	UE_LOG(LogTemp, Log, TEXT("RuneSocket NativeOnDrop 호출됨"));

	return EquipmentComponent->SocketRune(ItemDragOp->DraggedItemID, TargetEquipmentType);
}
