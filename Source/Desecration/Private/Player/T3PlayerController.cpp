// T3PlayerController.cpp


#include "Player/T3PlayerController.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Player/T3CharacterBase.h"
#include "Player/T3CombatComponent.h"



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
}

void AT3PlayerController::SetupInputComponent()
{
	APlayerController::SetupInputComponent();

	if (UEnhancedInputComponent* EnhancedInputComponent = CastChecked<UEnhancedInputComponent>(InputComponent))
	{
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AT3PlayerController::Input_Move);
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AT3PlayerController::Input_Look);

		EnhancedInputComponent->BindAction(LockOnAction, ETriggerEvent::Started, this, &AT3PlayerController::Input_LockOn);
		EnhancedInputComponent->BindAction(BlockingAction, ETriggerEvent::Started, this, &AT3PlayerController::Input_Block);
		EnhancedInputComponent->BindAction(RollingAction, ETriggerEvent::Started, this, &AT3PlayerController::Input_Roll);
		EnhancedInputComponent->BindAction(InteractAction, ETriggerEvent::Started, this, &AT3PlayerController::Input_Interact);
		EnhancedInputComponent->BindAction(AttackAction, ETriggerEvent::Triggered, this, &AT3PlayerController::Input_Attack);
	}
}

void AT3PlayerController::Input_Move(const FInputActionValue& Value)
{
	FVector2D MovementVector = Value.Get<FVector2D>();

	if (AT3CharacterBase* T3Char = Cast<AT3CharacterBase>(GetPawn()))
	{
		T3Char->Move(MovementVector);
	}
}

void AT3PlayerController::Input_Look(const FInputActionValue& Value)
{
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	if (AT3CharacterBase* T3Char = Cast<AT3CharacterBase>(GetPawn()))
	{
		T3Char->Look(LookAxisVector);
	}
}


void AT3PlayerController::Input_LockOn(const FInputActionValue& Value)
{
	if (AT3CharacterBase* T3Char = Cast<AT3CharacterBase>(GetPawn()))
	{
		if (auto* Combat = T3Char->GetCombatComponent())
		{
			Combat->ToggleLockOn();
		}
	}
}

void AT3PlayerController::Input_Block(const FInputActionValue& Value)
{
	if (AT3CharacterBase* T3Char = Cast<AT3CharacterBase>(GetPawn()))
	{
		if (auto* Combat = T3Char->GetCombatComponent())
		{
			if (Combat->GetCurrentState() != ECharacterCombatState::Blocking)
			{
				Combat->StartBlock();
			}
			else
			{
				Combat->EndBlock();
			}
		}
	}
}



void AT3PlayerController::Input_Roll(const FInputActionValue& Value)
{
	if (AT3CharacterBase* T3Char = Cast<AT3CharacterBase>(GetPawn()))
	{
		T3Char->Roll(Value);
	}
}

void AT3PlayerController::Input_Interact(const FInputActionValue& Value)
{
	// TODO: 상호작용 시스템 연결
}

void AT3PlayerController::Input_Attack(const FInputActionValue& Value)
{
	if (AT3CharacterBase* T3Char= Cast<AT3CharacterBase>(GetPawn()))
	{
		if (auto* Combat = T3Char->GetCombatComponent())
		{
			Combat->Attack();
		}
	}
}