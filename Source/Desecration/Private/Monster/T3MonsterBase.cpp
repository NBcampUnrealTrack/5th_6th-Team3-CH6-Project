// T3MonsterBase.cpp

#include "Monster/T3MonsterBase.h"
#include "Components/CapsuleComponent.h"
#include "Perception/AIPerceptionSystem.h"
#include "Perception/AISense_Touch.h"
#include "Blueprint/UserWidget.h"
#include "Components/WidgetComponent.h"
#include "GameFramework/CharacterMovementComponent.h"


AT3MonsterBase::AT3MonsterBase()
{
	PrimaryActorTick.bCanEverTick = true;

	bIsDead = false;

	// 1. 컴포넌트 생성
	LockOnWidgetComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("LockOnWidget"));

	// 2. 부착 
	LockOnWidgetComponent->SetupAttachment(GetMesh(), TEXT("LockOn_Socket"));

	// 3. 설정
	LockOnWidgetComponent->SetWidgetSpace(EWidgetSpace::World);
	LockOnWidgetComponent->SetVisibility(false);
	LockOnWidgetComponent->SetRelativeLocation(FVector::ZeroVector); // 소켓 위치로 초기화

	if (UCapsuleComponent* Capsule = GetCapsuleComponent())
	{
		float Radius = Capsule->GetScaledCapsuleRadius();
		float TargetScale = Radius * 0.02f;
		LockOnWidgetComponent->SetWorldScale3D(FVector(TargetScale));
	}
}

void AT3MonsterBase::BeginPlay()
{
	Super::BeginPlay();

	// 엔진에 설정된 기본 MaxWalkSpeed를 저장해둡니다.
	if (GetCharacterMovement())
	{
		DefaultMaxWalkSpeed = GetCharacterMovement()->MaxWalkSpeed;
	}

	// 헬스 컴포넌트 부착 및 초기화
	HealthComponent = FindComponentByClass<UT3HealthComponent>();

	if (UCapsuleComponent* Capsule = GetCapsuleComponent())
	{
		Capsule->OnComponentBeginOverlap.AddDynamic(this, &AT3MonsterBase::OnCapsuleBeginOverlap);
	}

	/*
	if (LockOnWidgetComponent && GetMesh())
	{
		// 이미 부착되어 있더라도 안전하게 다시 부착 (KeepRelativeTransform 사용)
		LockOnWidgetComponent->AttachToComponent(GetMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, TEXT("LockOn_Socket"));

		// 위치 초기화 (소켓 정중앙으로)
		LockOnWidgetComponent->SetRelativeLocation(FVector::ZeroVector);

		UE_LOG(LogTemp, Log, TEXT("[Confirmed] LockOnWidget forced to Socket: %s"), *LockOnWidgetComponent->GetAttachSocketName().ToString());
	}
	*/
}

void AT3MonsterBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AT3MonsterBase::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
}

float AT3MonsterBase::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	float ActualDamage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);

	if (HealthComponent)
	{
		HealthComponent->HandleTakeDamage(ActualDamage, DamageEvent, EventInstigator, DamageCauser);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("T3MonsterBase: HealthComponent is null when taking damage."));
	}

	return ActualDamage;
}

float AT3MonsterBase::PlayAnimMontage(UAnimMontage* AnimMontage, float InPlayRate, FName StartSectionName)
{
	// 장신구로 설정한 CurrentAttackRate를 곱해서 최종 속도를 결정합니다.
	float FinalPlayRate = InPlayRate * CurrentAttackRate;

	return Super::PlayAnimMontage(AnimMontage, FinalPlayRate, StartSectionName);
}

void AT3MonsterBase::OnMovementModeChanged(EMovementMode PrevMovementMode, uint8 PreviousCustomMode)
{
	Super::OnMovementModeChanged(PrevMovementMode, PreviousCustomMode);

	if (GetCharacterMovement() && GetCharacterMovement()->MovementMode == EMovementMode::MOVE_Falling)
	{
		StartFallHeight = GetActorLocation().Z;
	}
}

void AT3MonsterBase::Landed(const FHitResult& Hit)
{
	Super::Landed(Hit);

	const float EndFallHeight = GetActorLocation().Z;
	const float FallDistance = StartFallHeight - EndFallHeight;

	if (FallDistance > DeathFallDistance)
	{
		TakeDamage(9999.f, FDamageEvent(), GetController(), this);
	}
}

