// T3CombatComponent.cpp

#include "Player/T3CombatComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/KismetMathLibrary.h"
#include "Engine/OverlapResult.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Player/T3DamageTypes.h"
#include "Player/T3CharacterBase.h"
#include "Player/T3PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Player/T3DamageTestActor.h"


UT3CombatComponent::UT3CombatComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UT3CombatComponent::BeginPlay()
{
	Super::BeginPlay();
	OwnerChar = Cast<AT3CharacterBase>(GetOwner());
	if (OwnerChar)
	{
		OwnerPC = OwnerChar->GetController<APlayerController>();
		OwnerChar->OnTakeAnyDamage.AddDynamic(this, &UT3CombatComponent::HandleTakeAnyDamage);
	}
}

// --- 막기 로직 ---
void UT3CombatComponent::StartBlock()
{
	if (OwnerChar->GetCurrentStamina() < 50.f) return GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Red, TEXT("You Need Stamina."));
	if (CurrentState != ECharacterCombatState::Idle) return;
	CurrentState = ECharacterCombatState::Blocking;
	OwnerChar->PlayerInputState.bIsBlocking = true;
	OwnerChar->GetCharacterMovement()->MaxWalkSpeed = 200.0f;

	GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Cyan, TEXT("BlockingMode"));
	// 여기서 몽타주 재생 로직 추가 (CharacterDataAsset 활용)
}

void UT3CombatComponent::EndBlock()
{
	CurrentState = ECharacterCombatState::Idle;
	OwnerChar->PlayerInputState.bIsBlocking = false;
	OwnerChar->GetCharacterMovement()->MaxWalkSpeed = 500.0f;
	GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Red, TEXT("BlockingModeOff"));
}

void UT3CombatComponent::Attack()
{
	OwnerChar->OnAttack();
}

void UT3CombatComponent::SetParryingEnabled(bool bEnabled)
{
	if (CurrentState == ECharacterCombatState::Idle) return;
	CurrentState = bEnabled ? ECharacterCombatState::Parrying : ECharacterCombatState::Blocking;
}


// --- 록온 로직 ---
void UT3CombatComponent::ToggleLockOn()
{
	if (!OwnerChar || !OwnerPC) return;

	if (bIsLockOn)
	{
		ResetLockOn();
		return;
	}

	CurrentTarget = FindBestTarget();
	if (CurrentTarget)
	{
		bIsLockOn = true;
		OwnerChar->PlayerInputState.bIsLockOn = true;
		OwnerPC->SetIgnoreLookInput(true);
		SetComponentTickEnabled(true);
		UpdateTargetUI(CurrentTarget, true);

		OwnerChar->GetCharacterMovement()->bOrientRotationToMovement = false;
		OwnerChar->bUseControllerRotationYaw = true;
		GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Cyan, TEXT("LockOn"));
	}
}

void UT3CombatComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (bIsLockOn && CurrentTarget && OwnerPC)
	{
		FRotator LookAtRot = UKismetMathLibrary::FindLookAtRotation(OwnerChar->GetActorLocation(), CurrentTarget->GetActorLocation());
		FRotator NewRot = FMath::RInterpTo(OwnerPC->GetControlRotation(), LookAtRot, DeltaTime, InterpSpeed);
		OwnerPC->SetControlRotation(NewRot);
	}
}

