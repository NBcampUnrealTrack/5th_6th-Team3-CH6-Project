// T3PlayerController.cpp


#include "Player/T3PlayerController.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Player/T3CharacterBase.h"
#include "Player/T3CombatComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/Character.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Blueprint/UserWidget.h"
#include "Item/Component/T3InventoryComponent.h"
#include "UI/T3PopUpMenu.h"

void AT3PlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
		if (DefaultMappingContext)
		{
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
		}
	}
	
	if (IsValid(MainInventoryWidgetClass))
	{
		MainInventoryWidget = CreateWidget<UUserWidget>(this, MainInventoryWidgetClass);
		
		MainInventoryWidget->SetVisibility(ESlateVisibility::Collapsed);
		MainInventoryWidget->AddToViewport(3);
	}

	if (IsValid(CombatWidgetClass))
	{
		CombatWidget = CreateWidget<UUserWidget>(this, CombatWidgetClass);

		//CombatWidget->SetVisibility(ESlateVisibility::Collapsed);
		CombatWidget->AddToViewport();
	}
	
	if (IsValid(PopUpMenuClass))
	{
		PopUpMenu = CreateWidget<UT3PopUpMenu>(this, PopUpMenuClass);
		PopUpMenu->AddToViewport();
	}
	
	if (IsValid(HUDSlotWidgetClass))
	{
		HUDSlotWidget = CreateWidget<UUserWidget>(this, HUDSlotWidgetClass);
		HUDSlotWidget->AddToViewport();
	}

	if (LockOnWidgetClass)
	{
		LockOnWidget = CreateWidget<UUserWidget>(this, LockOnWidgetClass);
		if (LockOnWidget)
		{
			LockOnWidget->AddToViewport();
			LockOnWidget->SetVisibility(ESlateVisibility::Collapsed); // 평소엔 숨김
		}
	}

	APawn* NewPawn = GetPawn();
	OwnerChar = Cast<AT3CharacterBase>(NewPawn);
	Combat = OwnerChar->GetCombatComponent();
	
}

void AT3PlayerController::SetupInputComponent()
{
	APlayerController::SetupInputComponent();

	if (UEnhancedInputComponent* EnhancedInputComponent = CastChecked<UEnhancedInputComponent>(InputComponent))
	{
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AT3PlayerController::Input_Move);
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AT3PlayerController::Input_Look);

		EnhancedInputComponent->BindAction(LockOnAction, ETriggerEvent::Started, this, &AT3PlayerController::Input_LockOn);
		EnhancedInputComponent->BindAction(BlockingAction, ETriggerEvent::Started, this, &AT3PlayerController::Input_BlockStart);
		EnhancedInputComponent->BindAction(BlockingAction, ETriggerEvent::Completed, this, &AT3PlayerController::Input_BlockEnd);
		EnhancedInputComponent->BindAction(RollingAction, ETriggerEvent::Started, this, &AT3PlayerController::Input_Roll);
		EnhancedInputComponent->BindAction(InteractAction, ETriggerEvent::Started, this, &AT3PlayerController::Input_Interact);
		EnhancedInputComponent->BindAction(InputTest, ETriggerEvent::Started, this, &AT3PlayerController::Input_Test);
		EnhancedInputComponent->BindAction(AttackAction, ETriggerEvent::Started, this, &AT3PlayerController::Input_Attack);
	
		EnhancedInputComponent->BindAction(InventoryAction, ETriggerEvent::Started, this, &AT3PlayerController::ToggleInventoryInput);

		// 슬롯 체인지 및 사용
		EnhancedInputComponent->BindAction(ChangeSkillSlotAction, ETriggerEvent::Started, this, &AT3PlayerController::Input_ChangeSkillSlot);
		EnhancedInputComponent->BindAction(ChangePotionSlotAction, ETriggerEvent::Started, this, &AT3PlayerController::Input_ChangePotionSlot);
		EnhancedInputComponent->BindAction(ChangeConsumableSlotAction, ETriggerEvent::Started, this, &AT3PlayerController::Input_ChangeConsumableSlot);
		EnhancedInputComponent->BindAction(ActiveSkillSlotAction, ETriggerEvent::Started, this, &AT3PlayerController::Input_ActiveSkillSlot);
		EnhancedInputComponent->BindAction(ActivePotionSlotAction, ETriggerEvent::Started, this, &AT3PlayerController::Input_ActivePotionSlot);
		EnhancedInputComponent->BindAction(ActiveConsumableSlotAction, ETriggerEvent::Started, this, &AT3PlayerController::Input_ActiveConsumableSlot);

	}
}

void AT3PlayerController::Input_Move(const FInputActionValue& Value)
{
	if (bIsInventoryOpen || !OwnerChar->CanExecuteAction())
	{
		return;
	}

	FVector2D MovementVector = Value.Get<FVector2D>();

	if (OwnerChar)
	{
		OwnerChar->Move(MovementVector);
	}
}

