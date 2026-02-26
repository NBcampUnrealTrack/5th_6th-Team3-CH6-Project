// T3CharacterBase.cpp


#include "Player/T3CharacterBase.h"
//#include "SNegativeActionButton.h"
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
#include "Player/T3SkillComponentBase.h"
#include "Equipment/T3PlayerEquipmentComponent.h"
#include "GameSystem/T3GameMode.h"
#include "Player/T3PlayerController.h"
#include "UI/T3HUDSlotWidget.h"
#include "Player/T3HolyGaugeWidget.h"


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
	CurrentMana = MaxMana;

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
	EquipComp = CreateDefaultSubobject<UT3PlayerEquipmentComponent>(TEXT("EquipmentComponent"));
	
	LoadTimeAfterDeath = 3.0f;
}

void AT3CharacterBase::RequestSellItem(const FInventorySlot& SlotData, const int32& Count)
{
	OnSellItemRequested.Broadcast(SlotData, Count);
}

//void AT3CharacterBase::BeginPlay()
//{
//	Super::BeginPlay();
//
//	if (CharacterData)
//	{
//		ApplyCharacterData(CharacterData);
//	}
//	
//	//캐릭터 정보 세팅
//	if (const TObjectPtr<AT3GameMode> T3GameMode = Cast<AT3GameMode>(GetWorld()->GetAuthGameMode()))
//	{
//		T3GameMode->SetCharacterBySavedData(this);
//	}
//
//	// 스태미너 자동 회복
//		GetWorldTimerManager().SetTimer(
//		StaminaRegenTimerHandle,
//		this,
//		&AT3CharacterBase::RegenerateStamina,
//		StaminaRegenInterval,
//		true
//	);
//
//		// 시작 시 전투모드 활성화
//		PlayerInputState.bIsCombatState = true;
//
//		
//		EquipComp->OnEquipmentStatsChanged.AddDynamic(this, &AT3CharacterBase::OnEquipmentStatsUpdated);
//
//
//}


void AT3CharacterBase::BeginPlay()
{
	Super::BeginPlay();

	UWorld* World = GetWorld();
	if (!World) return; // 월드 유효성 검사 추가

	if (CharacterData)
	{
		ApplyCharacterData(CharacterData);
	}

	// GameMode 참조 안전하게 수정
	if (AT3GameMode* T3GameMode = Cast<AT3GameMode>(World->GetAuthGameMode()))
	{
		T3GameMode->SetCharacterBySavedData(this);
	}

	// 타이머 및 변수 체크
	if (StaminaRegenInterval > 0.0f)
	{
		World->GetTimerManager().SetTimer(
			StaminaRegenTimerHandle,
			this,
			&AT3CharacterBase::RegenerateStamina,
			StaminaRegenInterval,
			true
		);
	}

	PlayerInputState.bIsCombatState = true;

	// 컴포넌트 유효성 검사 필수
	if (EquipComp)
	{
		EquipComp->OnEquipmentStatsChanged.AddDynamic(this, &AT3CharacterBase::OnEquipmentStatsUpdated);
	}
}

void AT3CharacterBase::OnEquipmentStatsUpdated(float Atk, float Def)
{
	SetAttackPower(Atk);
	SetDefense(Def * 0.01);

	UE_LOG(LogTemp, Display, TEXT("Atk : %.1f, Def : %.1f"), AttackPower, Defense);
}


void AT3CharacterBase::Tick(float DeltaTime)
{

	Super::Tick(DeltaTime);

	// 강제 이동
	if (bIsForcedMoving)
	{
		UpdateForcedMovement(DeltaTime);
	}

	// 강제 이동 종료 후 강제 회전
	if (bIsRotatingToTarget)
	{
		UpdateForcedRotation(DeltaTime);
	}


	float CurrentGroundSpeed = GetVelocity().Size2D();
	PlayerInputState.CurrentSpeed = CurrentGroundSpeed;

	FVector InputVector = GetLastMovementInputVector();
	float FutureSpeed = FMath::Min(InputVector.Size2D(), 1.0f) * (GetCharacterMovement()->MaxWalkSpeed);
	PlayerInputState.FutureSpeed = FutureSpeed;

	PlayerInputState.bWantsToMove = (InputVector.Size() > KINDA_SMALL_NUMBER) && (FutureSpeed >= (CurrentGroundSpeed + 10.f));
	PlayerInputState.bIsMoving = CurrentGroundSpeed > 3.0f;
	PlayerInputState.bIsInAir = GetCharacterMovement()->IsFalling();
	PlayerInputState.bWantsToStop = PlayerInputState.bIsMoving && (FutureSpeed < KINDA_SMALL_NUMBER);
	PlayerInputState.T3GaitState = (GetCharacterMovement()->MaxWalkSpeed > 400.0f) ? EGaitState::Run : EGaitState::Walk;
	}

