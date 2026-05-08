// T3CombatComponent.cpp

#include "Player/T3CombatComponent.h"

#include "Desecration.h"
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
#include "Player/T3CommonSkillComponent.h"
#include "Player/T3LockOnTarget.h"
#include "Components/WidgetComponent.h"
#include "Monster/T3MonsterBase.h"
#include "Player/Taoist/T3Taoist_SkillComponent.h"
#include "Player/Taoist/T3FanWeapon.h"
#include "GameSystem/Interface/T3Poisonable.h"


UT3CombatComponent::UT3CombatComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	bWantsInitializeComponent = true;
}

void UT3CombatComponent::BeginPlay()
{
	Super::BeginPlay();

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

void UT3CombatComponent::InitializeComponent()
{
	Super::InitializeComponent();

	// BeginPlay보다 먼저 Owner를 캐싱하여 안전성 확보
	OwnerChar = Cast<AT3CharacterBase>(GetOwner());
	if (OwnerChar)
	{
		SpringArm = OwnerChar->FindComponentByClass<USpringArmComponent>();
	}
}

void UT3CombatComponent::SetCombatState(ECharacterCombatState NewState)
{
	if (CurrentState == NewState) return;

	// 이전 상태에서 빠져나올 때 처리
	if (CurrentState == ECharacterCombatState::Dead) return; 
	CurrentState = NewState;
}

void UT3CombatComponent::SetInvincible(bool bIsInvincible)
{
	if (bIsInvincible)
	{
		SetCombatState(ECharacterCombatState::Invincible);
	}
	else
	{
		// 무적 해제 시 Idle로 돌아가기
		SetCombatState(ECharacterCombatState::Idle);
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
	// 1. 조건 체크 (스태미너 등)
	if (!OwnerChar || OwnerChar->GetCurrentStamina() < 50.f) return;
	if (CurrentState != ECharacterCombatState::Idle || !bCanBlock || OwnerChar->PlayerInputState.bIsAttacking) return;
	if (OwnerChar->PlayerInputState.bIsAttacking) return;
	
	// 2. 즉시 막기 상태로 전환
	CurrentState = ECharacterCombatState::Blocking;
	OwnerChar->PlayerInputState.bIsBlocking = true;
	bCanBlock = false;
	OwnerChar->GetCharacterMovement()->MaxWalkSpeed = 200.0f;


	// 도사인 경우 방어 시작 시 부채 펴기
	if (OwnerChar->GetCurrentClass() == ECharacterClass::Taoist)
	{
		AT3FanWeapon* FanWeapon = Cast<AT3FanWeapon>(GetWeaponBySlot(EEquipSlot::RightHand));
		if (IsValid(FanWeapon))
		{
			FanWeapon->OpenFan();
		}
	}

}

void UT3CombatComponent::EndBlock()
{
	if (!OwnerChar || !OwnerChar->PlayerInputState.bIsBlocking) return;

	CurrentState = ECharacterCombatState::Idle;
	OwnerChar->PlayerInputState.bIsBlocking = false;
	OwnerChar->GetCharacterMovement()->MaxWalkSpeed = 500.0f;
	
	GetWorld()->GetTimerManager().SetTimer(
		BlockingCooldownTimerHandle,
		this,
		&UT3CombatComponent::ResetBlockCooldown,
		BlockCooldownTime,
		false
	);


	// 도사인 경우 방어 종료 시 부채 접기
	if (OwnerChar->GetCurrentClass() == ECharacterClass::Taoist)
	{
		AT3FanWeapon* FanWeapon = Cast<AT3FanWeapon>(GetWeaponBySlot(EEquipSlot::RightHand));
		if (IsValid(FanWeapon))
		{
			FanWeapon->CloseFan();
		}
	}
}

void UT3CombatComponent::ResetBlockCooldown()
{
	bCanBlock = true;
	// GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Green, TEXT("Block Ready Again"));
}

void UT3CombatComponent::Attack()
{
	if (OwnerChar->GetCurrentStamina() > 10.f) // 스태미나 10 이하면 공격 불가
		// 공격 시 스태미너 10 소모
	{
		bIsBasicAttacking = true;
		OwnerChar->OnAttack();
		UE_LOG(LogTemp, Warning, TEXT("attack"));
	}
}

void UT3CombatComponent::SetParryingEnabled(bool bEnabled)
{
	if (bEnabled)
	{
		// 이전 상태 저장 후 패링으로 전환
		PreState = CurrentState;
		CurrentState = ECharacterCombatState::Parrying;
	}
	else
	{
		// 패링 시간이 끝나면 Idle이나 원래 하던 Blocking으로 복구
		CurrentState = PreState;
	}
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

// --- 록온 로직 ---
void UT3CombatComponent::ToggleLockOn()
{
	if (!OwnerChar || !OwnerPC) return;

	if (bIsLockOn)
	{
		// 록온 해제 시 기존 타겟 위젯 끄기
		if (IT3LockOnTarget* TargetInterface = Cast<IT3LockOnTarget>(CurrentTarget))
		{
			TargetInterface->SetLockOnWidgetVisible(false);
		}
		ResetLockOn();
		return;
	}

	CurrentTarget = FindBestTarget();
	// 인터페이스를 상속받았는지 확인 (안전한 캐스팅)
	IT3LockOnTarget* LockOnInterface = Cast<IT3LockOnTarget>(CurrentTarget);
	if (CurrentTarget && LockOnInterface)
	{
		bIsLockOn = true;
		OwnerChar->PlayerInputState.bIsLockOn = true;
		OwnerPC->SetIgnoreLookInput(true);
		SetComponentTickEnabled(true);

		// 카메라 랙 설정
		DefaultSocketOffsetZ = SpringArm->SocketOffset.Z;
		SpringArm->bEnableCameraRotationLag = true;
		SpringArm->bEnableCameraLag = true;
		OwnerChar->GetCharacterMovement()->bOrientRotationToMovement = false;
		OwnerChar->GetCharacterMovement()->bUseControllerDesiredRotation = true;

		// 위젯 켜기 (인터페이스 함수 호출)
		LockOnInterface->SetLockOnWidgetVisible(true);

		// GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Cyan, TEXT("LockOn"));
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

	// 몬스터 사망 시 록온 해제 
	if (AT3MonsterBase* TargetMonster = Cast<AT3MonsterBase>(CurrentTarget))
	{
		if (TargetMonster->bIsDead) // 또는 IsDead() 함수 호출
		{
			ResetLockOn();
			return;
		}
	}

	// 거리 초과 시 록온 해제 
	float DistanceToTarget = FVector::Dist(OwnerChar->GetActorLocation(), CurrentTarget->GetActorLocation());
	if (DistanceToTarget > SearchRadius)
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


	//// 록온 대상의 높이가 높아질수록 광각으로 카메라가 멀어짐
	//float RawAlpha = FMath::GetMappedRangeValueClamped(FVector2D(100.f, 1000.f), FVector2D(0.f, 1.f), HeightDifference);
	//float ExponentialAlpha = FMath::Clamp(RawAlpha * 1.5f, 0.f, 1.f);

	//// 스프링암 길이
	//float DynamicMaxExtra = 2500.f;
	//float TargetArmLength = DefaultArmLength + (ExponentialAlpha * DynamicMaxExtra);

	//float TargetDistance = FMath::Lerp(DefaultArmLength, 2500.f, ExponentialAlpha);

	//// 광각 범위
	//float TargetFOV = FMath::Lerp(90.f, 120.f, ExponentialAlpha);

	//// SocketOffset: 카메라를 더 위로 올려서 아래를 내려다보게 함 (High Angle)
	//float TargetSocketZ = FMath::Lerp(50.f, 500.f, ExponentialAlpha);

	float TargetDistance = DefaultArmLength; // 기본 길이에 고정
	float TargetFOV = 90.f;                // 기본 시야각에 고정 (원하는 기본값으로 설정하세요)
	float TargetSocketZ = 50.f;

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
	LookAtRot.Roll = 0.f;
	LookAtRot.Pitch = FMath::Clamp(LookAtRot.Pitch, -75.f, 20.f);
	
	// 2. ControlRotation에 직접 Set하는 대신 RInterpTo를 사용
	// 갑작스러운 타겟 이동이나 수직 위치 변화 시 카메라가 튀는 것을 방지합니다.
	FRotator CurrentRot = OwnerPC->GetControlRotation();

	// Smoothness를 위해 InterpSpeed를 조절 (예: 7.0f)
	FRotator SmoothRot = FMath::RInterpTo(CurrentRot, LookAtRot, DeltaTime, 7.0f);

	OwnerPC->SetControlRotation(SmoothRot);

	// 록온 표시 상대적 스케일 조정
	if (bIsLockOn && CurrentTarget)
	{
		UpdateLockOnWidgetScale();
	}

	// 4. 디버깅 
	// DrawDebugSphere(GetWorld(), TargetLocation, 20.f, 12, FColor::Red, false, -1.f, 0, 2.f);
}

void UT3CombatComponent::UpdateLockOnWidgetScale()
{
	if (!CurrentTarget || !OwnerChar) return;

	// 1. 거리 계산
	float Distance = FVector::Dist(OwnerChar->GetActorLocation(), CurrentTarget->GetActorLocation());

	// 2. 스케일 값 계산 (거리가 멀어질수록 NewScale은 작아짐)
	// 1000.f는 기준 거리입니다. 본인 프로젝트의 스케일에 맞춰 조절하세요.
	float NewScale = FMath::Clamp(1000.f / Distance, 0.3f, 1.5f);

	// 3. 인터페이스를 통해 위젯 컴포넌트 접근 (또는 직접 접근)
	// 여기서는 간단하게 CurrentTarget에서 컴포넌트를 찾아 스케일을 조절합니다.
	if (UWidgetComponent* TargetWidget = CurrentTarget->FindComponentByClass<UWidgetComponent>())
	{
		TargetWidget->SetWorldScale3D(FVector(NewScale));

		// [꿀팁] 위젯이 항상 카메라를 정면으로 바라보게 함 (Billboard 효과)
		FRotator TargetRotation = OwnerPC->GetControlRotation();
		TargetWidget->SetWorldRotation(TargetRotation);
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
	// 1. 인터페이스를 통한 UI 끄기
	if (CurrentTarget)
	{
		if (IT3LockOnTarget* TargetInterface = Cast<IT3LockOnTarget>(CurrentTarget))
		{
			TargetInterface->SetLockOnWidgetVisible(false);
		}
	}

	// 상태 변수 초기화
	bIsLockOn = false;
	OwnerChar->PlayerInputState.bIsLockOn = false;
	CurrentTarget = nullptr;

	if (OwnerPC)
	{
		OwnerPC->ResetIgnoreLookInput(); // 마우스 입력 다시 허용
		// GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Red, TEXT("LockOff"));

		OwnerChar->GetCharacterMovement()->bOrientRotationToMovement = true;
		OwnerChar->GetCharacterMovement()->bUseControllerDesiredRotation = false;

		SpringArm->bEnableCameraRotationLag = false;
		SpringArm->bEnableCameraLag = false;
		SpringArm->TargetArmLength = DefaultArmLength;
		SpringArm->SocketOffset.Z = DefaultSocketOffsetZ;
		OwnerPC->PlayerCameraManager->SetFOV(90.f);
	}

	SetComponentTickEnabled(false); // 틱 중지하여 자원 절약
}

// ========== 전투 로직 ===============

// 피격 로직

void UT3CombatComponent::ExecuteHitLogic(AActor* DamageCauser, float Damage, const UDamageType* DamageType, AController* InstigatedBy, EHitIntensity Intensity, float ReceievedDamageMultiplier)
{
	if (Damage <= 0.f || !OwnerChar || CurrentState == ECharacterCombatState::Dead) return;
	if (!DamageCauser || !DamageType) return;

	// 1. [디버그] 공격자 정보 및 데미지 타입 확인
	FString TypeName = DamageType ? DamageType->GetClass()->GetName() : TEXT("Normal");
	// GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::White,
		// FString::Printf(TEXT("Hit by: %s | Original Damage: %.1f | Type: %s | multi: %.1f"),
			// *DamageCauser->GetName(), Damage, *TypeName, ReceievedDamageMultiplier));

	// 최종 데미지 계산
	float FinalDamage = CalculateFinalDamage(Damage, DamageType, ReceievedDamageMultiplier);

	// 3. [상태별 로그 출력]
	if (FinalDamage <= 0.f)
	{
		// 회피 성공 시
		if (CurrentState == ECharacterCombatState::Dodge)
		{
			
			// 도사인 경우 회피 성공 시 패시브 스킬 효과 발동
			if (OwnerChar->GetCurrentClass() == ECharacterClass::Taoist)
			{
				UT3Taoist_SkillComponent* TaoistSkill = Cast< UT3Taoist_SkillComponent>(SkillComp);
				if (IsValid(TaoistSkill))
				{
					TaoistSkill->SetEmpowermentState(true);
				}
			}
			return;
		}
		// GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Green, TEXT("Result: [EVADE] - Invincible Frame!"));

		else if (CurrentState == ECharacterCombatState::Parrying)
		{
			// 패링 성공 시 
			OwnerChar->OnParryReaction();

			// 보스에게 스턴치 10 부여
			AT3BossMonster* HitBoss = Cast<AT3BossMonster>(DamageCauser);
			if (HitBoss) { HitBoss->Damage(0, 10.f); }

			// 팔라딘의 경우 신성게이지 40 증가
			if (OwnerChar->GetCurrentClass() == ECharacterClass::Paladin)
			{
				SkillComp->AddResource(HolyGaugeChargeAmount);
			}
			UE_LOG(LogTemp, Display, TEXT("Parrying!"));
			// GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Cyan, TEXT("Result: [PARRY] - Success!"));
			return;
		}
	}

	else if (CurrentState == ECharacterCombatState::Blocking)
	{
		// 막기 성공 시
		OwnerChar->OnBlockReaction();
		// 스태미나 50 차감 후 스태미너 0 이하로 떨어지면 막기 해제
		ConsumeStamina(50.f);

		
		//if (OwnerChar->GetCurrentStamina() <= 0.f)
		//{
		//	EndBlock();
		//}

		// 팔라딘이라면 신성 게이지 10 상승
		if (OwnerChar->GetCurrentClass() == ECharacterClass::Paladin)
		{
			SkillComp->AddResource(HolyGaugeChargeAmount * 3.0f / 8.0f);
			UE_LOG(LogItem, Display, TEXT("신성 게이지 %.1f 상승"), HolyGaugeChargeAmount * 3.0f / 8.0f);
		}

		/*GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Yellow,
			FString::Printf(TEXT("Result: [BLOCK] - Reduced Damage: %.1f"), FinalDamage));*/
	}



	// 4. 실제 체력 차감 및 상태 보고
	float NewHP = FMath::Max(0.f, OwnerChar->GetCurrentHP() - FinalDamage);
	OwnerChar->SetCurrentHP(NewHP);

	//GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Red,
	//	FString::Printf(TEXT("HP Status: %.1f / %.1f"), NewHP, OwnerChar->GetMaxHP()));
	UE_LOG(LogTemp, Warning, TEXT("HP Status: %.1f / %.1f"), NewHP, OwnerChar->GetMaxHP());
	UE_LOG(LogTemp, Display, TEXT("final : %.1f"), FinalDamage);

	// 팔라딘의 경우 신의 심판 시전 중 피격 당하면 스킬 캔슬
	if (OwnerChar->GetCurrentClass() == ECharacterClass::Paladin)
	{
		if (IsValid(SkillComp) && OwnerChar->bUsingSkill)
		{
			GetSkillComponent()->CancelCurrentSkill();
			OwnerChar->StopAnimMontage();
		}
	}

	// 팔라딘, 도사 피격 사운드
	if (OwnerChar->GetCurrentClass() == ECharacterClass::Paladin || OwnerChar->GetCurrentClass() == ECharacterClass::Taoist)
	{
		switch (Intensity)
		{
		case EHitIntensity::Light:
			PlaySkillEffectSound(LightHitVoice,2.0f);
			break;
		case EHitIntensity::Medium:
			PlaySkillEffectSound(MediumHitVoice, 2.0f);
			break;
		case EHitIntensity::Heavy:
			PlaySkillEffectSound(HeavyHitVoice, 2.0f);
			break;
		default:
			break;
		}
	}

	// 사망 판정
	if (NewHP <= 0.f)
	{
		if (OwnerChar->GetIsUndyingState())
		{
			OwnerChar->OnUndyingTriggered.Broadcast();
		}
		else
		{
			CurrentState = ECharacterCombatState::Dead;
			// 사망 시 록온 해제
			ResetLockOn();
			// 사망 로직 실행
			OwnerChar->OnDeath();
			return;
		}
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
	//GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Orange, FString::Printf(TEXT("Hit Direction: [%s]"), *DirName));
	//UE_LOG(LogTemp, Warning, TEXT("Hit Direction: [%s]"), *DirName);
	
	OnTakeDamage.Broadcast();
}


// 피격 데미지 계산
float UT3CombatComponent::CalculateFinalDamage(float IncomingDamage, const class UDamageType* DamageType, float ReceievedDamageMultiplier)
{
	// 0. 무적 상태
	if (CurrentState == ECharacterCombatState::Invincible)
	{
		return 0.f;
	}

	// 데미지 계산 로직
	float Defence = OwnerChar->GetDefense();
	float DamageReductionScale = FMath::Max(0.5f, ReceievedDamageMultiplier - Defence);
	IncomingDamage *= DamageReductionScale;  // 데미지 * (데미지 배율 - 방어력 배율)


	// 무적 상태가 아니면 무조건 데미지
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

void UT3CombatComponent::RequestAttackDamage(AActor* TargetActor, float DamageAmount, EHitIntensity Intensity, float DamageMultiflier, TSubclassOf<UT3DamageType_Base> DamageTypeClass, float InStunAmount, bool bIsBasicAttack)
{
	if (!TargetActor) { UE_LOG(LogTemp, Warning, TEXT("Target Missing!")); return; }
	if (!OwnerChar && !AIChar) { UE_LOG(LogTemp, Warning, TEXT("Owner Missing!")); return; }
	if (!DamageTypeClass) { DamageTypeClass = UT3DamageType_Base::StaticClass(); }
	

	// 커스텀 데미지 이벤트 생성
	FT3DamageEvent T3DamageEvent(DamageTypeClass);
	T3DamageEvent.HitIntensity = Intensity; // 공격 강도를 구조체에 직접 삽입
	T3DamageEvent.HitDamageMultiplier = DamageMultiflier;
	T3DamageEvent.StunAmount = InStunAmount;

	AT3BossMonster* HitBoss = Cast<AT3BossMonster>(TargetActor);

	if (HitBoss)
	{
		// GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Red, TEXT("Hit Boss!"));
		//HitBoss->Damage(DamageAmount, InStunAmount);  // 테스트용 스턴 20
		// HitBoss->Damage(CurrentAttackDamage, StunAmount);
		
		float ActualDamage = DamageAmount;

		if (bIsBasicAttack)
		{
			if (OwnerChar->GetSmiteThreshold() > 0 && OwnerChar->GetSmiteCounter() >= OwnerChar->GetSmiteThreshold())
			{
				ActualDamage = DamageAmount * OwnerChar->GetSmiteMultiplier();

				OwnerChar->SetSmiteCounter(0);
			}
			else
			{
				OwnerChar->IncrementSmiteCounter();
			}
		}

		HitBoss->Damage(ActualDamage, InStunAmount);
		
		OwnerChar->OnDamageDealt.Broadcast(TargetActor, DamageAmount);
		
		UE_LOG(LogItem, Warning, TEXT("현재 공격 횟수 : %d, 입힌 데미지 : %.1f"), OwnerChar->GetSmiteCounter(), ActualDamage);
	}
	
	// TakeDamage 호출 시 커스텀 이벤트 구조체를 전달
	else if (OwnerChar)
	{
		float ActualDamage = DamageAmount;

		if (bIsBasicAttack)
		{
			if (OwnerChar->GetSmiteThreshold() > 0 && OwnerChar->GetSmiteCounter() >= OwnerChar->GetSmiteThreshold())
			{
				ActualDamage = DamageAmount * OwnerChar->GetSmiteMultiplier();

				OwnerChar->SetSmiteCounter(0);
			}
			else
			{
				OwnerChar->IncrementSmiteCounter();
			}
		}

		TargetActor->TakeDamage(ActualDamage, T3DamageEvent, OwnerPC, OwnerChar);
		
		OwnerChar->OnDamageDealt.Broadcast(TargetActor, ActualDamage);
		
		UE_LOG(LogItem, Warning, TEXT("현재 공격 횟수 : %d, 입힌 데미지 : %.1f"), OwnerChar->GetSmiteCounter(), ActualDamage);
	}
	else if (AIChar) // OwnerChar가 아닐 때만 AIChar로 실행
	{
		TargetActor->TakeDamage(DamageAmount, T3DamageEvent, AIPC, AIChar);
	}

	// 기본 공격 + 독 룬 활성화 상태일 때 대상에게 독 스택 적용
	if (bIsBasicAttack && OwnerChar && OwnerChar->IsPoisonAttackEnabled())
	{
		if (TargetActor->Implements<UT3Poisonable>())
		{
			IT3Poisonable::Execute_ApplyPoisonStack(TargetActor, OwnerChar->GetPoisonStacksPerHit());
		}
	}

	if (SkillComp && DamageTypeClass)
	{
		if (DamageTypeClass->GetName().Contains(TEXT("BP_T3DamageType_ValkyriePassive")))
		{
			SkillComp->BasicAttackCount(); // 발키리면 스택 1 증가!
		}
	}
	// 디버그 출력
	// const UEnum* EnumPtr = StaticEnum<EHitIntensity>();
	// FString IntensityString = EnumPtr ? EnumPtr->GetNameStringByValue((int64)Intensity) : TEXT("Unknown");

	if (GEngine)
	{
		//GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Cyan,
		//FString::Printf(TEXT("Attack Sent -> Target: %s, Damage: %.1f"), *TargetActor->GetName(), DamageAmount));
	}
}

// 스태미나 소모 함수
void UT3CombatComponent::ConsumeStamina(float Amount)
{
	if (OwnerChar)
	{
		float NewStamina = OwnerChar->GetCurrentStamina() - Amount;
		OwnerChar->SetCurrentStamina(NewStamina);
		UE_LOG(LogTemp, Display, TEXT("Consume Stamina: %.1f, Remaining Stamina: %.1f"), Amount,OwnerChar->GetCurrentStamina());
	}
}


// 스킬&아이템 슬롯 함수
void UT3CombatComponent::ChangeActiveSlot(ESlotType Type)
{
	if (Type == ESlotType::Skill && SkillComp)
	{
		if (SkillComp->GetSkillIDBySlotIndex(2) == 0) return;

		// 1. 실제 데이터 스왑
		SkillComp->SwapSkills();

		// 2. 스왑 후의 데이터를 가져와서 정확하게 보고
		int32 S1_ID = SkillComp->GetSkillIDBySlotIndex(1);
		int32 S2_ID = SkillComp->GetSkillIDBySlotIndex(2);

		// GetSkillDataFull: 직업 스킬(1~4)과 보스 스킬(5~8) 모두 조회
		FSkillData* Data1 = SkillComp->GetSkillDataFull(S1_ID);
		FSkillData SafeData1 = Data1 ? *Data1 : FSkillData();
		SkillComp->OnSkillSlotUpdated.Broadcast(1, S1_ID, SafeData1);

		FSkillData* Data2 = SkillComp->GetSkillDataFull(S2_ID);
		FSkillData SafeData2 = Data2 ? *Data2 : FSkillData();
		SkillComp->OnSkillSlotUpdated.Broadcast(2, S2_ID, SafeData2);
	}
}

void UT3CombatComponent::ExecuteCurrentSlotAction(ESlotType Type)
{
	switch (Type)
	{
	case ESlotType::Skill:
	{
		bIsBasicAttacking = false;
		if (!SkillComp) break;

		int32 CurrentSkillID = SkillComp->GetSkillIDBySlotIndex(CurrentSkillSlot);
		if (UT3CommonSkillComponent::IsBossSkillID(CurrentSkillID) && CommonSkillComp)
		{
			// 보스 스킬: CommonSkillComponent에서 실행
			CommonSkillComp->ExecuteBossSkill(CurrentSkillID);
		}
		else
		{
			// 직업 스킬: 기존 방식
			SkillComp->ExecuteSkill(CurrentSkillSlot);
		}
		break;
	}
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

void UT3CombatComponent::ExecuteCurrentSlotAction_Completed(ESlotType Type)
{
	switch (Type)
	{
	case ESlotType::Skill:
	{
		if (!SkillComp) break;

		int32 CurrentSkillID = SkillComp->GetSkillIDBySlotIndex(CurrentSkillSlot);
		if (UT3CommonSkillComponent::IsBossSkillID(CurrentSkillID) && CommonSkillComp)
		{
			CommonSkillComp->ExecuteBossSkillCompleted(CurrentSkillID);
		}
		else
		{
			SkillComp->ExecuteSkill_Completed(CurrentSkillSlot);
		}
		break;
	}
	case ESlotType::Consumable:
		break;
	case ESlotType::Potion:
		break;
	}
}

// 인벤토리에서 호출할 스킬 슬롯 업데이트 함수
UFUNCTION(BlueprintCallable)
void UT3CombatComponent::RequestUpdateSkill(int32 SkillID, bool bIsEquip)
{
	if (SkillComp)
	{
		SkillComp->SetSkillSlot(SkillID, bIsEquip);
	}
}

float UT3CombatComponent::GetHolyGaugeChargeAmount() const
{
	return HolyGaugeChargeAmount;
}

void UT3CombatComponent::SetHolyGaugeChargeAmount(float NewAmount)
{
	HolyGaugeChargeAmount = NewAmount;
}


void UT3CombatComponent::PlaySkillEffectSound(USoundBase* Sound, float Volume)
{
	if (Sound && GetWorld())
	{
		UGameplayStatics::PlaySoundAtLocation(this, Sound, GetOwner()->GetActorLocation(), Volume);
	}
}