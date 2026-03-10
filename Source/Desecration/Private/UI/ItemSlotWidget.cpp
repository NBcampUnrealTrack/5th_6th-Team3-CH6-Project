#include "UI/ItemSlotWidget.h"

#include "Components/Image.h"
#include "Components/Border.h"
#include "UI/ItemDragDropOperation.h"
#include "Item/Component/T3InventoryComponent.h"

void UItemSlotWidget::SetSelected(bool bSelected)
{
	Border_EquippedOrder->SetVisibility(bSelected ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
}

bool UItemSlotWidget::GetSlotData_Implementation(FInventorySlot& OutSlotData) const
{
	return false;
}

FReply UItemSlotWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	// 왼쪽 마우스 버튼이 눌렸는지 확인
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
		
		// 드래그 감지를 먼저 활성화 (Super 호출 전에)
		if (TSharedPtr<SWidget> SlateWidget = GetCachedWidget())
		{
			UE_LOG(LogTemp, Log, TEXT("[ItemSlotWidget] 마우스 클릭 감지 - 슬롯 %d, 드래그 감지 시작"), SlotIndex);
			
			// OnSlotClicked 브로드캐스트는 드래그 감지 후에 호출
			// 이렇게 하면 드래그가 우선적으로 처리됨
			OnSlotClicked.Broadcast(this);
			
			return FReply::Handled().DetectDrag(SlateWidget.ToSharedRef(), EKeys::LeftMouseButton);
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("[ItemSlotWidget] SlateWidget이 유효하지 않음 - 슬롯 %d"), SlotIndex);
		}
		
		// SlateWidget이 없으면 기본 동작
		OnSlotClicked.Broadcast(this);
		return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
	}
	
	// 다른 버튼이면 기본 동작
	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

void UItemSlotWidget::NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent, UDragDropOperation*& OutOperation)
{
	UE_LOG(LogTemp, Log, TEXT("[ItemSlotWidget] NativeOnDragDetected 호출됨 - 슬롯 %d"), SlotIndex);
	
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
	
	if (SlotIndex < 0 || ItemDragOp->SourceSlotIndex < 0)
	{
		return false;
	}
	
	if (ItemDragOp->SourceSlotIndex == SlotIndex)
	{
		return false;
	}
	
	// 실제 슬롯 교체
	InventoryComponent->SwapSlots(ItemDragOp->SourceSlotIndex, SlotIndex);
	
	UE_LOG(LogTemp, Log, TEXT("드롭 완료: 슬롯 %d -> %d"), ItemDragOp->SourceSlotIndex, SlotIndex);
	
	return true;
}