AActor* UT3CombatComponent::FindBestTarget()
{
	if (!OwnerPC) return nullptr;

	TArray<FOverlapResult> Overlaps;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(OwnerChar);

	// 1. 물리적인 범위 내의 적들을 가져옴 (성능 최적화)
	bool bHit = GetWorld()->OverlapMultiByChannel(
		Overlaps,
		OwnerChar->GetActorLocation(),
		FQuat::Identity,
		ECC_Pawn,
		FCollisionShape::MakeSphere(SearchRadius),
		Params
	);

	AActor* BestTarget = nullptr;
	float MinScreenDistance = TNumericLimits<float>::Max(); // 화면 중앙과의 최소 거리 저장

	// 뷰포트 크기 가져오기
	int32 ViewportSizeX, ViewportSizeY;
	OwnerPC->GetViewportSize(ViewportSizeX, ViewportSizeY);
	FVector2D ScreenCenter(ViewportSizeX * 0.5f, ViewportSizeY * 0.5f);

	if (bHit)
	{
		for (auto& Result : Overlaps)
		{
			AActor* Potential = Result.GetActor();

			// 태그 확인 및 시야 확인
			if (Potential && Potential->ActorHasTag(TargetTag) && IsTargetVisible(Potential))
			{
				// 2. 몬스터의 월드 위치를 화면 2D 위치로 변환
				FVector2D ScreenPosition;
				bool bIsOnScreen = OwnerPC->ProjectWorldLocationToScreen(Potential->GetActorLocation(), ScreenPosition);

				if (bIsOnScreen)
				{
					// 3. 화면 중앙과 몬스터 화면 위치 사이의 거리 계산
					float DistanceFromCenter = FVector2D::Distance(ScreenCenter, ScreenPosition);

					// 화면 중앙에서 가장 가까운 대상을 선택
					if (DistanceFromCenter < MinScreenDistance)
					{
						MinScreenDistance = DistanceFromCenter;
						BestTarget = Potential;
					}
				}
			}
		}
	}
	return BestTarget;
}

bool UT3CombatComponent::IsTargetVisible(AActor* Target) const
{
	FHitResult Hit;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(OwnerChar);

	FVector Start = OwnerChar->GetActorLocation() + FVector(0, 0, 75.f);
	FVector End = Target->GetActorLocation();

	// 시야 가림 여부 확인
	bool bBlocked = GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params);
	return !bBlocked || (Hit.GetActor() == Target);
}

void UT3CombatComponent::ResetLockOn()
{
	UpdateTargetUI(CurrentTarget, false);
	bIsLockOn = false;
	OwnerChar->PlayerInputState.bIsLockOn = false;
	CurrentTarget = nullptr;

	if (OwnerPC)
	{
		OwnerPC->ResetIgnoreLookInput(); // 마우스 입력 다시 허용
		GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Red, TEXT("LockOff"));

		OwnerChar->GetCharacterMovement()->bOrientRotationToMovement = true;
		OwnerChar->bUseControllerRotationYaw = false;
	}

	SetComponentTickEnabled(false); // 틱 중지하여 자원 절약
}

// 록온 타겟 위에 록온 위젯 생성
void UT3CombatComponent::UpdateTargetUI(AActor* Target, bool bIsVisible)
{
	AT3DamageTestActor* Enemy = Cast<AT3DamageTestActor>(Target);
	if (Enemy)
	{
		Enemy->SetLockOnWidgetVisible(bIsVisible);
	}
}


// ========== 전투 로직 ===============

