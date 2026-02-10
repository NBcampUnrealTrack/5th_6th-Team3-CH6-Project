// T3CombatComponent.cpp

#include "Player/T3CombatComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/KismetMathLibrary.h"
#include "Engine/OverlapResult.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Player/T3CharacterBase.h"
#include "Player/T3PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Player/T3DamageTestActor.h"
#include "Player/T3CharacterDataAsset.h"
#include "Player/T3WeaponBase.h"
#include "Monster/T3BossMonster.h"
#include "Components/CapsuleComponent.h"
#include "DrawDebugHelpers.h"
#include "GameFramework/SpringArmComponent.h"
#include "Player/T3SkillComponentBase.h"


UT3CombatComponent::UT3CombatComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UT3CombatComponent::BeginPlay()
{
	Super::BeginPlay();
	OwnerChar = Cast<AT3CharacterBase>(GetOwner());
	SpringArm = OwnerChar->FindComponentByClass<USpringArmComponent>();

	if (OwnerChar)
	{
		OwnerPC = OwnerChar->GetController<APlayerController>();
		CurrentState = ECharacterCombatState::Idle; // 생성시 캐릭터 상태 초기화
	}


	AIChar = Cast<ACharacter>(GetOwner());
	if (OwnerChar)
	{
		AIPC = OwnerChar->GetController<AController>();
	}

}

void UT3CombatComponent::InitializeWeapons(const TMap<EEquipSlot, FWeaponEquipInfo>& WeaponMap)
{
	ClearWeapons();

	for (const auto& Pair : WeaponMap)
	{
		const EEquipSlot Slot = Pair.Key;
		const FWeaponEquipInfo& Info = Pair.Value;

		if (!Info.WeaponClass) continue;

		FActorSpawnParameters Params;
		Params.Owner = GetOwner();
		Params.Instigator = Cast<APawn>(GetOwner());

		if (AT3WeaponBase* NewWeapon = GetWorld()->SpawnActor<AT3WeaponBase>(Info.WeaponClass, Params))
		{
			// 캐릭터의 메시에 부착
			NewWeapon->AttachToComponent(OwnerChar->GetMesh(),
				FAttachmentTransformRules::SnapToTargetIncludingScale, Info.SocketName);
			NewWeapon->SetInstigator(OwnerChar);
			NewWeapon->SetOwner(OwnerChar);
			// 데이터 에셋에 설정된 상대적 위치/회전값 적용
			NewWeapon->SetActorRelativeTransform(Info.RelativeTransform);
			EquippedWeapons.Add(Slot, NewWeapon);
		}
}
	}

AT3WeaponBase* UT3CombatComponent::GetWeaponBySlot(EEquipSlot Slot) const
{
	if (const TObjectPtr<AT3WeaponBase>* WeaponPtr = EquippedWeapons.Find(Slot))
	{
		return WeaponPtr->Get();
	}
	return nullptr;
}

void UT3CombatComponent::ClearWeapons()
{
	for (auto& Pair : EquippedWeapons)
	{
		if (Pair.Value) Pair.Value->Destroy();
	}
	EquippedWeapons.Empty();
}

// --- 막기 로직 ---
void UT3CombatComponent::StartBlock()
{
	// 스태미너 50이상만 막기 가능
	if (!OwnerChar || OwnerChar->GetCurrentStamina() < 50.f)
	{
		GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Red, TEXT("You Need Stamina."));
		return;
	}

	if (OwnerChar->PlayerInputState.bIsBlocking || CurrentState != ECharacterCombatState::Idle || !bCanBlock) return;

	// 2. 초기 상태 설정: 패링(Parrying) 모드 진입
	GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Blue, TEXT("BlockingModeOn"));
	CurrentState = ECharacterCombatState::Parrying;
	OwnerChar->PlayerInputState.bIsBlocking = true;
	bCanBlock = false;
	OwnerChar->GetCharacterMovement()->MaxWalkSpeed = 200.0f;


	// 0.2초 후 SwitchToBlockingState 호출
	GetWorld()->GetTimerManager().ClearTimer(ParryingToBlockingTimerHandle);
	GetWorld()->GetTimerManager().SetTimer(
		ParryingToBlockingTimerHandle,
		this,
		&UT3CombatComponent::SwitchToBlockingState,
		0.2f,
		false
	);
}

