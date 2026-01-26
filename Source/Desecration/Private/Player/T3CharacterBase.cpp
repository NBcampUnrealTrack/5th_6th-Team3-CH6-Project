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
	CurrentHP = MaxHP;

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
	
	float CurrentAcceleration = GetCharacterMovement()->GetCurrentAcceleration().Size();
	PlayerInputState.bWantsToMove = CurrentAcceleration > KINDA_SMALL_NUMBER;
	
	float CurrentGroundSpeed = GetVelocity().Size2D();
	const float MoveThreshold = 3.0f;
	
	PlayerInputState.bIsMoving = CurrentGroundSpeed > (MoveThreshold);
	
	PlayerInputState.CurrentSpeed = CurrentGroundSpeed;
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
	if (GetCurrentStamina() < 20.f) return; // 스태미나 부족 시 실행 불가
	
	if (PlayerInputState.bWantsToRoll == false)
	{

		// 스태미나 20 차감 및 설정
		float NewStamina = FMath::Max(0.f, GetCurrentStamina() - 20.f);
		SetCurrentStamina(NewStamina);

		// 현재 스태미너 로그 출력
		GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Green,
			FString::Printf(TEXT("Dodge! Remaining Stamina: %.1f / %.1f"), NewStamina, GetMaxStamina()));
		
		PlayerInputState.bWantsToRoll = true;
		
		float CurrentAngle = PlayerInputState.InputYawOffset;
		PlayerInputState.RollDirection = GetRollDirection(CurrentAngle);
		
		// 0.5초간 무적 상태 활성화 
		SetIsInvincible(true, 0.5f);

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

// 호출용 이동속도 버프 함수 (이동속도 배율, 지속시간)
void AT3CharacterBase::SetMoveSpeedTemporary(float NewSpeed, float Duration)
{
	if (!GetCharacterMovement()) return;

	// 기존에 돌고 있던 복구 타이머가 있다면 취소 (새로운 버프/디버프 갱신)
	if (GetWorldTimerManager().IsTimerActive(SpeedResetTimerHandle))
	{
		GetWorldTimerManager().ClearTimer(SpeedResetTimerHandle);
	}
	else
	{
		// 처음 속도를 바꾸는 것이라면 현재 속도를 저장해둠
		OriginalMoveSpeed = GetCharacterMovement()->MaxWalkSpeed;
	}

	// 속도 적용
	GetCharacterMovement()->MaxWalkSpeed *= NewSpeed;

	if (Duration > 0.f)
	{
		// Duration 후에 ResetMoveSpeed 호출
		GetWorldTimerManager().SetTimer(
			SpeedResetTimerHandle,
			this,
			&AT3CharacterBase::ResetMoveSpeed,
			Duration,
			false
		);
	}
}

void AT3CharacterBase::ResetMoveSpeed()
{
	if (GetCharacterMovement())
	{
		GetCharacterMovement()->MaxWalkSpeed = OriginalMoveSpeed;
		UE_LOG(LogTemp, Log, TEXT("MoveSpeed Restored to: %f"), OriginalMoveSpeed);
	}
}

float AT3CharacterBase::GetMoveSpeed() const
{
	if (GetCharacterMovement())
	{
		return GetCharacterMovement()->MaxWalkSpeed;
	}
	return 0.f;
}

void AT3CharacterBase::SetMoveSpeed(float NewSpeed)
{
	if (auto* Movement = GetCharacterMovement())
	{
		Movement->MaxWalkSpeed = NewSpeed;
	}
}

void AT3CharacterBase::SetIsInvincible(bool bNewInvincible, float Duration)
{
	// 기존 타이머가 돌고 있다면 안전하게 취소
	GetWorldTimerManager().ClearTimer(InvincibleTimerHandle);

	bIsInvincible = bNewInvincible;

	if (bIsInvincible)
	{
		UE_LOG(LogTemp, Log, TEXT("무적 활성화"));

		if (Duration > 0.f)
		{
			GetWorldTimerManager().SetTimer(InvincibleTimerHandle, FTimerDelegate::CreateLambda([this]()
				{
					SetIsInvincible(false); // 람다를 사용하여 간결하게 해제 함수 호출
				}), Duration, false);
		}
	}
	else
	{
		UE_LOG(LogTemp, Log, TEXT("무적 비활성화."));
	}
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