void AT3PlayerController::Input_Look(const FInputActionValue& Value)
{
	if (bIsInventoryOpen || !OwnerChar->CanExecuteAction())
	{
		return;
	}

	FVector2D LookAxisVector = Value.Get<FVector2D>();

	if (OwnerChar)
	{
		OwnerChar->Look(LookAxisVector);
	}
}


void AT3PlayerController::Input_LockOn(const FInputActionValue& Value)
{
	if (bIsInventoryOpen || !OwnerChar->CanExecuteAction())
	{
		return;
	}
	
	if (Combat)
	{
		Combat->ToggleLockOn();
	}
}

void AT3PlayerController::Input_BlockStart(const FInputActionValue& Value)
{
	if (bIsInventoryOpen || !OwnerChar->CanExecuteAction())
	{
		return;
	}
	
	if (Combat)
	{
			Combat->StartBlock();
	}
}

void AT3PlayerController::Input_BlockEnd(const FInputActionValue& Value)
{
	if (bIsInventoryOpen || !OwnerChar->CanExecuteAction())
	{
		return;
	}
	
	if (Combat)
	{
			Combat->EndBlock();
	}
}



void AT3PlayerController::Input_Roll(const FInputActionValue& Value)
{
	if (bIsInventoryOpen || !OwnerChar->CanExecuteAction())
	{
		return;
	}
	
	if (OwnerChar)
	{
		OwnerChar->Roll(Value);
	}
}

void AT3PlayerController::Input_Interact(const FInputActionValue& Value)
{
	if (bIsInventoryOpen || !OwnerChar->CanExecuteAction())
	{
		return;
	}
	
	// TODO: 상호작용 시스템 연결
}

void AT3PlayerController::Input_Test(const FInputActionValue& Value)
{
	if (bIsInventoryOpen || !OwnerChar->CanExecuteAction())
	{
		return;
	}
	
	if (OwnerChar)
	{
		if (OwnerChar->PlayerInputState.bIsCombatState == false)
		{
			OwnerChar->PlayerInputState.bIsCombatState = true;
		}
		else
		{
			OwnerChar->PlayerInputState.bIsCombatState = false;
		}
		
	}
}

void AT3PlayerController::Input_Attack(const FInputActionValue& Value)
{
	if (bIsInventoryOpen || !OwnerChar->CanExecuteAction())
	{
		return;
	}
	
	if (Combat)
	{
			Combat->Attack();
	}
}

void AT3PlayerController::ToggleInventoryInput()
{
	if (IsValid(MainInventoryWidget))
	{
		if (MainInventoryWidget->IsVisible())
		{
			MainInventoryWidget->SetVisibility(ESlateVisibility::Collapsed);
			
			FInputModeGameOnly InputModeGameOnly;
			SetInputMode(InputModeGameOnly);
			
			SetShowMouseCursor(false);
			SetInventoryOpen(false);
		}
		else
		{
			MainInventoryWidget->SetVisibility(ESlateVisibility::Visible);
			
			FInputModeGameAndUI InputModeGameAndUI;
			SetInputMode(InputModeGameAndUI);
			
			SetShowMouseCursor(true);
			SetInventoryOpen(true);
		}
	}
}


void AT3PlayerController::Input_ChangeSkillSlot(const FInputActionValue& Value)
{	
	if (bIsInventoryOpen || !OwnerChar->CanExecuteAction())	{return;}
	if (Combat)	{Combat->ChangeActiveSlot(ESlotType::Skill);}
}

void AT3PlayerController::Input_ChangePotionSlot(const FInputActionValue& Value)
{
	if (bIsInventoryOpen || !OwnerChar->CanExecuteAction()) { return; }
	
	if (IsValid(OwnerChar))
	{
		OwnerChar->InventoryComponent->SwapHPMPSlot();
	}
}

void AT3PlayerController::Input_ChangeConsumableSlot(const FInputActionValue& Value)
{
	if (!OwnerChar->CanExecuteAction()) { return; }
	
	if (IsValid(OwnerChar))
	{
		OwnerChar->InventoryComponent->SwapEquippedItem();
	}
}

void AT3PlayerController::Input_ActiveSkillSlot(const FInputActionValue& Value)
{
	if (bIsInventoryOpen || !OwnerChar->CanExecuteAction()) { return; }
	if (Combat) { Combat->ExecuteCurrentSlotAction(ESlotType::Skill); } 
}

void AT3PlayerController::Input_ActivePotionSlot(const FInputActionValue& Value)
{
	if (bIsInventoryOpen || !OwnerChar->CanExecuteAction()) { return; }
	
	if (IsValid(OwnerChar))
	{
		OwnerChar->InventoryComponent->UseCurrentPotion();
	}
}

void AT3PlayerController::Input_ActiveConsumableSlot(const FInputActionValue& Value)
{
	if (bIsInventoryOpen || !OwnerChar->CanExecuteAction()) { return; }
	
	if (IsValid(OwnerChar))
	{
		OwnerChar->InventoryComponent->UseEquippedItem();
	}
}

void AT3PlayerController::SetInventoryOpen(bool bIsOpen)
{
	bIsInventoryOpen = bIsOpen;
}


