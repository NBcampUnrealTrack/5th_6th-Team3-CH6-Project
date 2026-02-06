#include "UI/ItemSlotWidget.h"

#include "Components/Image.h"
#include "Components/Border.h"
#include "UI/ItemDragDropOperation.h"
#include "Item/Component/T3InventoryComponent.h"

void UItemSlotWidget::SetSelected(bool bSelected)
{
	UE_LOG(LogTemp, Warning, TEXT("SetSelected called: %s"), bSelected ? TEXT("true") : TEXT("false"));

	SelectionBorder->SetVisibility(bSelected ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
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
		
		if (!InventoryComponent->Items.IsValidIndex(SlotIndex))
		{
			return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
		}
		
		if (InventoryComponent->Items[SlotIndex].ItemID == NAME_None)
		{
			return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
		}
		
		FReply Reply = Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
		
		OnSlotClicked.Broadcast(this);
		
		// 드래그 감지를 활성화
		if (TSharedPtr<SWidget> SlateWidget = GetCachedWidget())
		{
			return FReply::Handled().DetectDrag(SlateWidget.ToSharedRef(), EKeys::LeftMouseButton);
		}
		
		return Reply;
	}
	
	// 다른 버튼이면 기본 동작
	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

void UItemSlotWidget::NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent, UDragDropOperation*& OutOperation)
{
	if (!IsValid(InventoryComponent) || !InventoryComponent->Items.IsValidIndex(SlotIndex))
	{
		return;
	}
	
	if (InventoryComponent->Items[SlotIndex].ItemID == NAME_None)
	{
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
	// 드래그 중인 오퍼레이션이 유효한지 확인
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
	
	if (!IsValid(InventoryComponent))
	{
		return false;
	}
	
	// 드롭 가능하다고 표시 (시각적 피드백을 위해)
	// Blueprint에서 위젯 색상 변경 등을 할 수 있음
	
	return true;
}

bool UItemSlotWidget::NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	if (!IsValid(InOperation))
	{
		return false;
	}
	
	UItemDragDropOperation* ItemDragOp = Cast<UItemDragDropOperation>(InOperation);
	if (!IsValid(ItemDragOp))
	{
		return false;
	}
	
	if (!IsValid(InventoryComponent))
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