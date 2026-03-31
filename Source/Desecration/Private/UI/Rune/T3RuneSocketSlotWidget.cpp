#include "UI/Rune/T3RuneSocketSlotWidget.h"
#include "UI/ItemDragDropOperation.h"
#include "Equipment/T3PlayerEquipmentComponent.h"

FReply UT3RuneSocketSlotWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::RightMouseButton)
	{
		if (IsValid(EquipmentComponent))
		{
			const TArray<FName>& SocketedIDs =
				(TargetEquipmentType == ET3EquipmentType::Weapon) ? EquipmentComponent->WeaponSocketedRuneIDs :
				(TargetEquipmentType == ET3EquipmentType::Armor)  ? EquipmentComponent->ArmorSocketedRuneIDs :
																	EquipmentComponent->ClassSocketedRuneIDs;

			if (SocketedIDs.Num() > 0)
			{
				EquipmentComponent->UnsocketRune(SocketedIDs[0], TargetEquipmentType);
			}
		}
		return FReply::Handled();
	}

	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		if (!IsValid(EquipmentComponent))
		{
			return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
		}

		const TArray<FName>& SocketedIDs =
			(TargetEquipmentType == ET3EquipmentType::Weapon) ? EquipmentComponent->WeaponSocketedRuneIDs :
			(TargetEquipmentType == ET3EquipmentType::Armor)  ? EquipmentComponent->ArmorSocketedRuneIDs :
																EquipmentComponent->ClassSocketedRuneIDs;

		if (SocketedIDs.Num() == 0)
		{
			return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
		}

		if (TSharedPtr<SWidget> SlateWidget = GetCachedWidget())
		{
			return FReply::Handled().DetectDrag(SlateWidget.ToSharedRef(), EKeys::LeftMouseButton);
		}
	}

	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

void UT3RuneSocketSlotWidget::NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent,
	UDragDropOperation*& OutOperation)
{
	if (!IsValid(EquipmentComponent))
	{
		return;
	}

	const TArray<FName>& SocketedIDs =
		(TargetEquipmentType == ET3EquipmentType::Weapon) ? EquipmentComponent->WeaponSocketedRuneIDs :
		(TargetEquipmentType == ET3EquipmentType::Armor)  ? EquipmentComponent->ArmorSocketedRuneIDs :
															EquipmentComponent->ClassSocketedRuneIDs;

	if (SocketedIDs.Num() == 0)
	{
		return;
	}

	UItemDragDropOperation* DragOp = NewObject<UItemDragDropOperation>();
	DragOp->bIsFromRuneSocket = true;
	DragOp->DraggedItemID = SocketedIDs[0];
	DragOp->SourceEquipmentComponent = EquipmentComponent;
	DragOp->SourceEquipmentType = TargetEquipmentType;
	DragOp->DefaultDragVisual = this;
	DragOp->Pivot = EDragPivot::MouseDown;

	OutOperation = DragOp;
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

	if (!IsValid(ItemDragOp))
	{
		return false;
	}

	if (ItemDragOp->bIsFromRuneSocket && ItemDragOp->SourceEquipmentType == TargetEquipmentType)
	{
		ItemDragOp->bDropHandledBySameSocket = true;
		return true;
	}

	if (!ItemDragOp->bIsFromRuneSocket && ItemDragOp->DraggedItemID != NAME_None)
	{
		UE_LOG(LogTemp, Log, TEXT("RuneSocket NativeOnDrop 호출됨"));
		return EquipmentComponent->SocketRune(ItemDragOp->DraggedItemID, TargetEquipmentType);
	}

	return false;
}
