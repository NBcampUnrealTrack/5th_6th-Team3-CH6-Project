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
#include "Kismet/KismetMathLibrary.h"


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
	if (CurrentState != ECharacterCombatState::Idle) return;
	CurrentState = ECharacterCombatState::Blocking;
	OwnerChar->PlayerInputState.bIsBlocking = true;
	OwnerChar->GetCharacterMovement()->MaxWalkSpeed = 200.0f;
	// 여기서 몽타주 재생 로직 추가 (CharacterDataAsset 활용)
}

void UT3CombatComponent::EndBlock()
{
	CurrentState = ECharacterCombatState::Idle;
	OwnerChar->PlayerInputState.bIsBlocking = false;
	OwnerChar->GetCharacterMovement()->MaxWalkSpeed = 500.0f;
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
	}

	SetComponentTickEnabled(false); // 틱 중지하여 자원 절약
}

// 록온 타겟 위에 록온 위젯 생성
void UT3CombatComponent::UpdateTargetUI(AActor* Target, bool bIsVisible)
{
	//AAICharacter* Enemy = Cast<AAICharacter>(Target);
	//if (Enemy)
	//{
	//	Enemy->SetLockOnWidgetVisible(bIsVisible);
	//}
}


// ========== 전투 로직 ===============

// 피격 로직
void UT3CombatComponent::HandleTakeAnyDamage(AActor* DamagedActor, float Damage, const class UDamageType* DamageType, class AController* InstigatedBy, AActor* DamageCauser)
{
	if (Damage <= 0.f || !OwnerChar || CurrentState == ECharacterCombatState::Dead) return;
	if (!DamageCauser || !DamageType) return;

	// 최종 데미지 계산
	float FinalDamage = CalculateFinalDamage(Damage, DamageType);

	// 체력 깎이는 로직
	float NewHP = OwnerChar->GetCurrentHP() - FinalDamage;
	OwnerChar->SetCurrentHP(NewHP);
	
	// 사망 판정
	if (NewHP <= 0.f)
	{
		CurrentState = ECharacterCombatState::Dead;
		// 사망 로직 실행
		return;
	}

	// 리액션 분기

	// 회피 성공 시 아무것도 안 함
	if (CurrentState == ECharacterCombatState::Dodge && FinalDamage <= 0.f) return;

	// 패링 시
	 if (FinalDamage <= 0.f && CurrentState == ECharacterCombatState::Parrying)
	{
		// 패링 효과 연출
		 GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Cyan, TEXT("Parry Success!"));
		 return;
	}

	 // 막기 시
	else if (FinalDamage < Damage && CurrentState == ECharacterCombatState::Blocking)
	{
		// 막기 효과 연출
		 GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Yellow, TEXT("Block Success!"));

		 // 막기 성공 시 스태미너 50 차감
		 float NewStamina = FMath::Max(0.f, OwnerChar->GetCurrentStamina() - 50.f);
		 OwnerChar->SetCurrentStamina(NewStamina);
		 return;
	 }

	 // 위 조건들에 해당 안 되면 피격방향 계산 후 ENUM 도출
	 EHitDirection HitDir = CalculateHitDirection(DamageCauser->GetActorLocation());
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
void UT3CombatComponent::RequestAttackDamage(AActor* TargetActor, float DamageAmount, TSubclassOf<UDamageType> DamageTypeClass)
{
	if (!TargetActor || !OwnerChar || !OwnerPC) return;

	// 이 함수가 실행되면 TargetActor 내부의 HandleTakeAnyDamage가 자동으로 호출됩니다.
	UGameplayStatics::ApplyDamage(
		TargetActor, 
		DamageAmount, 
		OwnerPC,            // 데미지 유발 컨트롤러
		OwnerChar,          // 데미지 유발자
		DamageTypeClass        // 데미지 타입 
	);
}