void UT3CombatComponent::EndBlock()
{
	if (!OwnerChar || !OwnerChar->PlayerInputState.bIsBlocking) return;

	CurrentState = ECharacterCombatState::Idle;
	OwnerChar->PlayerInputState.bIsBlocking = false;
	OwnerChar->GetCharacterMovement()->MaxWalkSpeed = 500.0f;
	
	GetWorld()->GetTimerManager().ClearTimer(ParryingToBlockingTimerHandle);

	GetWorld()->GetTimerManager().SetTimer(
		BlockingCooldownTimerHandle,
		this,
		&UT3CombatComponent::ResetBlockCooldown,
		BlockCooldownTime,
		false
	);
}

void UT3CombatComponent::ResetBlockCooldown()
{
	bCanBlock = true;
	GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Green, TEXT("Block Ready Again"));
}

void UT3CombatComponent::Attack()
{
	if (OwnerChar->GetCurrentStamina() > 10.f) // 스태미나 10 이하면 공격 불가
		// 공격 시 스태미너 10 소모
	{
		OwnerChar->OnAttack();
	}
}

void UT3CombatComponent::SetParryingEnabled(bool bEnabled)
{
	if (CurrentState == ECharacterCombatState::Idle) return;
	CurrentState = bEnabled ? ECharacterCombatState::Parrying : ECharacterCombatState::Blocking;
}

void UT3CombatComponent::SetDodgingEnabled(bool bEnabled)
{
	if (bEnabled)
	{
		CurrentState = ECharacterCombatState::Dodge;
		UE_LOG(LogTemp, Display, TEXT("DodgeOn"));
	}
	else
	{
		// 현재가 Dodge일 때만 해제
		if (CurrentState == ECharacterCombatState::Dodge)
		{
			CurrentState = ECharacterCombatState::Idle;
			UE_LOG(LogTemp, Display, TEXT("DodgeOff"));
		}
	}
}

void UT3CombatComponent::SwitchToBlockingState()
{
	if (CurrentState == ECharacterCombatState::Parrying)
	{
		CurrentState = ECharacterCombatState::Blocking;
	}
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

		// 카메라 랙 설정
		SpringArm->bEnableCameraRotationLag = true;
		SpringArm->bEnableCameraLag = true;

		OwnerChar->GetCharacterMovement()->bOrientRotationToMovement = false;
		OwnerChar->GetCharacterMovement()->bUseControllerDesiredRotation = true;
		GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Cyan, TEXT("LockOn"));
	}
}

void UT3CombatComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!bIsLockOn || !IsValid(CurrentTarget) || !OwnerChar || !OwnerPC || !SpringArm)
	{
		ResetLockOn();
		return;
	}

	// 타겟의 실시간 월드 위치
	FVector TargetLocation = CurrentTarget->GetActorLocation();

	// 타겟의 록온 높이 퍼센트 적용
	if (ACharacter* TargetChar = Cast<ACharacter>(CurrentTarget))
	{
		float HalfHeight = TargetChar->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
		float FootZ = TargetLocation.Z - HalfHeight;
		TargetLocation.Z = FootZ + (HalfHeight * 2.0f * TargetHeightPercent);
	}

	// 높이 차이 계산
	float HeightDifference = TargetLocation.Z - OwnerChar->GetActorLocation().Z;


	// 록온 대상의 높이가 높아질수록 광각으로 카메라가 멀어짐
	float RawAlpha = FMath::GetMappedRangeValueClamped(FVector2D(100.f, 1000.f), FVector2D(0.f, 1.f), HeightDifference);
	float ExponentialAlpha = FMath::Clamp(RawAlpha * 1.5f, 0.f, 1.f);

	// 스프링암 길이
	float DynamicMaxExtra = 2500.f;
	float TargetArmLength = DefaultArmLength + (ExponentialAlpha * DynamicMaxExtra);

	float TargetDistance = FMath::Lerp(DefaultArmLength, 2500.f, ExponentialAlpha);

	// 광각 범위
	float TargetFOV = FMath::Lerp(90.f, 120.f, ExponentialAlpha);

	// SocketOffset: 카메라를 더 위로 올려서 아래를 내려다보게 함 (High Angle)
	float TargetSocketZ = FMath::Lerp(50.f, 500.f, ExponentialAlpha);

	// 부드러운 카메라 전환
	SpringArm->TargetArmLength = FMath::FInterpTo(SpringArm->TargetArmLength, TargetDistance, DeltaTime, 5.0f);
	SpringArm->SocketOffset.Z = FMath::FInterpTo(SpringArm->SocketOffset.Z, TargetSocketZ, DeltaTime, 5.0f);

	if (OwnerPC->PlayerCameraManager)
	{
		float CurrentFOV = OwnerPC->PlayerCameraManager->GetFOVAngle();
		OwnerPC->PlayerCameraManager->SetFOV(FMath::FInterpTo(CurrentFOV, TargetFOV, DeltaTime, 4.0f));
	}

	// 바라보기 회전 
	FVector CameraLocation = SpringArm->GetComponentLocation(); // 캐릭터 위치가 아닌 카메라 기준
	FRotator LookAtRot = UKismetMathLibrary::FindLookAtRotation(CameraLocation, TargetLocation);
	
	LookAtRot.Pitch = FMath::Clamp(LookAtRot.Pitch, -75.f, 20.f);
	
	// 2. ControlRotation에 직접 Set하는 대신 RInterpTo를 사용
	// 갑작스러운 타겟 이동이나 수직 위치 변화 시 카메라가 튀는 것을 방지합니다.
	FRotator CurrentRot = OwnerPC->GetControlRotation();

	// Smoothness를 위해 InterpSpeed를 조절 (예: 7.0f)
	FRotator SmoothRot = FMath::RInterpTo(CurrentRot, LookAtRot, DeltaTime, 7.0f);

	OwnerPC->SetControlRotation(SmoothRot);

	// 4. 디버깅 
	DrawDebugSphere(GetWorld(), TargetLocation, 20.f, 12, FColor::Red, false, -1.f, 0, 2.f);
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

