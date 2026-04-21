#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Equipment/T3EquipmentTypes.h"
#include "T3RuneSocketSlotWidget.generated.h"

class UT3PlayerEquipmentComponent;

// 룬 소켓이 비어있을 때 우클릭 — 장신구 해제 등 외부 처리용
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnRuneSocketRightClickedEmpty);

UCLASS()
class DESECRATION_API UT3RuneSocketSlotWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (ExposeOnSpawn = "true"))
	ET3EquipmentType TargetEquipmentType;

	UPROPERTY(BlueprintReadWrite, meta = (ExposeOnSpawn = "true"))
	TObjectPtr<UT3PlayerEquipmentComponent> EquipmentComponent;

	// 소켓이 비어있을 때 우클릭 시 발동 — 블루프린트에서 장신구 해제 등에 바인딩
	UPROPERTY(BlueprintAssignable, Category = "Rune Socket")
	FOnRuneSocketRightClickedEmpty OnRightClickedEmpty;

protected:
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
    
	virtual void NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent, UDragDropOperation*& OutOperation) override;
    
	virtual bool NativeOnDragOver(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;
    
	virtual bool NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;

};
