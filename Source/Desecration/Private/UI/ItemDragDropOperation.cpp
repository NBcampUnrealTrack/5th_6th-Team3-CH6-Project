#include "UI/ItemDragDropOperation.h"
#include "UI/ItemSlotWidget.h"
#include "Equipment/T3PlayerEquipmentComponent.h"

void UItemDragDropOperation::Drop_Implementation(const FPointerEvent& PointerEvent)
{
	Super::Drop_Implementation(PointerEvent);

	if (bIsFromRuneSocket && !bDropHandledBySameSocket && IsValid(SourceEquipmentComponent))
	{
		SourceEquipmentComponent->UnsocketRune(DraggedItemID, SourceEquipmentType);
	}
}

void UItemDragDropOperation::DragCancelled_Implementation(const FPointerEvent& PointerEvent)
{
	Super::DragCancelled_Implementation(PointerEvent);

	if (bIsFromRuneSocket && IsValid(SourceEquipmentComponent))
	{
		SourceEquipmentComponent->UnsocketRune(DraggedItemID, SourceEquipmentType);
	}
}