//void UT3CombatComponent::SetLockOnTarget(AActor* NewTarget)
//{
//	// 1. 기존 타겟의 마커 숨기기
//	if (CurrentTarget)
//	{
//		UWidgetComponent* OldMarker = CurrentTarget->FindComponentByClass<UWidgetComponent>();
//		if (OldMarker) OldMarker->SetHiddenInGame(true);
//	}
//
//	CurrentTarget = NewTarget;
//
//	// 2. 새 타겟의 마커 보여주기
//	if (CurrentTarget)
//	{
//		UWidgetComponent* NewMarker = CurrentTarget->FindComponentByClass<UWidgetComponent>();
//		if (NewMarker)
//		{
//			NewMarker->SetHiddenInGame(false);
//			// 필요하다면 여기서 마커의 애니메이션을 재생시킬 수도 있어
//		}
//	}
//}

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
		OwnerChar->GetCharacterMovement()->bUseControllerDesiredRotation = false;

		SpringArm->bEnableCameraRotationLag = false;
		SpringArm->bEnableCameraLag = false;
		SpringArm->TargetArmLength = DefaultArmLength;
		OwnerPC->PlayerCameraManager->SetFOV(90.f);
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

void UT3CombatComponent::ExecuteHitLogic(AActor* DamageCauser, float Damage, const UDamageType* DamageType, AController* InstigatedBy, EHitIntensity Intensity, float ReceievedDamageMultiplier)
{
	if (Damage <= 0.f || !OwnerChar || CurrentState == ECharacterCombatState::Dead) return;
	if (!DamageCauser || !DamageType) return;
	
	// 1. [디버그] 공격자 정보 및 데미지 타입 확인
	FString TypeName = DamageType ? DamageType->GetClass()->GetName() : TEXT("Normal");
	GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::White,
		FString::Printf(TEXT("Hit by: %s | Original Damage: %.1f | Type: %s | multi: %.1f"),
			*DamageCauser->GetName(), Damage, *TypeName, ReceievedDamageMultiplier));

	// 최종 데미지 계산
	float FinalDamage = CalculateFinalDamage(Damage, DamageType, ReceievedDamageMultiplier);

	// 3. [상태별 로그 출력]
	if (FinalDamage <= 0.f)
	{
		if (CurrentState == ECharacterCombatState::Dodge)
			GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Green, TEXT("Result: [EVADE] - Invincible Frame!"));

		else if (CurrentState == ECharacterCombatState::Parrying)
			GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Cyan, TEXT("Result: [PARRY] - Success!"));
		// 패링 성공 시 보스에게 스턴치 10 부여
		AT3BossMonster* HitBoss = Cast<AT3BossMonster>(DamageCauser);
		if (HitBoss) { HitBoss->Damage(0, 10.f); }
		return;
	}
	else if (CurrentState == ECharacterCombatState::Blocking)
	{
		// 1. 스태미나 50 차감
		ConsumeStamina(50.f);

		GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Yellow,
			FString::Printf(TEXT("Result: [BLOCK] - Reduced Damage: %.1f"), FinalDamage));
	}



	// 4. 실제 체력 차감 및 상태 보고
	float NewHP = FMath::Max(0.f, OwnerChar->GetCurrentHP() - FinalDamage);
	OwnerChar->SetCurrentHP(NewHP);

	GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Red,
		FString::Printf(TEXT("HP Status: %.1f / %.1f"), NewHP, OwnerChar->GetMaxHP()));
	   UE_LOG(LogTemp, Warning, TEXT("HP Status: %.1f / %.1f"), NewHP, OwnerChar->GetMaxHP());
	   UE_LOG(LogTemp, Display, TEXT("final : %.1f"), FinalDamage);



	// 사망 판정
	if (NewHP <= 0.f)
	{
		CurrentState = ECharacterCombatState::Dead;
		// 사망 로직 실행
		return;
	}


	// 공격 강도
	EHitIntensity ReceivedIntensity = Intensity;
	HitIntensity = ReceivedIntensity;

	// 공격 강도 로그 출력
	FString IntensityStr = StaticEnum<EHitIntensity>()->GetNameStringByValue((int64)ReceivedIntensity);
	UE_LOG(LogTemp, Warning, TEXT("피격 강도: %s"), *IntensityStr);

	// 5. 피격 방향 계산 및 출력
	EHitDirection HitDir = CalculateHitDirection(DamageCauser->GetActorLocation());
	HitDirection = HitDir;
	FString DirName = StaticEnum<EHitDirection>()->GetNameStringByValue((int64)HitDir);
	GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Orange, FString::Printf(TEXT("Hit Direction: [%s]"), *DirName));
	UE_LOG(LogTemp, Warning, TEXT("Hit Direction: [%s]"), *DirName);
}

