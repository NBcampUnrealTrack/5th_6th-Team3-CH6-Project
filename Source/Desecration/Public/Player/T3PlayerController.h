// T3PlayerController.h

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "InputActionValue.h"
#include "T3PlayerController.generated.h"

class UT3ShopComponent;
class UT3PopUpMenu;
class UInputMappingContext;
class UInputAction;
class UUserWidget;
class UT3ShopWidget;

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
	
	UFUNCTION(BlueprintCallable, Category="UI")
	void SetShopUIOpen(bool bIsOpen);

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UUserWidget> CombatWidgetClass;

	UPROPERTY(BlueprintReadOnly, Category = "UI")
	TObjectPtr<UUserWidget> CombatWidget;
	
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UT3PopUpMenu> PopUpMenuClass;
	
	UPROPERTY(BlueprintReadOnly, Category = "UI")
	TObjectPtr<UT3PopUpMenu> PopUpMenu;
	
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<class UT3HolyGaugeWidget> HolyGaugeWidgetClass;


	UPROPERTY(EditDefaultsOnly, Category="UI")
	TSubclassOf<class UT3HUDSlotWidget> HUDSlotWidgetClass;
	
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UT3ShopWidget> ShopWidgetClass;
	
	UPROPERTY(BlueprintReadOnly, Category = "UI")
	TObjectPtr<UT3ShopWidget> ShopWidget;
	
public:
	UPROPERTY(BlueprintReadOnly, Category = "UI")
	TObjectPtr<class UT3HolyGaugeWidget> HolyGaugeWidget;

	UPROPERTY(BlueprintReadOnly, Category="UI")
	TObjectPtr<class UT3HUDSlotWidget> HUDSlotWidget;

	void ShowShopUI(UT3ShopComponent* ShopComp);
	
	bool GetIsUpgradeUIOpen() const;
	void SetIsUpgradeUIOpen(bool bIsOpen);
	
protected:
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

	UPROPERTY()
	TObjectPtr<class AT3CharacterBase> OwnerChar;
	UPROPERTY()
	TObjectPtr<class UT3CombatComponent> Combat;
	
	//설정에서 지정한 내용
	UPROPERTY()
	TObjectPtr<class UT3SaveUserSettings> SaveUserSettings;

private:
	bool bIsInventoryOpen = false;
	bool bIsShopUIOpen = false;
	void Input_Move(const FInputActionValue& Value);
	void Input_Look(const FInputActionValue& Value);

	void Input_LockOn(const FInputActionValue& Value);
	void Input_BlockStart(const FInputActionValue& Value);
	void Input_BlockEnd(const FInputActionValue& Value);
	void Input_Parry(const FInputActionValue& Value);
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
	void Input_ActiveSkillSlot_Completed(const FInputActionValue& Value);
	void Input_ActivePotionSlot(const FInputActionValue& Value);
	void Input_ActiveConsumableSlot(const FInputActionValue& Value);

	uint8 bIsUpgradeUIOpen : 1 = false;
};
