#include "UI/ItemSlotWidget.h"

#include "Components/Image.h"
#include "Components/Border.h"
#include "Equipment/T3UpgradeStation.h"
#include "UI/ItemDragDropOperation.h"
#include "Item/Component/T3InventoryComponent.h"
#include "UI/Rune/T3SynthesisSlotWidget.h"

void UItemSlotWidget::SetSelected(bool bSelected)
{
	Border_EquippedOrder->SetVisibility(bSelected ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
}

bool UItemSlotWidget::GetIsRuneSlot() const
{
	return bIsRuneSlot;
}

bool UItemSlotWidget::SetIsRuneSlot(bool IsRuneSlot)
{
	bIsRuneSlot = IsRuneSlot;

	return bIsRuneSlot;
}

bool UItemSlotWidget::GetIsEtcSlot() const
{
	return bIsEtcSlot;
}

bool UItemSlotWidget::SetIsEtcSlot(bool IsEtcSlot)
{
	bIsEtcSlot = IsEtcSlot;

	return bIsEtcSlot;
}

bool UItemSlotWidget::GetIsAccSlot() const
{
	return bIsAccSlot;
}

bool UItemSlotWidget::SetIsAccSlot(bool IsAccSlot)
{
	bIsAccSlot = IsAccSlot;

	return bIsAccSlot;
}

bool UItemSlotWidget::GetSlotData_Implementation(FInventorySlot& OutSlotData) const
{
	return false;
}	

FReply UItemSlotWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::RightMouseButton)
	{
		OnSlotClicked.Broadcast(this, true);
		return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
	}
	
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		if (!IsValid(InventoryComponent))
		{
			return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
		}

		FInventorySlot SlotData;
		if (!GetSlotData(SlotData) || SlotData.ItemID == NAME_None)
		{
			return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
		}

		if (TSharedPtr<SWidget> SlateWidget = GetCachedWidget())
		{
			UE_LOG(LogTemp, Log, TEXT("[ItemSlotWidget] 마우스 클릭 감지 - 슬롯 %d, 드래그 감지 시작"), SlotIndex);

			bDragDetected = false;
			return FReply::Handled().DetectDrag(SlateWidget.ToSharedRef(), EKeys::LeftMouseButton);
		}

		return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
	}
	
	// 다른 버튼이면 기본 동작
	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

FReply UItemSlotWidget::NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && !bDragDetected)
	{
		FInventorySlot SlotData;
		if (IsValid(InventoryComponent) && GetSlotData(SlotData) && SlotData.ItemID != NAME_None)
		{
			OnSlotClicked.Broadcast(this, false);
		}
	}

	return Super::NativeOnMouseButtonUp(InGeometry, InMouseEvent);
}

void UItemSlotWidget::NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent, UDragDropOperation*& OutOperation)
{
	UE_LOG(LogTemp, Log, TEXT("[ItemSlotWidget] NativeOnDragDetected 호출됨 - 슬롯 %d"), SlotIndex);
	bDragDetected = true;
	
	FInventorySlot SlotData;
	if (!IsValid(InventoryComponent) || !GetSlotData(SlotData) || SlotData.ItemID == NAME_None)
	{
		UE_LOG(LogTemp, Warning, TEXT("[ItemSlotWidget] InventoryComponent 또는 슬롯 %d에 유효한 아이템 없음"), SlotIndex);
		return;
	}
	
	UItemDragDropOperation* DragOperation = NewObject<UItemDragDropOperation>();
	
	// 드래그 시작한 슬롯 정보 저장
	DragOperation->SourceSlotIndex = SlotIndex;
	DragOperation->SourceSlotWidget = this;
	DragOperation->DraggedItemID = SlotData.ItemID;
	
	if (IsValid(ItemIcon))
	{
		// 아이콘 이미지를 드래그 중 표시
		DragOperation->DefaultDragVisual = ItemIcon;
	}
	else
	{
		// 아이콘이 없으면 현재 위젯 자체를 표시
		DragOperation->DefaultDragVisual = this;
	}
	
	// 드래그 시작점을 마우스 위치로 설정
	DragOperation->Pivot = EDragPivot::MouseDown;
	
	// OutOperation에 할당 (이것이 실제로 드래그되는 오퍼레이션)
	OutOperation = DragOperation;
	
	UE_LOG(LogTemp, Log, TEXT("드래그 시작: 슬롯 %d"), SlotIndex);
}

bool UItemSlotWidget::NativeOnDragOver(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	if (!IsValid(InventoryComponent))
	{
		return false;
	}
	
	if (!IsValid(InOperation))
	{
		return false;
	}
	
	UItemDragDropOperation* ItemDragOp = Cast<UItemDragDropOperation>(InOperation);
	if (!IsValid(ItemDragOp))
	{
		return false;
	}
	
	if (ItemDragOp->SourceSlotIndex == SlotIndex)
	{
		return false;
	}
	
	return true;
}

bool UItemSlotWidget::NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	if (!IsValid(InventoryComponent))
	{
		return false;
	}
	
	if (!IsValid(InOperation))
	{
		return false;
	}
	
	UItemDragDropOperation* ItemDragOp = Cast<UItemDragDropOperation>(InOperation);
	if (!IsValid(ItemDragOp))
	{
		return false;
	}
	
	if (ItemDragOp->bIsFromRuneSocket)
	{
		return true;
	}
	
	if (ItemDragOp->bIsFromSynthesisSlot)
	{
		if (IsValid(ItemDragOp->SourceSynthesisSlotWidget))
		{
			ItemDragOp->SourceSynthesisSlotWidget->UpgradeStation->RemoveRuneFromSynthesisSlot(ItemDragOp->SourceSynthesisSlotWidget->SlotIndex);
		}
		return true;
	}
	
	if (SlotIndex < 0 || ItemDragOp->SourceSlotIndex < 0)
	{
		return false;
	}
	
	if (ItemDragOp->SourceSlotIndex == SlotIndex)
	{
		return false;
	}
	
	if (bIsRuneSlot)
	{
		InventoryComponent->SwapRuneSlots(ItemDragOp->SourceSlotIndex, SlotIndex);
	}
	else if (bIsEtcSlot)
	{
		InventoryComponent->SwapEtcSlots(ItemDragOp->SourceSlotIndex, SlotIndex);
	}
	else if (bIsAccSlot)
	{
		InventoryComponent->SwapAccessorySlots(ItemDragOp->SourceSlotIndex, SlotIndex);
	}
	else
	{
		InventoryComponent->SwapSlots(ItemDragOp->SourceSlotIndex, SlotIndex);
	}
	
	UE_LOG(LogTemp, Log, TEXT("드롭 완료: 슬롯 %d -> %d"), ItemDragOp->SourceSlotIndex, SlotIndex);
	
	return true;
}