#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Equipment/T3EquipmentTypes.h"
#include "T3RuneSocketSlotWidget.generated.h"

class UT3PlayerEquipmentComponent;

UCLASS()
class DESECRATION_API UT3RuneSocketSlotWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (ExposeOnSpawn = "true"))
	ET3EquipmentType TargetEquipmentType;

	UPROPERTY(BlueprintReadWrite, meta = (ExposeOnSpawn = "true"))
	TObjectPtr<UT3PlayerEquipmentComponent> EquipmentComponent;

protected:
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
    
	virtual void NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent, UDragDropOperation*& OutOperation) override;
    
	virtual bool NativeOnDragOver(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;
    
	virtual bool NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;

};