void AT3CharacterBase::UpdateForcedMovement(float DeltaTime)
{
	FVector CurrentLocation = GetActorLocation();

	// 이번 프레임에 이동해야 할 거리
	float MoveStep = ForcedMoveSpeed * DeltaTime;
	// 목적지까지 남은 거리
	float DistanceToTarget = FVector::Dist(CurrentLocation, ForcedTargetLocation);

	if (DistanceToTarget <= MoveStep)
	{
		SetActorLocation(ForcedTargetLocation, true);
		StopForcedMove();
		return; // 즉시 종료하여 아래 로직 실행 방지
	}

	// 1. 위치 이동: Sweep을 true로 설정하여 장애물 충돌 감지
	FVector NewLocation = FMath::VInterpConstantTo(CurrentLocation, ForcedTargetLocation, DeltaTime, ForcedMoveSpeed);

	// bSweep을 true로 주어야 벽을 뚫고 지나가지 않습니다.
	FHitResult Hit;
	SetActorLocation(NewLocation, true, &Hit);

	// 2. 회전 처리: 목적지를 부드럽게 바라보기
	FRotator CurrentRot = GetActorRotation();
	FRotator TargetRot = UKismetMathLibrary::FindLookAtRotation(CurrentLocation, ForcedTargetLocation);
	TargetRot.Pitch = 0.f;
	TargetRot.Roll = 0.f;


	SetActorRotation(FMath::RInterpTo(CurrentRot, TargetRot, DeltaTime, 20.f));

	// 컨트롤러 시점(카메라) 동기화
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		FRotator CurrentControlRot = PC->GetControlRotation();
		// 카메라 시점도 목적지를 향해 부드럽게 회전
		FRotator NewControlRot = FMath::RInterpTo(CurrentControlRot, TargetRot, DeltaTime, 15.f);
		PC->SetControlRotation(NewControlRot);
	}

	// 3. 애니메이션 연동: CharacterMovement의 속도값을 강제로 갱신
	// 이 작업이 있어야 AnimBP의 GroundSpeed가 계산되어 달리기 모션이 출력됩니다.
	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		MoveComp->Velocity = GetActorForwardVector() * ForcedMoveSpeed;
		GetCharacterMovement()->MaxWalkSpeed = ForcedMoveSpeed;
		// 이동 중 낙하 상태 등을 방지하기 위해 이동 모드를 고정할 수도 있습니다.
		// MoveComp->SetMovementMode(MOVE_Walking); 
	}

	// 4. 도착 체크 및 종료
	float DistanceSq = FVector::DistSquared(CurrentLocation, ForcedTargetLocation);
	if (DistanceSq < FMath::Square(100.f))
	{
		StopForcedMove();
	}
}

void AT3CharacterBase::UpdateForcedRotation(float DeltaTime)
{
	FRotator CurrentRot = GetActorRotation();
	// 목표 회전값으로 부드럽게 보간
	FRotator NewRot = FMath::RInterpTo(CurrentRot, ForcedTargetRotation, DeltaTime, 3.f);
	SetActorRotation(NewRot);

	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		PC->SetControlRotation(NewRot);
	}

	// 거의 다 돌아갔으면 회전 모드 종료 및 입력 복구
	if (CurrentRot.Equals(ForcedTargetRotation, 1.0f))
	{
		bIsRotatingToTarget = false;
		PlayerInputState.bIsCombatState = true;
		if (APlayerController* PC = Cast<APlayerController>(GetController()))
		{
			PC->SetIgnoreMoveInput(false);
			PC->ResetIgnoreInputFlags(); // 시점 제한까지 모두 해제
			GetCharacterMovement()->MaxWalkSpeed = DefaultMaxWalkSpeed; // 속도 원상 복구
		}

		if (OnForcedMoveEnd.IsBound())
		{
			OnForcedMoveEnd.Broadcast();
		}
	}
}