// 피격 데미지 계산
float UT3CombatComponent::CalculateFinalDamage(float IncomingDamage, const class UDamageType* DamageType, float ReceievedDamageMultiplier)
{
	float Defence = OwnerChar->GetDefense();
	float DamageReductionScale = FMath::Max(0.5f, ReceievedDamageMultiplier - Defence);
	IncomingDamage *= DamageReductionScale;  // 데미지 * (데미지 배율 - 방어력 배율)


	// 어떤 상황이든 예외 없이 데미지
	if (DamageType->IsA(UT3DamageType_Undodgable::StaticClass()))
	{
		return IncomingDamage;
	}

	// 1. 회피 상태
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
		// 막기 불가 공격 또는 패링 불가 공격인 경우
		if (DamageType->IsA(UT3DamageType_Unblockable::StaticClass()) || DamageType->IsA(UT3DamageType_Unparryable::StaticClass()))
		{
			return IncomingDamage; // 가드 뚫림
		}
		return IncomingDamage * 0.1f; // 데미지 90% 경감
	}



	UE_LOG(LogTemp, Display, TEXT("defence : %.1f"), Defence);
	UE_LOG(LogTemp, Display, TEXT("multi : %.1f"), DamageReductionScale);
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

void UT3CombatComponent::RequestAttackDamage(AActor* TargetActor, float DamageAmount, EHitIntensity Intensity, float DamageMultiflier, TSubclassOf<UT3DamageType_Base> DamageTypeClass)
{
	if (!TargetActor) { UE_LOG(LogTemp, Warning, TEXT("Target Missing!")); return; }
	if (!OwnerChar && !AIChar) { UE_LOG(LogTemp, Warning, TEXT("Owner Missing!")); return; }
	if (!DamageTypeClass) { DamageTypeClass = UT3DamageType_Base::StaticClass(); }
	

	// 커스텀 데미지 이벤트 생성
	FT3DamageEvent T3DamageEvent(DamageTypeClass);
	T3DamageEvent.HitIntensity = Intensity; // 공격 강도를 구조체에 직접 삽입
	T3DamageEvent.HitDamageMultiplier = DamageMultiflier;

	AT3BossMonster* HitBoss = Cast<AT3BossMonster>(TargetActor);

	if (HitBoss)
	{
		GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Red, TEXT("Hit Boss!"));
		HitBoss->Damage(DamageAmount, 20.f);  // 테스트용 스턴 20
		// HitBoss->Damage(CurrentAttackDamage, StunAmount);
	}

	// TakeDamage 호출 시 커스텀 이벤트 구조체를 전달
	else if (OwnerChar)
	{
		TargetActor->TakeDamage(DamageAmount, T3DamageEvent, OwnerPC, OwnerChar);
	}
	else if (AIChar) // OwnerChar가 아닐 때만 AIChar로 실행
	{
		TargetActor->TakeDamage(DamageAmount, T3DamageEvent, AIPC, AIChar);
	}
	
	// 디버그 출력
	// const UEnum* EnumPtr = StaticEnum<EHitIntensity>();
	// FString IntensityString = EnumPtr ? EnumPtr->GetNameStringByValue((int64)Intensity) : TEXT("Unknown");

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Cyan,
		FString::Printf(TEXT("Attack Sent -> Target: %s, Damage: %.1f"), *TargetActor->GetName(), DamageAmount));
	}
}

