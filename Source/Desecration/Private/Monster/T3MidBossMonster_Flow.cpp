// T3MidBossMonster_Flow.cpp — 활성화, 인트로, 사망 연출, 디졸브, 이동/회전/워프

#include "Monster/T3MidBossMonster.h"
#include "Monster/T3BossWeaponComponent.h"
#include "Monster/T3MidBossHPBarWidget.h"
#include "Monster/T3MidBossMaterialSet.h"
#include "Desecration.h"
#include "AIController.h"
#include "Blueprint/UserWidget.h"
#include "Components/AudioComponent.h"
#include "Components/CapsuleComponent.h"

#include "Components/TimelineComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"

// ============================================================
// 활성화 트리거 (플레이어 접근 감지)
// ============================================================

void AT3MidBossMonster::OnExternalTriggerOverlap(AActor* OverlappedActor, AActor* OtherActor)
{
	if (bIsActivated || IsDead())
	{
		return;
	}

	// 플레이어 폰인지 확인
	APawn* OtherPawn = Cast<APawn>(OtherActor);
	if (!OtherPawn || !OtherPawn->IsPlayerControlled())
	{
		return;
	}

	UE_LOG(LogDesecration, Warning, TEXT("T3_MidBoss: ★ 외부 트리거 활성화 — %s"), *OtherActor->GetName());

	ActivateBoss(OtherActor);
}

// ============================================================
// 외부 활성화 트리거 바인딩 (에디터 직접 배치 / 스포너 런타임 모두 대응)
// ============================================================

void AT3MidBossMonster::BindExternalTrigger(AActor* Trigger)
{
	if (!Trigger)
	{
		UE_LOG(LogDesecration, Warning, TEXT("T3_MidBoss: BindExternalTrigger — Trigger가 nullptr"));
		return;
	}

	ExternalActivationTrigger = Trigger;
	Trigger->OnActorBeginOverlap.AddDynamic(this, &AT3MidBossMonster::OnExternalTriggerOverlap);

	UE_LOG(LogDesecration, Log, TEXT("T3_MidBoss: 외부 활성화 트리거 바인딩 완료 — %s"), *Trigger->GetName());
}

// ============================================================
// 입장 (Entry) — 외부 트리거에서 호출
// ============================================================

void AT3MidBossMonster::ActivateBoss(AActor* Activator)
{
	if (bIsActivated || IsDead())
	{
		return;
	}

	if (!Activator)
	{
		UE_LOG(LogDesecration, Warning, TEXT("T3_MidBoss: ActivateBoss — Activator가 nullptr"));
		return;
	}

	bIsActivated = true;
	CombatTarget = Activator;

	// 외부 트리거 비활성화 (중복 방지)
	if (ExternalActivationTrigger)
	{
		ExternalActivationTrigger->SetActorEnableCollision(false);
	}

	// BGM 재생 (2D — 공간 감쇠 없이 음악처럼 재생)
	if (BossBGM)
	{
		BGMAudioComponent = UGameplayStatics::SpawnSound2D(this, BossBGM, BGMVolume);
	}

	// 보스 HP바 위젯 생성 (PlayerController 기반)
	if (BossHPBarWidgetClass)
	{
		if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
		{
			BossHPBarWidget = CreateWidget<UT3MidBossHPBarWidget>(PC, BossHPBarWidgetClass);
			if (BossHPBarWidget)
			{
				BossHPBarWidget->TargetBoss = this;
				BossHPBarWidget->AddToViewport();
			}
		}
	}

	OnMidBossActivated.Broadcast();

	UE_LOG(LogDesecration, Log, TEXT("T3_MidBoss: %s 활성화 (타겟: %s, BGM: %s, HPBar: %s)"),
		*BossDisplayName.ToString(), *Activator->GetName(),
		BossBGM ? TEXT("O") : TEXT("X"),
		BossHPBarWidget ? TEXT("O") : TEXT("X"));

	// 인트로 몽타주 재생 → 완료 후 StateTree 시작
	if (IntroMontage)
	{
		const float Duration = PlayAnimMontage(IntroMontage);
		if (Duration > 0.f)
		{
			UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
			if (AnimInstance)
			{
				FOnMontageEnded EndDelegate;
				EndDelegate.BindUObject(this, &AT3MidBossMonster::OnIntroMontageEnded);
				AnimInstance->Montage_SetEndDelegate(EndDelegate, IntroMontage);
			}

			UE_LOG(LogDesecration, Log, TEXT("T3_MidBoss: 인트로 몽타주 재생 (%.1f초)"), Duration);
			return;
		}
	}

	// 인트로 없으면 즉시 AI 로직 시작
	StartBossLogic();
}

void AT3MidBossMonster::OnIntroMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	UE_LOG(LogDesecration, Log, TEXT("T3_MidBoss: 인트로 몽타주 %s — AI 로직 시작"),
		bInterrupted ? TEXT("중단됨") : TEXT("완료"));

	StartBossLogic();
}