void AT3CharacterBase::StartForcedMove(FVector TargetLocation, FRotator TargetRotation, float Speed)
{
	DefaultMaxWalkSpeed = GetCharacterMovement()->MaxWalkSpeed;
	ForcedTargetLocation = TargetLocation;
	ForcedTargetRotation = TargetRotation;
	bIsForcedMoving = true;
	ForcedMoveSpeed = Speed;
	PlayerInputState.bIsCombatState = false;

	// 이동 중에는 플레이어의 입력을 막음
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		//PC->SetIgnoreMoveInput(true);
	}
}

void AT3CharacterBase::StopForcedMove()
{
	bIsForcedMoving = false;
	bIsRotatingToTarget = true;

	// 이동 중단 시 속도 초기화 (안 하면 미끄러질 수 있음)
	// if (GetCharacterMovement())
	// {
	// 	GetCharacterMovement()->Velocity = FVector::ZeroVector;
	// 	GetCharacterMovement()->MaxWalkSpeed = DefaultMaxWalkSpeed;
	// 	GetCharacterMovement()->StopMovementImmediately();
	// }
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
	
	// 0. 클래스 저장
	CurrentClass = Data->CharacterClass;
	
	FString ClassName = UEnum::GetDisplayValueAsText(CurrentClass).ToString();
	UE_LOG(LogTemp, Log, TEXT("Your Class is: %s"), *ClassName);

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
			UT3SkillComponentBase* NewSkillComp = NewObject<UT3SkillComponentBase>(this, Data->SkillComponent);
			if (NewSkillComp)
			{
				NewSkillComp->RegisterComponent();
				
				if (CombatComponent)
				{
					CombatComponent->SetSkillComponent(NewSkillComp);
				}

				FTimerHandle WidgetInitTimerHandle;
				GetWorldTimerManager().SetTimer(WidgetInitTimerHandle, [this, NewSkillComp]()
					{
						if (AT3PlayerController* PC = GetController<AT3PlayerController>())
						{
							if (PC->HUDSlotWidget)
							{
								PC->HUDSlotWidget->InitializeWidget(NewSkillComp);
								
								
								if (CurrentClass == ECharacterClass::Paladin)
								{
								PC->HolyGaugeWidget->InitializeWidget(NewSkillComp);
								}
								UE_LOG(LogTemp, Log, TEXT("Delayed Widget Initialization Success!"));
							}
						}
					}, 1.0f, false);
			}
		}
	}
}

void AT3CharacterBase::Move(const FVector2D& Value)
{
	if (bMoveLock) return;
	
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
	if (bCameraLock) return;
	
	AddControllerYawInput(Value.X);
	AddControllerPitchInput(Value.Y);
}