// 스태미나 소모 함수
void UT3CombatComponent::ConsumeStamina(float Amount)
{
	if (OwnerChar && OwnerChar->GetCurrentStamina() >= Amount)
	{
		float NewStamina = OwnerChar->GetCurrentStamina() - Amount;
		OwnerChar->SetCurrentStamina(NewStamina);

		GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Cyan,
			FString::Printf(TEXT("Remaining Stamina: %.1f"), OwnerChar->GetCurrentStamina()));
	}
}


// 스킬&아이템 슬롯 함수
void UT3CombatComponent::ChangeActiveSlot(ESlotType Type)
{
	switch (Type)
	{
	case ESlotType::Skill:
		CurrentSkillSlot = (CurrentSkillSlot % MaxSkillSlots) + 1; // 슬롯 전환 시 순환으로 전환
		UE_LOG(LogTemp, Log, TEXT("Skill Slot Switched: %d"), CurrentSkillSlot);
		break;
	case ESlotType::Consumable:
		CurrentConsumableSlot = (CurrentConsumableSlot % MaxConsumableSlots) + 1;
		UE_LOG(LogTemp, Log, TEXT("Consumable Slot Switched: %d"), CurrentConsumableSlot);
		break;
	case ESlotType::Potion:
		CurrentPotionSlot = (CurrentPotionSlot % MaxPotionSlots) + 1;
		UE_LOG(LogTemp, Log, TEXT("Potion Slot Switched: %d"), CurrentPotionSlot);
		break;
	}

	if (OnSlotContentChanged.IsBound())
	{
		OnSlotSelectionChanged.Broadcast(Type, CurrentSkillSlot);
	}
}

void UT3CombatComponent::ExecuteCurrentSlotAction(ESlotType Type)
{
	switch (Type)
	{
	case ESlotType::Skill:
		if (SkillComp) SkillComp->ExecuteSkill(CurrentSkillSlot);
		break;
	case ESlotType::Consumable:
		// ItemComp->UseConsumable(CurrentConsumableSlot);
		UE_LOG(LogTemp, Log, TEXT("Using Consumable Slot: %d"), CurrentConsumableSlot);
		break;
	case ESlotType::Potion:
		// ItemComp->UsePotion(CurrentPotionSlot);
		UE_LOG(LogTemp, Log, TEXT("Using Potion Slot: %d"), CurrentPotionSlot);
		break;
	}
}

void UT3CombatComponent::UpdateSlotContent(ESlotType Type, int32 SlotIndex, int32 NewID)
{
	if (SlotIndex <= 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("Combat: Invalid SlotIndex %d"), SlotIndex);
		return;
	}

	switch (Type)
	{
	case ESlotType::Skill:
		if (SkillComp)
		{
			// 실제 데이터 변경은 각 컴포넌트에 위임
			SkillComp->SetSkillSlot(SlotIndex, NewID);
		}
		break;

	case ESlotType::Consumable:
		UE_LOG(LogTemp, Log, TEXT("Combat: Consumable Slot %d updated with ID %d"), SlotIndex, NewID);
		break;

	case ESlotType::Potion:
		UE_LOG(LogTemp, Log, TEXT("Combat: Potion Slot %d updated with ID %d"), SlotIndex, NewID);
		break;
	}

	// UI팀에게 알림
	if (OnSlotContentChanged.IsBound())
	{
		OnSlotContentChanged.Broadcast(Type, SlotIndex, NewID);
	}
}