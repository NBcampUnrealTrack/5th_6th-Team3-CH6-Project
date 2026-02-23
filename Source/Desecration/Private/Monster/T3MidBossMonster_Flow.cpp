// T3MidBossMonster_Flow.cpp — 활성화, 인트로, 사망 연출, 디졸브, 이동/회전/워프

#include "Monster/T3MidBossMonster.h"
#include "Monster/T3BossWeaponComponent.h"
#include "Monster/T3MidBossHPBarWidget.h"
#include "Desecration.h"
#include "AIController.h"
#include "Blueprint/UserWidget.h"
#include "Components/AudioComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SphereComponent.h"
#include "Components/TimelineComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"

// ============================================================
// 활성화 트리거 (플레이어 접근 감지)
// ============================================================

void AT3MidBossMonster::OnActivationTriggerOverlap(
	UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
	bool bFromSweep, const FHitResult& SweepResult)
{
	if (bIsActivated || IsDead())
	{
		return;
	}

	// 오버랩 발생 액터 로그 (디버그)
	const float Distance = FVector::Dist(GetActorLocation(), OtherActor->GetActorLocation());
	UE_LOG(LogDesecration, Warning, TEXT("T3_MidBoss: 트리거 오버랩 발생 — Actor=%s, Class=%s, 거리=%.0f, Radius=%.0f"),
		*OtherActor->GetName(), *OtherActor->GetClass()->GetName(), Distance, ActivationRadius);

	// 플레이어 폰인지 확인
	APawn* OtherPawn = Cast<APawn>(OtherActor);
	if (!OtherPawn || !OtherPawn->IsPlayerControlled())
	{
		UE_LOG(LogDesecration, Warning, TEXT("T3_MidBoss: 트리거 무시 — 플레이어 아님 (IsPawn=%s, IsPlayerControlled=%s)"),
			OtherPawn ? TEXT("Y") : TEXT("N"),
			(OtherPawn && OtherPawn->IsPlayerControlled()) ? TEXT("Y") : TEXT("N"));
		return;
	}

	UE_LOG(LogDesecration, Warning, TEXT("T3_MidBoss: ★ 활성화 트리거 통과 — %s, 거리=%.0f"), *OtherActor->GetName(), Distance);

	ActivateBoss(OtherActor);
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

	// 트리거 비활성화 (외부 호출 시에도 중복 방지)
	if (ActivationTriggerSphere)
	{
		ActivationTriggerSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
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
		*BossName, *Activator->GetName(),
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

	UE_LOG(LogDesecration, Log, TEXT("T3_MidBoss: %s StateTree 시작"), *BossName);
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

	const float Distance = FVector::Dist(GetActorLocation(), CombatTarget->GetActorLocation());

	// 회전용 — 거리와 무관하게 항상 갱신 (타겟 방향은 바라봐야 함)
	MotionWarpingComponent->AddOrUpdateWarpTargetFromComponent(
		MotionWarpTargetRotationName,
		CombatTarget->GetRootComponent(),
		NAME_None,
		true,  // bFollowComponent
		EWarpTargetLocationOffsetDirection::VectorFromTargetToOwner,
		FVector::ZeroVector
	);

	// MaxWarpDistance 초과 — 이동 워프 제거 (제자리 루트모션 공격)
	if (Distance > MaxWarpDistance)
	{
		MotionWarpingComponent->RemoveWarpTarget(MotionWarpTargetName);

		UE_LOG(LogDesecration, Verbose,
			TEXT("T3_MidBoss: 워프 거리 초과 — 이동 워프 제거 (Distance:%.0f > Max:%.0f)"),
			Distance, MaxWarpDistance);
		return;
	}

	// 이동용 — 거리 기반 동적 오프셋 (근거리 후진 방지)
	const float ClampedOffset = FMath::Clamp(WarpTargetOffset, 0.f, Distance - MinWarpDistance);

	MotionWarpingComponent->AddOrUpdateWarpTargetFromComponent(
		MotionWarpTargetName,
		CombatTarget->GetRootComponent(),
		NAME_None,
		true,  // bFollowComponent
		EWarpTargetLocationOffsetDirection::VectorFromTargetToOwner,
		FVector(ClampedOffset, 0.f, 0.f)
	);
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
			this, DeathSound, GetActorLocation(),
			SoundVolume * DeathVolumeMultiplier);
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
		*BossName, DeathCleanupDelay);
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
		UGameplayStatics::PlaySoundAtLocation(this, DissolveSound, GetActorLocation());
	}

	DissolveTimeline->PlayFromStart();

	UE_LOG(LogDesecration, Log, TEXT("T3_MidBoss: %s 디졸브 시작 (%.1f초, 머티리얼 %d개)"),
		*BossName, DissolveDuration, DynamicMaterials.Num());
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
	UE_LOG(LogDesecration, Log, TEXT("T3_MidBoss: %s 본체 디졸브 완료"), *BossName);

	// 본체 메시 숨김
	if (GetMesh())
	{
		GetMesh()->SetVisibility(false);
	}

	// 무기 디졸브 시작 (1초 딜레이)
	if (WeaponComponent && WeaponComponent->WeaponMeshComponent)
	{
		FTimerHandle WeaponDissolveTimer;
		GetWorldTimerManager().SetTimer(WeaponDissolveTimer, [this]()
		{
			if (WeaponComponent)
			{
				WeaponComponent->StartWeaponDissolve(
					WeaponComponent->WeaponDissolveDuration,
					WeaponComponent->WeaponDissolveParameterName);
			}

			// 무기 디졸브 완료 후 액터 제거
			SetLifeSpan(WeaponComponent ? WeaponComponent->WeaponDissolveDuration + 0.5f : 3.f);

		}, 1.f, false);

		UE_LOG(LogDesecration, Log, TEXT("T3_MidBoss: 1초 후 무기 디졸브 예정"));
		return;
	}

	// 무기 없으면 바로 제거
	SetLifeSpan(0.5f);
}