void AT3MidBossMonster::StartBossLogic()
{
	if (StateTreeComponent)
	{
		StateTreeComponent->StartLogic();
	}

	UE_LOG(LogDesecration, Log, TEXT("T3_MidBoss: %s StateTree 시작"), *BossDisplayName.ToString());
}

// ============================================================
// 타겟 방향 회전
// ============================================================

void AT3MidBossMonster::FaceTarget(float InterpSpeed)
{
	if (!CombatTarget)
	{
		return;
	}

	const FVector Direction = (CombatTarget->GetActorLocation() - GetActorLocation()).GetSafeNormal2D();
	if (!Direction.IsNearlyZero())
	{
		const FRotator TargetRotation = Direction.Rotation();
		SetActorRotation(FMath::RInterpTo(
			GetActorRotation(), TargetRotation,
			GetWorld()->GetDeltaSeconds(), InterpSpeed));
	}
}

// ============================================================
// 유틸리티 함수
// ============================================================

void AT3MidBossMonster::MoveToTarget(float Duration, float Distance)
{
	if (!CombatTarget || Duration <= 0.f)
	{
		return;
	}

	const FVector ToTarget = CombatTarget->GetActorLocation() - GetActorLocation();
	const FVector Direction = ToTarget.GetSafeNormal2D();

	bIsMovingToTarget = true;
	MoveToTargetElapsed = 0.f;
	MoveToTargetDuration = Duration;
	MoveToTargetDirection = Direction;
	MoveToTargetSpeed = Distance / Duration;

	UE_LOG(LogDesecration, Verbose,
		TEXT("T3_MidBoss: MoveToTarget 시작 (Duration:%.2f, Distance:%.0f)"), Duration, Distance);
}

void AT3MidBossMonster::UpdateMotionWarpTarget()
{
	if (!MotionWarpingComponent || !CombatTarget)
	{
		return;
	}

	// 회전용 — 실시간 추적 (플레이어가 움직여도 방향을 따라감)
	MotionWarpingComponent->AddOrUpdateWarpTargetFromComponent(
		MotionWarpTargetRotationName,
		CombatTarget->GetRootComponent(),
		NAME_None,
		true,  // bFollowComponent — 회전은 실시간
		EWarpTargetLocationOffsetDirection::VectorFromTargetToOwner,
		FVector::ZeroVector
	);

	// 이동용 — 호출 시점의 위치를 스냅샷 (실시간 추적하지 않음)
	const FVector BossLoc = GetActorLocation();
	const FVector TargetLoc = CombatTarget->GetActorLocation();
	const float Distance = FVector::Dist(BossLoc, TargetLoc);

	// 근거리 시 워프 비활성화 — 제자리 공격
	if (Distance <= WarpDisableDistance)
	{
		MotionWarpingComponent->AddOrUpdateWarpTargetFromLocation(
			MotionWarpTargetName, BossLoc);
		UE_LOG(LogDesecration, Log,
			TEXT("T3_MidBoss: 워프 스킵 — 거리:%.0f ≤ %.0f, 제자리 공격"),
			Distance, WarpDisableDistance);
		return;
	}

	// 체인 엔트리별 오버라이드 확인 (0 이하면 보스 기본값)
	float EffectiveMaxWarp = MaxWarpDistance;
	const FMidBossAttackPattern* PatternData = FindPatternData(CurrentPatternName);
	if (PatternData && PatternData->MontageChain.IsValidIndex(CurrentChainIndex))
	{
		const float Override = PatternData->MontageChain[CurrentChainIndex].MaxWarpDistanceOverride;
		if (Override > 0.f)
		{
			EffectiveMaxWarp = Override;
		}
	}

	// MaxWarpDistance 초과 시 — 최대 사거리 지점으로 클램핑
	const float WarpDistance = FMath::Min(Distance, EffectiveMaxWarp);

	// 타겟 방향으로 워프할 최종 위치 계산 (오프셋 적용)
	const FVector Direction = (TargetLoc - BossLoc).GetSafeNormal();
	const float ClampedOffset = FMath::Clamp(WarpTargetOffset, 0.f, WarpDistance - MinWarpDistance);
	const FVector FinalLocation = BossLoc + Direction * (WarpDistance - ClampedOffset);

	MotionWarpingComponent->AddOrUpdateWarpTargetFromLocation(
		MotionWarpTargetName,
		FinalLocation
	);

	UE_LOG(LogDesecration, Log,
		TEXT("T3_MidBoss: 워프 스냅샷 — Distance:%.0f, EffectiveMaxWarp:%.0f, WarpDist:%.0f, Offset:%.0f, 패턴:'%s' 체인[%d]"),
		Distance, EffectiveMaxWarp, WarpDistance, ClampedOffset,
		*CurrentPatternName.ToString(), CurrentChainIndex);
}