// 피격 로직
void UT3CombatComponent::HandleTakeAnyDamage(AActor* DamagedActor, float Damage, const class UDamageType* DamageType, class AController* InstigatedBy, AActor* DamageCauser)
{
	if (Damage <= 0.f || !OwnerChar || CurrentState == ECharacterCombatState::Dead) return;
	if (!DamageCauser || !DamageType) return;

	// 1. [디버그] 공격자 정보 및 데미지 타입 확인
	FString TypeName = DamageType ? DamageType->GetClass()->GetName() : TEXT("Normal");
	GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::White,
		FString::Printf(TEXT("Hit by: %s | Original Damage: %.1f | Type: %s"),
			*DamageCauser->GetName(), Damage, *TypeName));

	// 최종 데미지 계산
	float FinalDamage = CalculateFinalDamage(Damage, DamageType);

	// 3. [상태별 로그 출력]
	if (FinalDamage <= 0.f)
	{
		if (CurrentState == ECharacterCombatState::Dodge)
			GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Green, TEXT("Result: [EVADE] - Invincible Frame!"));
		
		else if (CurrentState == ECharacterCombatState::Parrying)
			GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Cyan, TEXT("Result: [PARRY] - Success!"));
		return;
	}
	else if (CurrentState == ECharacterCombatState::Blocking)
	{
		// 1. 스태미나 50 차감
		float NewStamina = FMath::Max(0.f, OwnerChar->GetCurrentStamina() - 50.f);
		OwnerChar->SetCurrentStamina(NewStamina);

		// 2. 결과 출력
		GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Cyan,
			FString::Printf(TEXT("Remaining Stamina: %.1f"), NewStamina));
		
		GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Yellow,
			FString::Printf(TEXT("Result: [BLOCK] - Reduced Damage: %.1f"), FinalDamage));
	}



	// 4. 실제 체력 차감 및 상태 보고
	float NewHP = FMath::Max(0.f, OwnerChar->GetCurrentHP() - FinalDamage);
	OwnerChar->SetCurrentHP(NewHP);

	GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Red,
		FString::Printf(TEXT("HP Status: %.1f / %.1f"), NewHP, OwnerChar->GetMaxHP()));


	
	// 사망 판정
	if (NewHP <= 0.f)
	{
		CurrentState = ECharacterCombatState::Dead;
		// 사망 로직 실행
		return;
	}

	 // 5. 피격 방향 계산 및 출력
	 EHitDirection HitDir = CalculateHitDirection(DamageCauser->GetActorLocation());
	 FString DirName = StaticEnum<EHitDirection>()->GetNameStringByValue((int64)HitDir);
	 GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Orange, FString::Printf(TEXT("Hit Direction: [%s]"), *DirName));
}

// 피격 데미지 계산
float UT3CombatComponent::CalculateFinalDamage(float IncomingDamage, const class UDamageType* DamageType)
{

	// 즉사 공격은 어떤 상황이든 예외 없이 최우선 사망
	if (DamageType->IsA(UT3DamageType_InstantDeath::StaticClass()))
	{
		return 9999.f;
	}

	// 1. 회피 상태 (무적)
	if (CurrentState == ECharacterCombatState::Dodge)
	{
		return 0.f;
	}

	// 2. 패링 상태
	if (CurrentState == ECharacterCombatState::Parrying)
	{
		// 패링 불가 공격인지 확인
		if (DamageType->IsA(UT3DamageType_Unparryable::StaticClass()))
		{
			return IncomingDamage; // 패링 실패, 생으로 맞음
		}
		return 0.f; // 패링 성공 (데미지 0)
	}

	// 3. 막기 상태인 경우
	if (CurrentState == ECharacterCombatState::Blocking)
	{
		// 막기 불가 공격인지 확인
		if (DamageType->IsA(UT3DamageType_Unblockable::StaticClass()))
		{
			return IncomingDamage; // 가드 뚫림
		}
		return IncomingDamage * 0.1f; // 데미지 90% 경감
	}




	return IncomingDamage; // 일반 상태 피격
}

// 피격 방향 로직
EHitDirection UT3CombatComponent::CalculateHitDirection(const FVector& HitLocation)
{
	if (!OwnerChar) return EHitDirection::Front;

	// 캐릭터 위치에서 공격 위치로의 방향 벡터 (평면상의 계산을 위해 Z값 무시)
	FVector OwnerLoc = OwnerChar->GetActorLocation();
	FVector TargetLoc = HitLocation;
	OwnerLoc.Z = TargetLoc.Z = 0.f;

	FVector ToHit = (TargetLoc - OwnerLoc).GetSafeNormal();
	FVector Forward = OwnerChar->GetActorForwardVector();
	FVector Right = OwnerChar->GetActorRightVector();

	// 1. 앞/뒤 판정 (내적)
	// 결과값: 1에 가까우면 앞, -1에 가까우면 뒤
	float ForwardDot = FVector::DotProduct(Forward, ToHit);

	// 2. 좌/우 판정 (내적)
	// 결과값: 1에 가까우면 우측, -1에 가까우면 좌측
	float RightDot = FVector::DotProduct(Right, ToHit);

	// 각도를 기준으로 4방향 분할 (45도 기준)
	if (ForwardDot >= 0.5f) return EHitDirection::Front;
	if (ForwardDot <= -0.5f) return EHitDirection::Back;

	// 앞뒤가 아닐 때 우측 혹은 좌측
	return (RightDot >= 0.f) ? EHitDirection::Right : EHitDirection::Left;
}


