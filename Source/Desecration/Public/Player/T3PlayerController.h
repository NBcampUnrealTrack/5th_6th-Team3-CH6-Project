// T3PlayerController.h

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "InputActionValue.h"
#include "T3PlayerController.generated.h"

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
	

	TObjectPtr<class AT3CharacterBase> OwnerChar;
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
};