void AT3MonsterBase::OnCapsuleBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
    if (OtherActor && OtherActor != this && OtherActor->GetOwner() != this)
    {
        // TargetActor
        AActor* TargetActor = OtherActor;

        APawn* PotentialPawn = Cast<APawn>(OtherActor);
        if (!PotentialPawn)
        {
            AActor* OwnerActor = OtherActor->GetOwner();
            if (Cast<APawn>(OwnerActor))
            {
                TargetActor = OwnerActor;
            }
        }

        UAISense_Touch::ReportTouchEvent(
            GetWorld(),
            this,
            TargetActor,
            TargetActor->GetActorLocation()
        );
    }
}

void AT3MonsterBase::SetLockOnWidgetVisible(bool bVisible)
{
	if (LockOnWidgetComponent)
	{
		LockOnWidgetComponent->SetVisibility(bVisible);
	}
}

float AT3MonsterBase::GetCurrentAttackDamage() const
{
	return BaseDamage * DamageMultiplier * LevelMultiplier;
}

// 자신의 헬스 컴포넌트의 최대 체력을 현재 스테이지에 맞게 조정하는 함수
void AT3MonsterBase::UpdateByStage()
{
	if(!HealthComponent)
	{
		UE_LOG(LogTemp, Warning, TEXT("T3MonsterBase: HealthComponent is null when updating by stage."));
		return;
	}

	float StageMultiplier = 1.0f;

	switch (CurrentStage)
	{
	case 0:
			StageMultiplier = 1.0f;
			break;
	case 1:
			StageMultiplier = 1.3f;
			break;
	case 2:
			StageMultiplier = 1.6f;
			break;
	case 3:
			StageMultiplier = 2.0f;	
			break;
	}
	HealthComponent->MaxHP = HealthComponent->BaseHP * StageMultiplier;
	HealthComponent->ResetCurrentHP();
}

float AT3MonsterBase::GetHPPercent() const
{
	if (!HealthComponent || HealthComponent->MaxHP <= 0.f)
	{
		return 0.f;
	}

	return HealthComponent->CurrentHP / HealthComponent->MaxHP;
}

ET3MonsterType AT3MonsterBase::GetMonsterType() const
{
	return MonsterType;
}

void AT3MonsterBase::ApplyBonusDamage(float BonusDamage)
{
	if (HealthComponent)
	{
		FT3DamageEvent DamageEvent;
		HealthComponent->HandleTakeDamage(BonusDamage, DamageEvent, nullptr, nullptr);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("T3MonsterBase: HealthComponent is null when taking damage."));
	}
}

void AT3MonsterBase::SetAnimationSpeedMultiplier(float MoveAnimMultiplier, float AttackAnimMultiplier)
{
	// 1. 배율 데이터 갱신 (단순 덮어쓰기)
	CurrentMoveRate = MoveAnimMultiplier;
	CurrentAttackRate = AttackAnimMultiplier;

	// 2. 이동 속도 동기화
	ApplyCurrentWalkSpeed();

	// 3. 현재 재생 중인 몽타주 속도 즉시 갱신
	// 몽타주는 루핑되거나 긴 시간 재생될 수 있으므로, 호출 즉시 PlayRate를 바꿔줘야 시각적으로 자연스럽습니다.
	if (UAnimInstance* AnimInst = GetMesh()->GetAnimInstance())
	{
		if (UAnimMontage* ActiveMontage = AnimInst->GetCurrentActiveMontage())
		{
			// 특정 타입을 구분하지 않기로 했으므로 AttackAnimMultiplier를 일괄 적용하거나, 
			// 이동 배율과 공격 배율 중 더 낮은(더 느린) 값을 적용하여 시스템의 일관성을 유지할 수 있습니다.
			float FinalRate = AttackAnimMultiplier;

			AnimInst->Montage_SetPlayRate(ActiveMontage, FinalRate);
		}
	}
}

void AT3MonsterBase::ApplyCurrentWalkSpeed()
{
	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		// 장신구 시스템이 이미 조절 중이라 하셨으므로, 
		// 여기서 다시 계산하면 수치가 중첩(0.5 * 0.5 = 0.25)될 위험이 있습니다.
		// 만약 장신구 시스템이 이 함수를 통해서만 속도를 조절하게 하려면 아래 코드를 유지하세요.
		MoveComp->MaxWalkSpeed = DefaultMaxWalkSpeed * CurrentMoveRate;
	}
}