// 공격 로직

// 노티파이를 통해 공격 탐지
void UT3CombatComponent::SetAttackDetectionEnabled(bool bEnabled, float InDamageMultiflier, TSubclassOf<UDamageType> InType)
{
	if (bEnabled && OwnerChar)
	{
		HitActors.Empty();
		CurrentAttackDamage = InDamageMultiflier * OwnerChar->GetAttackPower();
		CurrentDamageType = InType;
		GetWorld()->GetTimerManager().SetTimer(AttackTraceTimerHandle, this, &UT3CombatComponent::ExecuteAttackTrace, 0.01f, true);
	}
	else
	{
		GetWorld()->GetTimerManager().ClearTimer(AttackTraceTimerHandle);
	}
}

// 캐릭터에 붙어있는 소켓 트레이스로 공격 실행
void UT3CombatComponent::ExecuteAttackTrace()
{
	if (!OwnerChar) return;

	UStaticMeshComponent* WeaponMesh = Cast<UStaticMeshComponent>(OwnerChar->GetDefaultSubobjectByName(TEXT("WeaponMesh")));

	if (!WeaponMesh)
	{
		TArray<UStaticMeshComponent*> MeshComps;
		OwnerChar->GetComponents<UStaticMeshComponent>(MeshComps);
		for (UStaticMeshComponent* Mesh : MeshComps)
		{
			if (Mesh->GetName() == TEXT("WeaponMesh"))
			{
				WeaponMesh = Mesh;
				break;
			}
		}
	}

	if (!WeaponMesh)
	{
		UE_LOG(LogTemp, Warning, TEXT("WeaponMesh Not Found!"));
		return;
	}

	FVector Start = WeaponMesh->GetSocketLocation(TEXT("Start_Socket"));
	FVector End = WeaponMesh->GetSocketLocation(TEXT("End_Socket"));

	TArray<FHitResult> HitResults;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(OwnerChar);

	// 구체 트레이스
	bool bHit = GetWorld()->SweepMultiByChannel(
		HitResults,
		Start,
		End,
		FQuat::Identity,
		ECC_Pawn,
		FCollisionShape::MakeSphere(15.f),
		Params
	);

	// 디버그 라인 그리기
	DrawDebugCapsule(GetWorld(), (Start + End) * 0.5f, FVector::Distance(Start, End) * 0.5f + 15.f, 15.f,
		FRotationMatrix::MakeFromZ(End - Start).ToQuat(), FColor::Red, false, 0.5f);

	if (bHit)
	{
		for (const FHitResult& Hit : HitResults)
		{
			AActor* Target = Hit.GetActor();
			if (Target && !HitActors.Contains(Target))
			{
				HitActors.Add(Target);

				// 데미지 전달
				RequestAttackDamage(Target, CurrentAttackDamage, CurrentDamageType);

				// 타격 성공 로그
				GEngine->AddOnScreenDebugMessage(-1, 0.5f, FColor::Yellow, TEXT("Attack Hit!"));
			}
		}
	}
}

void UT3CombatComponent::RequestAttackDamage(AActor* TargetActor, float DamageAmount, TSubclassOf<UDamageType> DamageTypeClass)
{
	if (!TargetActor || !OwnerChar || !OwnerPC) return;

	UGameplayStatics::ApplyDamage(
		TargetActor, 
		DamageAmount, 
		OwnerPC,            // 데미지 유발 컨트롤러
		OwnerChar,          // 데미지 유발자
		DamageTypeClass        // 데미지 타입 
	);
}