void AT3CharacterBase::Roll(const FInputActionValue& Value)
{
	TObjectPtr<UT3CombatComponent> Combat = GetCombatComponent();
	if (!Combat || GetCurrentStamina() < 20.f)
		return; //GEngine->AddOnScreenDebugMessage(-1,1.f,FColor::Emerald,FString::Printf(TEXT("You Need Stamina"))); // 스태미나 부족 시 실행 불가
	
	OnWakeUp();
	
	if (PlayerInputState.bWantsToRoll == false && bIsLying == false && bIsKnockback == false)
	{

		// 스태미나 20 차감
		Combat->ConsumeStamina(20);

		PlayerInputState.bWantsToRoll = true;
		
		float CurrentAngle = PlayerInputState.InputYawOffset;
		PlayerInputState.RollDirection = GetRollDirection(CurrentAngle);

		OnRollTriggered();



		// 팔라딘의 경우 신의 심판 시전 중 구르면 스킬 캔슬
		if (GetCurrentClass() == ECharacterClass::Paladin)
		{
				Combat->GetSkillComponent()->CancelCurrentSkill();
		}
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

// 통합 스탯 델리게이트 함수
void AT3CharacterBase::BroadcastStatChange(ET3StatType StatType)
{
	if (!OnStatChanged.IsBound()) return;

	switch (StatType)
	{
	case ET3StatType::HP:
		OnStatChanged.Broadcast(StatType, CurrentHP, MaxHP);
		break;
	case ET3StatType::MP:
		OnStatChanged.Broadcast(StatType, CurrentMana, MaxMana);
		break;
	case ET3StatType::Stamina:
		OnStatChanged.Broadcast(StatType, CurrentStamina, MaxStamina);
		break;
	case ET3StatType::Attack:
		OnStatChanged.Broadcast(StatType, AttackPower, -1.f); // 최대값이 없는 스탯은 -1 전달
		break;
	case ET3StatType::Defense:
		OnStatChanged.Broadcast(StatType, Defense, -1.f);
		break;
	case ET3StatType::MoveSpeed:
		OnStatChanged.Broadcast(StatType, GetMoveSpeed(), -1.f);
		break;
	default:
		break;
	}
}


// 회복 함수

// 스테미너 자연 회복
void AT3CharacterBase::RegenerateStamina()
{
	if (!CombatComponent) return;

	if (!bCanRegenStamina || PlayerInputState.bIsBlocking) // 공격, 구르기, 막기 중 스태미너 소량 회복
	{
		AddStamina(StaminaRegenLowRate * StaminaRegenInterval);
	}

	else if (CurrentStamina < MaxStamina)
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

void AT3CharacterBase::ResetMoveSpeed()
{
	if (GetCharacterMovement())
	{
		GetCharacterMovement()->MaxWalkSpeed = OriginalMoveSpeed;
		BroadcastStatChange(ET3StatType::Stamina);
		UE_LOG(LogTemp, Log, TEXT("MoveSpeed Restored to: %f"), OriginalMoveSpeed);
	}
}

void AT3CharacterBase::OnDeath()
{
	bMoveLock = true;
	OnDeathAnimation();
	
	//GetWorld()->GetTimerManager().SetTimer(AfterDeathTimerHandle, FTimerDelegate::CreateLambda([&]()
	//{
	//	if (const TObjectPtr<AT3GameMode> T3GameMode = Cast<AT3GameMode>(GetWorld()->GetAuthGameMode()))
	//	{
	//		T3GameMode->LoadGame();
	//	}
	//}), LoadTimeAfterDeath, false);
}

void AT3CharacterBase::ConsumeMana(float Amount)
{
	if (CurrentMana >= Amount)
	{
		float NewMana = CurrentMana - Amount;
		SetCurrentMana(NewMana);
		
		//GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Cyan,
		//FString::Printf(TEXT("Remaining Mana: %.1f"), CurrentMana));
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
		BroadcastStatChange(ET3StatType::MoveSpeed);
	}
}

// 내부 수치 합산 및 제한 로직
void AT3CharacterBase::AddHP(float Amount)
{
	CurrentHP = FMath::Clamp(CurrentHP + Amount, 0.f, MaxHP);
	BroadcastStatChange(ET3StatType::HP);
}

void AT3CharacterBase::AddMP(float Amount)
{
	CurrentMana = FMath::Clamp(CurrentMana + Amount, 0.f, MaxMana);
	BroadcastStatChange(ET3StatType::MP);
}

void AT3CharacterBase::AddStamina(float Amount)
{
	CurrentStamina = FMath::Clamp(CurrentStamina + Amount, 0.f, MaxStamina);
	BroadcastStatChange(ET3StatType::Stamina);
}

float AT3CharacterBase::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* InstigatedBy, AActor* DamageCauser)
{
	float ActualDamage = Super::TakeDamage(DamageAmount, DamageEvent, InstigatedBy, DamageCauser);
	
	EHitIntensity ReceivedIntensity = EHitIntensity::Light;
	float ReceievedDamageMultiplier = 1.0f;
	if (DamageEvent.GetTypeID() == FT3DamageEvent::ClassID)
	{
		const FT3DamageEvent* T3Event = static_cast<const FT3DamageEvent*>(&DamageEvent);
		ReceivedIntensity = T3Event->HitIntensity;
		ReceievedDamageMultiplier = T3Event->HitDamageMultiplier;
	}

	if (CombatComponent)
	{
		const UDamageType* DamageTypePtr = DamageEvent.DamageTypeClass ? DamageEvent.DamageTypeClass->GetDefaultObject<UDamageType>() : nullptr;

		CombatComponent->ExecuteHitLogic(DamageCauser, ActualDamage, DamageTypePtr, InstigatedBy, ReceivedIntensity, ReceievedDamageMultiplier);
		OnHit();
	}

	return ActualDamage;
}

bool AT3CharacterBase::CanExecuteAction() const
{
	return !bIsForcedMoving;
}