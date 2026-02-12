// T3PlayerController.h

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "InputActionValue.h"
#include "T3PlayerController.generated.h"

class UT3PopUpMenu;
class UInputMappingContext;
class UInputAction;
class UUserWidget;

UCLASS()
class DESECRATION_API AT3PlayerController : public APlayerController
{
	GENERATED_BODY()

protected:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;

	// 에디터에서 할당할 위젯 클래스
	UPROPERTY(EditAnywhere, Category = "UI")
	TSubclassOf<class UUserWidget> LockOnWidgetClass;

	// 실제 생성된 위젯 인스턴스
	UPROPERTY()
	class UUserWidget* LockOnWidget;
	
	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputMappingContext> DefaultMappingContext;

	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputAction> MoveAction;

	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputAction> LookAction;

	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputAction> RollingAction;

	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputAction> BlockingAction;

	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputAction> LockOnAction;

	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputAction> InteractAction;

	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputAction> AttackAction;

	UPROPERTY(EditAnywhere, Category="Input")
	TObjectPtr<UInputAction> InventoryAction;

	UPROPERTY(EditAnywhere, Category="Input")
	TObjectPtr<UInputAction> InputTest;
	
	UPROPERTY(EditDefaultsOnly, Category="UI")
	TSubclassOf<UUserWidget> MainInventoryWidgetClass;
	
	UPROPERTY(BlueprintReadOnly, Category="UI")
	TObjectPtr<UUserWidget> MainInventoryWidget;
	
	UFUNCTION(BlueprintCallable, Category="UI")
	void SetInventoryOpen(bool bIsOpen);
	
	UFUNCTION(BlueprintPure, Category="UI")
	bool IsInventoryOpen() const { return bIsInventoryOpen; }

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UUserWidget> CombatWidgetClass;

	UPROPERTY(BlueprintReadOnly, Category = "UI")
	TObjectPtr<UUserWidget> CombatWidget;
	
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UT3PopUpMenu> PopUpMenuClass;
	
	UPROPERTY(BlueprintReadOnly, Category = "UI")
	TObjectPtr<UT3PopUpMenu> PopUpMenu;
	
	UPROPERTY(EditDefaultsOnly, Category="UI")
	TSubclassOf<UUserWidget> HUDSlotWidgetClass;
	
	UPROPERTY(BlueprintReadOnly, Category="UI")
	TObjectPtr<UUserWidget> HUDSlotWidget;

	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputAction> ChangeSkillSlotAction;
	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputAction> ChangePotionSlotAction;
	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputAction> ChangeConsumableSlotAction;
	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputAction> ActiveSkillSlotAction;
	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputAction> ActivePotionSlotAction;
	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputAction> ActiveConsumableSlotAction;

	TObjectPtr<class AT3CharacterBase> OwnerChar;
	TObjectPtr<class UT3CombatComponent> Combat;




private:
	bool bIsInventoryOpen = false;
	void Input_Move(const FInputActionValue& Value);
	void Input_Look(const FInputActionValue& Value);

	void Input_LockOn(const FInputActionValue& Value);
	void Input_BlockStart(const FInputActionValue& Value);
	void Input_BlockEnd(const FInputActionValue& Value);
	void Input_Roll(const FInputActionValue& Value);
	void Input_Interact(const FInputActionValue& Value);
	void Input_Test(const FInputActionValue& Value);

	void Input_Attack(const FInputActionValue& Value);

	void ToggleInventoryInput();

	// 슬롯 전환 및 사용
	void Input_ChangeSkillSlot(const FInputActionValue& Value);
	void Input_ChangePotionSlot(const FInputActionValue& Value);
	void Input_ChangeConsumableSlot(const FInputActionValue& Value);
	void Input_ActiveSkillSlot(const FInputActionValue& Value);
	void Input_ActivePotionSlot(const FInputActionValue& Value);
	void Input_ActiveConsumableSlot(const FInputActionValue& Value);


};
