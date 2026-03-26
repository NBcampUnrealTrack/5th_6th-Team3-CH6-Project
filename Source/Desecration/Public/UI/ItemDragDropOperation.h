#pragma once

#include "CoreMinimal.h"
#include "Blueprint/DragDropOperation.h"
#include "Equipment/T3EquipmentTypes.h"
#include "ItemDragDropOperation.generated.h"

class UItemSlotWidget;
class UT3PlayerEquipmentComponent;
class UT3SynthesisSlotWidget;

UCLASS()
class DESECRATION_API UItemDragDropOperation : public UDragDropOperation
{
	GENERATED_BODY()

public:
	// 드래그 시작한 슬롯의 인덱스
	UPROPERTY(BlueprintReadWrite, Category = "Drag Drop")
	int32 SourceSlotIndex = -1;
	
	UPROPERTY(BlueprintReadWrite, Category = "Drag Drop")
	FName DraggedItemID = NAME_None;
	
	// 드래그 시작한 슬롯 위젯 참조
	UPROPERTY(BlueprintReadWrite, Category = "Drag Drop")
	TObjectPtr<UItemSlotWidget> SourceSlotWidget;
	
	UPROPERTY(BlueprintReadWrite, Category = "Drag Drop|Rune")
	bool bIsFromRuneSocket = false;

	UPROPERTY(BlueprintReadWrite, Category = "Drag Drop|Rune")
	TObjectPtr<UT3PlayerEquipmentComponent> SourceEquipmentComponent;

	UPROPERTY(BlueprintReadWrite, Category = "Drag Drop|Rune")
	ET3EquipmentType SourceEquipmentType = ET3EquipmentType::Weapon;
	
	bool bDropHandledBySameSocket = false;

	UPROPERTY()
	bool bIsFromSynthesisSlot = false;

	UPROPERTY()
	TObjectPtr<UT3SynthesisSlotWidget> SourceSynthesisSlotWidget = nullptr;
	
protected:
	virtual void Drop_Implementation(const FPointerEvent& PointerEvent) override;

	virtual void DragCancelled_Implementation(const FPointerEvent& PointerEvent) override;

};

