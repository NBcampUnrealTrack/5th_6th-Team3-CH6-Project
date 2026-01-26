// T3CharacterBase.cpp


#include "Player/T3CharacterBase.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "Player/T3CombatComponent.h"
#include "Item/Component/T3InventoryComponent.h"
#include "Item/Component/T3ItemUseComponent.h"



AT3CharacterBase::AT3CharacterBase()
{
	PrimaryActorTick.bCanEverTick = true;
	
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, -1.0f, 0.0f); 
	GetCharacterMovement()->JumpZVelocity = 500.f;
	GetCharacterMovement()->AirControl = 0.35f;
	GetCharacterMovement()->MaxWalkSpeed = 500.f;
	GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 400.0f;
	CameraBoom->bUsePawnControlRotation = true;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	CombatComponent = CreateDefaultSubobject<UT3CombatComponent>(TEXT("CombatComponent"));

	InventoryComponent = CreateDefaultSubobject<UT3InventoryComponent>(TEXT("InventoryComponent")); 
	ItemUseComponent = CreateDefaultSubobject<UT3ItemUseComponent>(TEXT("ItemUseComponent"));
}

void AT3CharacterBase::BeginPlay()
{
	Super::BeginPlay();

	// 스태미너 자동 회복
		GetWorldTimerManager().SetTimer(
		StaminaRegenTimerHandle,
		this,
		&AT3CharacterBase::RegenerateStamina,
		StaminaRegenInterval,
		true
	);

}

void AT3CharacterBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	
	float CurrentAccelerationSq = GetCharacterMovement()->GetCurrentAcceleration().Size();
	PlayerInputState.bWantsToMove = CurrentAccelerationSq > KINDA_SMALL_NUMBER;
	
	float CurrentGroundSpeedSq = GetVelocity().Size2D();
	const float MoveThreshold = 3.0f;
	
	PlayerInputState.bIsMoving = CurrentGroundSpeedSq > (MoveThreshold);
	
	PlayerInputState.CurrentSpeed = CurrentGroundSpeedSq;
}


void AT3CharacterBase::Move(const FVector2D& Value)
{
	if (Controller != nullptr)
	{
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
		
		FVector WorldDirection = GetLastMovementInputVector();
		if (!WorldDirection.IsZero())
		{
			FRotator TargetRot = WorldDirection.Rotation();
			FRotator CurrentRot = GetActorRotation();
			
			FRotator DeltaRot = UKismetMathLibrary::NormalizedDeltaRotator(TargetRot, CurrentRot);
			
			PlayerInputState.InputYawOffset = DeltaRot.Yaw;
		}
		else
		{
			PlayerInputState.InputYawOffset = 0.0f;
		}

		AddMovementInput(ForwardDirection, Value.X);
		AddMovementInput(RightDirection, Value.Y);
	}
}

void AT3CharacterBase::Look(const FVector2D& Value)
{
	AddControllerYawInput(Value.X);
	AddControllerPitchInput(Value.Y);
}

void AT3CharacterBase::Roll(const FInputActionValue& Value)
{
	if (PlayerInputState.bWantsToRoll == false)
	{
		PlayerInputState.bWantsToRoll = true;
		
		float CurrentAngle = PlayerInputState.InputYawOffset;
		PlayerInputState.RollDirection = GetRollDirection(CurrentAngle);
		
		OnRollTriggered();
	}
}

ERollDirection AT3CharacterBase::GetRollDirection(float Angle) const
{
	if (GetLastMovementInputVector().IsZero()) return ERollDirection::Neutral;
	
	if (Angle >= -22.5f && Angle < 22.5f) return ERollDirection::Forward;
	if (Angle >= 22.5f && Angle < 67.5f) return ERollDirection::ForwardRight;
	if (Angle >= 67.5f && Angle < 112.5f) return ERollDirection::Right;
	if (Angle >= 112.5f && Angle < 157.5f) return ERollDirection::BackRight;
	if (Angle >= -67.5f && Angle < -22.5f) return ERollDirection::ForwardLeft;
	if (Angle >= -112.5f && Angle < -67.5f) return ERollDirection::Left;
	if (Angle >= -157.5f && Angle < -112.5f) return ERollDirection::BackLeft;

	return ERollDirection::Back;
}

// 회복 함수

// 스테미너 자연 회복
void AT3CharacterBase::RegenerateStamina()
{
	if (CurrentStamina < MaxStamina)
	{
		AddStamina(StaminaRegenRate * StaminaRegenInterval);
	}
}

// 호출용 체력 회복 함수
void AT3CharacterBase::RestoreHP(float HealAmount)
{
	if (HealAmount <= 0.f) return;

	AddHP(HealAmount);

	UE_LOG(LogTemp, Log, TEXT("HP Restored: %f / Current: %f"), HealAmount, CurrentHP);
}

// 호출용 마나 회복 함수
void AT3CharacterBase::RestoreMP(float Amount)
{
	if (Amount <= 0.f) return;

	AddMP(Amount);
}

// 내부 수치 합산 및 제한 로직
void AT3CharacterBase::AddHP(float Amount)
{
	CurrentHP = FMath::Clamp(CurrentHP + Amount, 0.f, MaxHP);
}

void AT3CharacterBase::AddMP(float Amount)
{
	CurrentMana = FMath::Clamp(CurrentMana + Amount, 0.f, MaxMana);
}

void AT3CharacterBase::AddStamina(float Amount)
{
	CurrentStamina = FMath::Clamp(CurrentStamina + Amount, 0.f, MaxStamina);
}