// ============================================================
// 사망 연출 (Death Sequence)
// ============================================================

void AT3MidBossMonster::BeginDeathSequence()
{
	// 록온 해제 — "Enemy" 태그 제거 + 록온 위젯 숨김
	// CombatComponent가 TickComponent에서 태그 부재 감지 시 자동 해제
	Tags.Remove(FName("Enemy"));
	SetLockOnWidgetVisible(false);

	// 사망 사운드 재생
	if (DeathSound)
	{
		UGameplayStatics::PlaySoundAtLocation(
			this, DeathSound, GetActorLocation(), FRotator::ZeroRotator,
			SoundVolume * DeathVolumeMultiplier, 1.f, 0.f, SoundAttenuationSettings);
	}

	// BGM 페이드아웃
	if (BGMAudioComponent && BGMAudioComponent->IsPlaying())
	{
		BGMAudioComponent->FadeOut(BGMFadeOutDuration, 0.f);
	}

	// 사망 몽타주 재생
	if (DeathMontage)
	{
		const float Duration = PlayAnimMontage(DeathMontage);
		if (Duration > 0.f)
		{
			UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
			if (AnimInstance)
			{
				// BlendingOut 시점에 콜백 — 블렌드아웃 전에 포즈 고정
				FOnMontageBlendingOutStarted BlendOutDelegate;
				BlendOutDelegate.BindUObject(this, &AT3MidBossMonster::OnDeathMontageEnded);
				AnimInstance->Montage_SetBlendingOutDelegate(BlendOutDelegate, DeathMontage);
			}

			UE_LOG(LogDesecration, Log, TEXT("T3_MidBoss: 사망 몽타주 재생 (%.1f초)"), Duration);
			return;
		}
	}

	// 몽타주 없거나 재생 실패 시 즉시 완료
	FinishDeathSequence();
}

void AT3MidBossMonster::OnDeathMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	// 사망 포즈 고정 — ABP가 Idle로 복귀하지 않도록 애니메이션 정지
	GetMesh()->bPauseAnims = true;

	FinishDeathSequence();
}

void AT3MidBossMonster::FinishDeathSequence()
{
	// 무기 드롭 (bAutoDropWeapon=false면 AnimNotify 등에서 수동 호출)
	if (bAutoDropWeapon && WeaponComponent)
	{
		WeaponComponent->DropWeapon();
	}

	// 캡슐 충돌 해제 — 플레이어 통과 가능
	if (UCapsuleComponent* Capsule = GetCapsuleComponent())
	{
		Capsule->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	// 이동 비활성화
	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		MoveComp->DisableMovement();
	}

	// AI 회전 정지
	if (AAIController* AIC = Cast<AAIController>(GetController()))
	{
		AIC->ClearFocus(EAIFocusPriority::Gameplay);
	}

	// StateTree 정지
	if (StateTreeComponent)
	{
		StateTreeComponent->StopLogic(TEXT("Death"));
	}

	// 사망 연출 완료 델리게이트 — Level BP에서 안개벽 해제, 보상 등
	OnMidBossDeathFinished.Broadcast();

	// 디졸브 시작 (활성화 시) — 디졸브 완료 후 SetLifeSpan
	if (bEnableDissolve && DynamicMaterials.Num() > 0)
	{
		StartDissolve();
		return;
	}

	// 디졸브 비활성화 시 기존 방식
	SetLifeSpan(DeathCleanupDelay);

	UE_LOG(LogDesecration, Log, TEXT("T3_MidBoss: %s 사망 연출 완료 (%.0f초 후 제거)"),
		*BossDisplayName.ToString(), DeathCleanupDelay);
}

// ============================================================
// 디졸브 연출
// ============================================================

void AT3MidBossMonster::CreateDynamicMaterials()
{
	USkeletalMeshComponent* MeshComp = GetMesh();
	if (!MeshComp) return;

	const int32 NumMaterials = MeshComp->GetNumMaterials();
	DynamicMaterials.Reserve(NumMaterials);

	for (int32 i = 0; i < NumMaterials; ++i)
	{
		UMaterialInstanceDynamic* DynMat = MeshComp->CreateAndSetMaterialInstanceDynamic(i);
		if (DynMat)
		{
			DynamicMaterials.Add(DynMat);
		}
	}

	UE_LOG(LogDesecration, Verbose, TEXT("T3_MidBoss: DynamicMaterial %d개 생성"), DynamicMaterials.Num());
}

