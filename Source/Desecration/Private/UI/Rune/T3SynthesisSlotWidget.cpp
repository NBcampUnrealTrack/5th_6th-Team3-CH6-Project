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

bool UT3SynthesisSlotWidget::NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	UItemDragDropOperation* ItemDragOp = Cast<UItemDragDropOperation>(InOperation);
	if (!IsValid(ItemDragOp) || ItemDragOp->bIsFromRuneSocket)
	{
		return false;
	}

	if (!UpgradeStation || ItemDragOp->DraggedItemID == NAME_None)
	{
		return false;
	}

	return UpgradeStation->AddRuneToSynthesisSlot(ItemDragOp->DraggedItemID);
}