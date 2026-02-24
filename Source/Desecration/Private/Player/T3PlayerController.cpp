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
#include "UI/T3HUDSlotWidget.h"
#include "Player/T3HolyGaugeWidget.h"
#include "UI/T3ShopWidget.h"

void AT3PlayerController::BeginPlay()
{
	Super::BeginPlay();
	
	bShowMouseCursor = false;
	const FInputModeGameOnly InputModeGameOnly;
	SetInputMode(InputModeGameOnly);

	APawn* NewPawn = GetPawn();
	OwnerChar = Cast<AT3CharacterBase>(NewPawn);
	Combat = OwnerChar->GetCombatComponent();


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
		MainInventoryWidget->AddToViewport(99);
	}

	if (IsValid(CombatWidgetClass))
	{
		CombatWidget = CreateWidget<UUserWidget>(this, CombatWidgetClass);

		//CombatWidget->SetVisibility(ESlateVisibility::Collapsed);
		CombatWidget->AddToViewport(98);
	}
	
	if (IsValid(PopUpMenuClass))
	{
		PopUpMenu = CreateWidget<UT3PopUpMenu>(this, PopUpMenuClass);
		PopUpMenu->AddToViewport(100);
	}
	
	if (IsValid(HUDSlotWidgetClass))
	{
		HUDSlotWidget = CreateWidget<UT3HUDSlotWidget>(this, HUDSlotWidgetClass);
		if (HUDSlotWidget)
		{
			HUDSlotWidget->AddToViewport();
		}
	}

	if (IsValid(LockOnWidgetClass))
	{
		LockOnWidget = CreateWidget<UUserWidget>(this, LockOnWidgetClass);
		if (LockOnWidget)
		{
			LockOnWidget->AddToViewport();
			LockOnWidget->SetVisibility(ESlateVisibility::Collapsed); // 평소엔 숨김
		}
	}

	// 팔라딘일 때만 신성게이지 위젯 생성
	if (OwnerChar->GetCurrentClass() == ECharacterClass::Paladin)
	{
		if (IsValid(HolyGaugeWidgetClass))
		{
			HolyGaugeWidget = CreateWidget<UT3HolyGaugeWidget>(this, HolyGaugeWidgetClass);
			if (HolyGaugeWidget)
			{
				HolyGaugeWidget->AddToViewport();
			}
		}
	}


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
	if (bIsInventoryOpen || !OwnerChar->CanExecuteAction() || OwnerChar->bIsSkillCanNotUse)
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
	if (bIsInventoryOpen || !OwnerChar->CanExecuteAction() || OwnerChar->bIsSkillCanNotUse)
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
	if (bIsInventoryOpen || !OwnerChar->CanExecuteAction() || OwnerChar->bIsSkillCanNotUse)
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
	if (bIsInventoryOpen || !OwnerChar->CanExecuteAction() || OwnerChar->bIsSkillCanNotUse)
	{
		return;
	}
	
	if (OwnerChar)
	{
		OwnerChar->OnInteract();
	}
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
	if (bIsInventoryOpen || !OwnerChar->CanExecuteAction() || OwnerChar->bIsSkillCanNotUse)
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
	if (bIsInventoryOpen || !OwnerChar->CanExecuteAction() || OwnerChar->bIsSkillCanNotUse) { return; }
	if (Combat) { Combat->ExecuteCurrentSlotAction(ESlotType::Skill); } 
}

void AT3PlayerController::Input_ActivePotionSlot(const FInputActionValue& Value)
{
	if (bIsInventoryOpen || !OwnerChar->CanExecuteAction() || OwnerChar->bIsSkillCanNotUse) { return; }
	
	if (IsValid(OwnerChar))
	{
		OwnerChar->OnActivatePotion();
	}
}

void AT3PlayerController::Input_ActiveConsumableSlot(const FInputActionValue& Value)
{
	if (bIsInventoryOpen || !OwnerChar->CanExecuteAction() || OwnerChar->bIsSkillCanNotUse) { return; }
	
	if (IsValid(OwnerChar))
	{
		OwnerChar->InventoryComponent->UseEquippedItem();
	}
}

void AT3PlayerController::SetInventoryOpen(bool bIsOpen)
{
	bIsInventoryOpen = bIsOpen;
}

void AT3PlayerController::ShowShopUI(UT3ShopComponent* ShopComp)
{
	if (!IsValid(ShopWidgetClass))
	{
		UE_LOG(LogTemp, Error, TEXT("상점 위젯 할당안됨"));
		return;
	}
	
	ShopWidget = CreateWidget<UT3ShopWidget>(this, ShopWidgetClass);
	
	if (!IsValid(ShopWidget))
	{
		return;
	}
	
	AT3CharacterBase* T3Character = Cast<AT3CharacterBase>(GetPawn());
	
	if (!IsValid(T3Character))
	{
		UE_LOG(LogTemp, Error, TEXT("캐릭터 캐스트 실패"));
		return;
	}
	
	FInputModeGameAndUI InputModeGameAndUI;
    SetInputMode(InputModeGameAndUI);

	ShopWidget->Init(T3Character->InventoryComponent, ShopComp, T3Character);
	ShopWidget->AddToViewport();
	
	SetShowMouseCursor(true);
	SetInventoryOpen(true);
}