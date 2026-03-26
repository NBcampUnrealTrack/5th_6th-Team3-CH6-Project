#include "UI/Rune/T3SynthesisSlotWidget.h"
#include "Components/Image.h"
#include "Equipment/T3UpgradeStation.h"
#include "UI/ItemDragDropOperation.h"

void UT3SynthesisSlotWidget::SetRuneIcon(UTexture2D* Icon)
{
	if (!Img_RuneIcon)
	{
		return;
	}

	Img_RuneIcon->SetBrushFromTexture(Icon);
	Img_RuneIcon->SetVisibility(ESlateVisibility::Visible);
}

void UT3SynthesisSlotWidget::ClearRuneIcon()
{
	if (!Img_RuneIcon)
	{
		return;
	}

	Img_RuneIcon->SetVisibility(ESlateVisibility::Collapsed);
}

FReply UT3SynthesisSlotWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::RightMouseButton)
	{
		if (UpgradeStation && SlotIndex >= 0)
		{
			UpgradeStation->RemoveRuneFromSynthesisSlot(SlotIndex);
		}
		return FReply::Handled();
	}

	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		if (UpgradeStation && UpgradeStation->SynthesisSlots.IsValidIndex(SlotIndex)
			&& UpgradeStation->SynthesisSlots[SlotIndex] != NAME_None)
		{
			if (TSharedPtr<SWidget> SlateWidget = GetCachedWidget())
			{
				return FReply::Handled().DetectDrag(SlateWidget.ToSharedRef(), EKeys::LeftMouseButton);
			}
		}
		return FReply::Handled();
	}

	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

bool UT3SynthesisSlotWidget::NativeOnDragOver(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	UItemDragDropOperation* ItemDragOp = Cast<UItemDragDropOperation>(InOperation);
	if (!IsValid(ItemDragOp) || ItemDragOp->bIsFromRuneSocket)
	{
		return false;
	}

	return true;
}

void UT3SynthesisSlotWidget::NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent,
	UDragDropOperation*& OutOperation)
{
	if (!UpgradeStation || !UpgradeStation->SynthesisSlots.IsValidIndex(SlotIndex))
	{
		return;
	}

	FName RuneID = UpgradeStation->SynthesisSlots[SlotIndex];
	
	if (RuneID == NAME_None)
	{
		return;
	}

	UItemDragDropOperation* DragOp = NewObject<UItemDragDropOperation>();
	
	DragOp->DraggedItemID = RuneID;
	DragOp->bIsFromSynthesisSlot = true;
	DragOp->SourceSynthesisSlotWidget = this;
	DragOp->DefaultDragVisual = Img_RuneIcon;
	DragOp->Pivot = EDragPivot::MouseDown;

	OutOperation = DragOp;
}

bool UT3SynthesisSlotWidget::NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	UItemDragDropOperation* ItemDragOp = Cast<UItemDragDropOperation>(InOperation);
	
	if (!IsValid(ItemDragOp) || ItemDragOp->bIsFromRuneSocket || ItemDragOp->bIsFromSynthesisSlot)
	{
		return false;
	}

	if (!UpgradeStation || ItemDragOp->DraggedItemID == NAME_None)
	{
		return false;
	}

	return UpgradeStation->AddRuneToSynthesisSlot(ItemDragOp->DraggedItemID);
}
