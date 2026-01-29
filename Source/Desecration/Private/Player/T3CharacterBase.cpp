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
#include "Player/T3CharacterDataAsset.h"
#include "Player/T3DamageTypes.h"



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
	CurrentStamina = MaxStamina;

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

	if (CharacterData)
	{
		ApplyCharacterData(CharacterData);
	}

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
	
	float CurrentGroundSpeed = GetVelocity().Size2D();
	PlayerInputState.CurrentSpeed = CurrentGroundSpeed;
	FVector InputVector = GetLastMovementInputVector();
	float FutureSpeed = FMath::Min(InputVector.Size2D(), 1.0f) * (GetCharacterMovement()->MaxWalkSpeed);
	PlayerInputState.FutureSpeed = FutureSpeed;
	PlayerInputState.bWantsToMove = (InputVector.Size()>KINDA_SMALL_NUMBER) && (FutureSpeed >= (CurrentGroundSpeed +100));
	
	const float MoveThreshold = 3.0f;
	
	PlayerInputState.bIsMoving = CurrentGroundSpeed > MoveThreshold;
	PlayerInputState.bIsInAir = GetCharacterMovement()->IsFalling();
	
	if (GetCharacterMovement()->MaxWalkSpeed > 400.0f)
	{
		PlayerInputState.T3GaitState = EGaitState::Run;
	}
	else
	{
		PlayerInputState.T3GaitState = EGaitState::Walk;
	}
	
	if (!PlayerInputState.bIsMoving && !PlayerInputState.bWantsToMove)
	{
		PlayerInputState.T3GaitState = EGaitState::Idle;
	}
	
	if (GEngine)

	{
		FString DebugMsg = FString::Printf(TEXT("Current Speed: %f / PlayerInputStateCurrentSpeed : %f"), CurrentGroundSpeed, PlayerInputState.CurrentSpeed);

		GEngine->AddOnScreenDebugMessage(1, 2.0f, FColor::Green, DebugMsg);
	}
	
}

void AT3CharacterBase::OnMovementModeChanged(EMovementMode PrevMovementMode, uint8 PreviousCustomMode)
{
	Super::OnMovementModeChanged(PrevMovementMode, PreviousCustomMode);
	
	bool bCurrentOnGround = GetCharacterMovement()->IsMovingOnGround();
	
	if (PrevMovementMode == MOVE_Falling && bCurrentOnGround)
	{
		PlayerInputState.bIsJustLanded = true;
		
		FTimerHandle LandTimerHandle;
		GetWorld()->GetTimerManager().SetTimer(LandTimerHandle, [this]()
		{
			PlayerInputState.bIsJustLanded = false;
		}, 0.2f, false);
	}
}


void AT3CharacterBase::ApplyCharacterData(UT3CharacterDataAsset* Data)
{
	if (!Data) return;

	// 1. 외형 변경
	if (GetMesh() && Data->CharacterMesh)
	{
		GetMesh()->SetSkeletalMesh(Data->CharacterMesh);
	}

	// 2. 무기 장착 (CombatComponent에게 위임)
	if (CombatComponent)
	{
		CombatComponent->InitializeWeapons(Data->WeaponMap);
	}

	// 3. 스탯 설정
	MaxHP = Data->MaxHealth;
	CurrentHP = MaxHP;

	// 4. 스킬 컴포넌트 부착
	if (Data->SkillComponent)
	{
		UActorComponent* ExistingComp = GetComponentByClass(Data->SkillComponent);
		if (!ExistingComp)
		{
			UActorComponent* NewSkillComp = NewObject<UActorComponent>(this, Data->SkillComponent);
			if (NewSkillComp)
			{
				NewSkillComp->RegisterComponent();
			}
		}
	}
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
	TObjectPtr<UT3CombatComponent> Combat = GetCombatComponent();
	if (GetCurrentStamina() < 20.f) return; // 스태미나 부족 시 실행 불가
	
	if (PlayerInputState.bWantsToRoll == false)
	{

		// 스태미나 20 차감
		Combat->ConsumeStamina(20);

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
	if (!CombatComponent || !bCanRegenStamina) return;

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

}

// 호출용 마나 회복 함수
void AT3CharacterBase::RestoreMP(float Amount)
{
	if (Amount <= 0.f) return;

	AddMP(Amount);
}

// 호출용 이동속도 버프 함수 (이동속도 배율, 지속시간)
void AT3CharacterBase::SetMoveSpeedTemporary(float NewSpeedMultiflier, float Duration)
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
	GetCharacterMovement()->MaxWalkSpeed *= NewSpeedMultiflier;

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

float AT3CharacterBase::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* InstigatedBy, AActor* DamageCauser)
{
	float ActualDamage = Super::TakeDamage(DamageAmount, DamageEvent, InstigatedBy, DamageCauser);

	EHitIntensity ReceivedIntensity = EHitIntensity::Light;
	if (DamageEvent.GetTypeID() == FT3DamageEvent::ClassID)
	{
		const FT3DamageEvent* T3Event = static_cast<const FT3DamageEvent*>(&DamageEvent);
		ReceivedIntensity = T3Event->HitIntensity;
	}

	if (CombatComponent)
	{
		const UDamageType* DamageTypePtr = DamageEvent.DamageTypeClass ? DamageEvent.DamageTypeClass->GetDefaultObject<UDamageType>() : nullptr;

		CombatComponent->ExecuteHitLogic(DamageCauser, ActualDamage, DamageTypePtr, InstigatedBy, ReceivedIntensity);
	}

	return ActualDamage;
}