void AT3MidBossMonster::StartDissolve()
{
	if (!DissolveTimeline)
	{
		UE_LOG(LogDesecration, Warning, TEXT("T3_MidBoss: DissolveTimeline이 nullptr — 디졸브 스킵"));
		SetLifeSpan(DeathCleanupDelay);
		return;
	}

	// 디졸브 사운드 재생
	if (DissolveSound)
	{
		UGameplayStatics::PlaySoundAtLocation(
			this, DissolveSound, GetActorLocation(), FRotator::ZeroRotator,
			SoundVolume, 1.f, 0.f, SoundAttenuationSettings);
	}

	DissolveTimeline->PlayFromStart();

	UE_LOG(LogDesecration, Log, TEXT("T3_MidBoss: %s 디졸브 시작 (%.1f초, 머티리얼 %d개)"),
		*BossDisplayName.ToString(), DissolveDuration, DynamicMaterials.Num());
}

void AT3MidBossMonster::OnDissolveUpdate(float Value)
{
	for (UMaterialInstanceDynamic* DynMat : DynamicMaterials)
	{
		if (DynMat)
		{
			DynMat->SetScalarParameterValue(DissolveParameterName, Value);
		}
	}
}

void AT3MidBossMonster::OnDissolveFinished()
{
	UE_LOG(LogDesecration, Log, TEXT("T3_MidBoss: %s 본체 디졸브 완료"), *BossDisplayName.ToString());

	// 본체 메시 숨김
	if (GetMesh())
	{
		GetMesh()->SetVisibility(false);
	}

	// 무기 디졸브 시작 (1초 딜레이)
	if (WeaponComponent && WeaponComponent->WeaponMeshComponent)
	{
		FTimerHandle WeaponDissolveTimer;
		TWeakObjectPtr<AT3MidBossMonster> WeakThis(this);
		GetWorldTimerManager().SetTimer(WeaponDissolveTimer, [WeakThis]()
		{
			if (!WeakThis.IsValid())
			{
				return;
			}

			AT3MidBossMonster* Self = WeakThis.Get();
			if (Self->WeaponComponent)
			{
				Self->WeaponComponent->StartWeaponDissolve(
					Self->WeaponComponent->WeaponDissolveDuration,
					Self->WeaponComponent->WeaponDissolveParameterName);
			}

			// 무기 디졸브 완료 후 액터 제거
			Self->SetLifeSpan(Self->WeaponComponent ? Self->WeaponComponent->WeaponDissolveDuration + 0.5f : 3.f);

		}, 1.f, false);

		UE_LOG(LogDesecration, Log, TEXT("T3_MidBoss: 1초 후 무기 디졸브 예정"));
		return;
	}

	// 무기 없으면 바로 제거
	SetLifeSpan(0.5f);
}

// ============================================================
// 머티리얼 세트 적용
// ============================================================

void AT3MidBossMonster::ApplyMaterialSet(const UT3MidBossMaterialSet* MaterialSet)
{
	if (!MaterialSet)
	{
		return;
	}

	// 본체 스켈레탈 메시 머티리얼 적용
	if (USkeletalMeshComponent* BodyMesh = GetMesh())
	{
		const int32 SlotCount = FMath::Min(MaterialSet->BodyMaterials.Num(), BodyMesh->GetNumMaterials());
		for (int32 i = 0; i < SlotCount; ++i)
		{
			if (MaterialSet->BodyMaterials[i])
			{
				BodyMesh->SetMaterial(i, MaterialSet->BodyMaterials[i]);
			}
		}
	}

	// 무기 머티리얼 적용
	if (MaterialSet->WeaponMaterial && WeaponComponent && WeaponComponent->WeaponMeshComponent)
	{
		WeaponComponent->WeaponMeshComponent->SetMaterial(0, MaterialSet->WeaponMaterial);
	}

	// 디졸브용 DynamicMaterial 재생성 — SetMaterial()이 기존 DMI를 무효화하므로
	if (bEnableDissolve)
	{
		DynamicMaterials.Empty();
		CreateDynamicMaterials();

		// 디졸브/프레넬 색상 오버라이드 적용
		for (UMaterialInstanceDynamic* DynMat : DynamicMaterials)
		{
			if (!DynMat) continue;

			if (MaterialSet->bOverrideDissolveColor)
			{
				DynMat->SetVectorParameterValue(TEXT("DissolveEv"), MaterialSet->DissolveColor);
			}
			if (MaterialSet->bOverrideFresnelColor)
			{
				DynMat->SetVectorParameterValue(TEXT("ColorFresnel"), MaterialSet->FresnelColor);
			}
		}
	}

	UE_LOG(LogDesecration, Log, TEXT("T3_MidBoss: %s 머티리얼 세트 적용 (본체:%d슬롯, 무기:%s, DMI재생성:%s)"),
		*BossDisplayName.ToString(),
		MaterialSet->BodyMaterials.Num(),
		MaterialSet->WeaponMaterial ? TEXT("O") : TEXT("X"),
		bEnableDissolve ? TEXT("O") : TEXT("X"));
}
