#pragma once

#include "CoreMinimal.h"
#include "Blueprint/DragDropOperation.h"
#include "ItemDragDropOperation.generated.h"

// 전방 선언
class UItemSlotWidget;

/**
 * 인벤토리 아이템 드래그 앤 드롭을 위한 커스텀 DragDropOperation
 */
UCLASS()
class DESECRATION_API UItemDragDropOperation : public UDragDropOperation
{
	GENERATED_BODY()

public:
	// 드래그 시작한 슬롯의 인덱스
	UPROPERTY(BlueprintReadWrite, Category = "Drag Drop")
	int32 SourceSlotIndex = -1;
	
	// 드래그 시작한 슬롯 위젯 참조
	UPROPERTY(BlueprintReadWrite, Category = "Drag Drop")
	TObjectPtr<UItemSlotWidget> SourceSlotWidget;
};

