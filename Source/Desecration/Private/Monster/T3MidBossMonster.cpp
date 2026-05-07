// T3MidBossMonster.cpp — Core (생성자, BeginPlay, Tick, 상태 태그, 록온)

#include "Monster/T3MidBossMonster.h"
#include "Monster/T3BossWeaponComponent.h"
#include "Desecration.h"
#include "AIController.h"

#include "Components/TimelineComponent.h"
#include "Components/WidgetComponent.h"
#include "Curves/CurveFloat.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "NativeGameplayTags.h"

// Gameplay Tag 네이티브 정의 — 상태 태그 (State + Event 겸용: SendStateTreeEvent에도 사용)
UE_DEFINE_GAMEPLAY_TAG(TAG_Boss_State_Dead, "Boss.State.Dead");
UE_DEFINE_GAMEPLAY_TAG(TAG_Boss_State_Stunned, "Boss.State.Stunned");
UE_DEFINE_GAMEPLAY_TAG(TAG_Boss_State_ExecutingPattern, "Boss.State.ExecutingPattern");
UE_DEFINE_GAMEPLAY_TAG(TAG_Boss_State_SuperArmor, "Boss.State.SuperArmor");
UE_DEFINE_GAMEPLAY_TAG(TAG_Boss_State_ParryWindow, "Boss.State.ParryWindow");
UE_DEFINE_GAMEPLAY_TAG(TAG_Boss_State_Disengaging, "Boss.State.Disengaging");
UE_DEFINE_GAMEPLAY_TAG(TAG_Boss_State_Invulnerable, "Boss.State.Invulnerable");
UE_DEFINE_GAMEPLAY_TAG(TAG_Boss_State_Blocking, "Boss.State.Blocking");
// 막기/회피 직후 짧은 윈도우 — Consideration이 리액션 패턴 가중치 부풀림 (※ 기존 ParryWindow 카운터 패턴과 무관)
UE_DEFINE_GAMEPLAY_TAG(TAG_Boss_State_PostBlock, "Boss.State.PostBlock");
UE_DEFINE_GAMEPLAY_TAG(TAG_Boss_State_PostRoll, "Boss.State.PostRoll");
// Gameplay Tag 네이티브 정의 — StateTree 전용 이벤트 태그 (대응 State 없음)
UE_DEFINE_GAMEPLAY_TAG(TAG_Boss_Event_StunRecovered, "Boss.Event.StunRecovered");
UE_DEFINE_GAMEPLAY_TAG(TAG_Boss_Event_ActionCountDepleted, "Boss.Event.ActionCountDepleted");
UE_DEFINE_GAMEPLAY_TAG(TAG_Boss_Event_ParriedByPlayer, "Boss.Event.ParriedByPlayer");
UE_DEFINE_GAMEPLAY_TAG(TAG_Boss_Event_BlockReaction, "Boss.Event.BlockReaction");

AT3MidBossMonster::AT3MidBossMonster()
{
	PrimaryActorTick.bCanEverTick = true;

	ConfigureRotationSettings();

	// StateTree 컴포넌트 (standalone 스키마 — AI Controller 없이 동작)
	StateTreeComponent = CreateDefaultSubobject<UStateTreeComponent>(TEXT("StateTreeComponent"));
	StateTreeComponent->SetStartLogicAutomatically(false);

	// MotionWarping 컴포넌트
	MotionWarpingComponent = CreateDefaultSubobject<UMotionWarpingComponent>(TEXT("MotionWarpingComponent"));

	// 무기 컴포넌트
	WeaponComponent = CreateDefaultSubobject<UT3BossWeaponComponent>(TEXT("WeaponComponent"));

	// 디졸브 타임라인 컴포넌트
	DissolveTimeline = CreateDefaultSubobject<UTimelineComponent>(TEXT("DissolveTimeline"));

	CreateLockOnWidget();

	// 활성화 트리거: 외부 액터 레퍼런스 방식 (ExternalActivationTrigger를 에디터에서 지정)
}

// 회전 설정 — 컨트롤러 직접 회전 off + MovementComponent가 SetFocus 방향으로 부드럽게 보간
void AT3MidBossMonster::ConfigureRotationSettings()
{
	bUseControllerRotationYaw = false;
	bUseControllerRotationPitch = false;
	bUseControllerRotationRoll = false;

	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		MoveComp->bOrientRotationToMovement = false;       // 이동 방향 회전 OFF (스트레이프 시 타겟 바라보기 위해)
		MoveComp->bUseControllerDesiredRotation = true;    // 컨트롤러 SetFocus 방향으로 부드럽게 보간
		MoveComp->RotationRate = FRotator(0.f, 360.f, 0.f); // 초당 360도 (에디터에서 조절 가능)
	}
}

// 록온 위젯 컴포넌트 생성 + 스크린 공간/크기/초기 숨김 설정
void AT3MidBossMonster::CreateLockOnWidget()
{
	LockOnWidgetComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("LockOnWidget"));
	LockOnWidgetComponent->SetupAttachment(GetMesh(), TEXT("LockOn_Socket"));
	LockOnWidgetComponent->SetWidgetSpace(EWidgetSpace::Screen);
	LockOnWidgetComponent->SetDrawSize(FVector2D(30.f, 15.f));
	LockOnWidgetComponent->SetVisibility(false);
	LockOnWidgetComponent->SetRelativeLocation(FVector::ZeroVector);
}

// ============================================================
// 상태 태그 헬퍼
// ============================================================

void AT3MidBossMonster::AddStateTag(FGameplayTag Tag)
{
	ActiveGameplayTags.AddTag(Tag);
}

void AT3MidBossMonster::RemoveStateTag(FGameplayTag Tag)
{
	ActiveGameplayTags.RemoveTag(Tag);
}

bool AT3MidBossMonster::HasStateTag(FGameplayTag Tag) const
{
	return ActiveGameplayTags.HasTag(Tag);
}

void AT3MidBossMonster::ApplyTransientStateTag(FGameplayTag Tag, float Duration)
{
	if (!Tag.IsValid())
	{
		return;
	}

	// 즉시 부여 (이미 있어도 멱등)
	ActiveGameplayTags.AddTag(Tag);

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	// 기존 핸들이 있으면 재호출 시 타이머 갱신 (연장) — 핸들 매니저에서 자동 무효화
	FTimerHandle& Handle = TransientStateTagHandles.FindOrAdd(Tag);
	World->GetTimerManager().ClearTimer(Handle);

	if (Duration <= 0.f)
	{
		// 0/음수면 다음 틱에 즉시 제거 (사실상 즉시)
		World->GetTimerManager().SetTimerForNextTick([WeakThis = TWeakObjectPtr<AT3MidBossMonster>(this), Tag]()
		{
			if (AT3MidBossMonster* Self = WeakThis.Get())
			{
				Self->ActiveGameplayTags.RemoveTag(Tag);
				Self->TransientStateTagHandles.Remove(Tag);
			}
		});
		return;
	}

	World->GetTimerManager().SetTimer(Handle,
		FTimerDelegate::CreateWeakLambda(this, [this, Tag]()
		{
			ActiveGameplayTags.RemoveTag(Tag);
			TransientStateTagHandles.Remove(Tag);
		}),
		Duration, false);
}

bool AT3MidBossMonster::IsDead() const
{
	return ActiveGameplayTags.HasTag(TAG_Boss_State_Dead);
}

bool AT3MidBossMonster::IsStunned() const
{
	return ActiveGameplayTags.HasTag(TAG_Boss_State_Stunned);
}

bool AT3MidBossMonster::IsExecutingPattern() const
{
	return ActiveGameplayTags.HasTag(TAG_Boss_State_ExecutingPattern);
}

bool AT3MidBossMonster::HasSuperArmor() const
{
	return ActiveGameplayTags.HasTag(TAG_Boss_State_SuperArmor);
}

bool AT3MidBossMonster::IsDisengaging() const
{
	return ActiveGameplayTags.HasTag(TAG_Boss_State_Disengaging);
}

bool AT3MidBossMonster::IsInvulnerable() const
{
	return ActiveGameplayTags.HasTag(TAG_Boss_State_Invulnerable);
}

bool AT3MidBossMonster::IsBlocking() const
{
	return ActiveGameplayTags.HasTag(TAG_Boss_State_Blocking);
}

bool AT3MidBossMonster::IsHitFromFront(const AActor* DamageCauser) const
{
	if (!DamageCauser)
	{
		// 공격자 정보 없음 — 정면 처리 (안전 기본값, 막기 작동 보장)
		return true;
	}

	const FVector ToAttacker = (DamageCauser->GetActorLocation() - GetActorLocation()).GetSafeNormal2D();
	if (ToAttacker.IsNearlyZero())
	{
		return true;
	}

	const FVector Forward = GetActorForwardVector().GetSafeNormal2D();
	// 앞 반구 = Dot >= 0 (90도 기준 — 좌우 정밀 판정은 추후 뒤잡 도입 시 강화)
	return FVector::DotProduct(Forward, ToAttacker) >= 0.f;
}

// ============================================================
// 에디터 전용
// ============================================================

#if WITH_EDITOR
void AT3MidBossMonster::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	// MontageChain 내 DisplayTitle 자동 갱신
	for (FMidBossAttackPattern& Pattern : AttackPatterns)
	{
		for (FPatternMontageData& Entry : Pattern.MontageChain)
		{
			Entry.UpdateDisplayTitle();
		}
	}
}
#endif

// ============================================================
// BeginPlay / Tick
// ============================================================

void AT3MidBossMonster::BeginPlay()
{
	Super::BeginPlay();

	AttachLockOnWidgetToSocket();
	BindWeaponComponent();
	InitializeStatsAndTags();
	SetupDissolveTimeline();

	// 외부 활성화 트리거 바인딩 (에디터 직접 배치 시 — 스포너 경로는 SpawnAndPrepareBoss에서 런타임 연결)
	if (ExternalActivationTrigger)
	{
		BindExternalTrigger(ExternalActivationTrigger);
	}
	else
	{
		UE_LOG(LogDesecration, Log, TEXT("T3_MidBoss: ExternalActivationTrigger 미지정 — 스포너에서 런타임 연결 대기"));
	}

	OnMidBossSpawned.Broadcast();

	UE_LOG(LogDesecration, Log, TEXT("T3_MidBoss: %s 스폰 완료 (HP: %.0f, Stage: %d, 등록 패턴: %d개, Modifier: %s, Dissolve: %s, ExtTrigger: %s)"),
		*BossDisplayName.ToString(), MidBossStats.MaxHP, BossStage, AttackPatterns.Num(),
		ModifierDataAsset ? TEXT("O") : TEXT("X"),
		bEnableDissolve ? TEXT("O") : TEXT("X"),
		ExternalActivationTrigger ? *ExternalActivationTrigger->GetName() : TEXT("없음"));
}

// 록온 위젯 소켓 재부착 (SetupAttachment는 런타임 보장 안 됨)
void AT3MidBossMonster::AttachLockOnWidgetToSocket()
{
	if (!LockOnWidgetComponent || !GetMesh())
	{
		return;
	}

	LockOnWidgetComponent->AttachToComponent(
		GetMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, TEXT("LockOn_Socket"));
	LockOnWidgetComponent->SetRelativeLocation(FVector::ZeroVector);

	UE_LOG(LogDesecration, Log, TEXT("T3_MidBoss: LockOnWidget 소켓 부착 완료 (%s)"),
		*LockOnWidgetComponent->GetAttachSocketName().ToString());
}

// 무기 소켓 부착 + 히트 델리게이트 바인딩
void AT3MidBossMonster::BindWeaponComponent()
{
	if (WeaponComponent)
	{
		WeaponComponent->AttachToSocket(GetMesh());
		WeaponComponent->OnWeaponHitActor.AddDynamic(this, &AT3MidBossMonster::OnWeaponHit);
	}
}

// 스탯/태그/NotifyModifier/이동속도 캐시 초기화
void AT3MidBossMonster::InitializeStatsAndTags()
{
	// 에디터 직접 배치 경로 전용 초기화 (스포너 경로에선 SpawnAndPrepareBoss가 MidBossStats를 덮어씀)
	MidBossStats.CurrentHP = MidBossStats.MaxHP;
	MidBossStats.CurrentStunGauge = 0.f;

	// 천사 장신구 슬로우 복원 기준값 캐시 — BP CDO에서 설정한 값을 보존
	// ActiveBaseWalkSpeed도 동일 초기화 (STNodes가 Strafe/Dash 진입 시 푸시, 종료 시 복원)
	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		DefaultMaxWalkSpeed = MoveComp->MaxWalkSpeed;
		ActiveBaseWalkSpeed = DefaultMaxWalkSpeed;
	}

	// 초기 상태 태그 설정 (기존 bSuperArmor = true 대체)
	AddStateTag(TAG_Boss_State_SuperArmor);

	// 노티파이 보정기 생성 및 초기화
	NotifyModifier = NewObject<UMidBossNotifyModifier>(this);
	NotifyModifier->Initialize(ModifierDataAsset);
}

// 디졸브용 Dynamic Material 생성 + Timeline 바인딩 (bEnableDissolve 활성 시)
void AT3MidBossMonster::SetupDissolveTimeline()
{
	if (!bEnableDissolve)
	{
		return;
	}

	CreateDynamicMaterials();

	// 커브 결정 — 에디터에서 할당한 커브 우선, 없으면 선형 자동 생성
	UCurveFloat* CurveToUse = DissolveCurve;
	if (!CurveToUse)
	{
		DefaultDissolveCurve = NewObject<UCurveFloat>(this);
		DefaultDissolveCurve->FloatCurve.AddKey(0.f, 0.f);
		DefaultDissolveCurve->FloatCurve.AddKey(DissolveDuration, 1.f);
		CurveToUse = DefaultDissolveCurve;
	}

	// Timeline 바인딩
	if (DissolveTimeline && CurveToUse)
	{
		FOnTimelineFloat UpdateDelegate;
		UpdateDelegate.BindUFunction(this, FName("OnDissolveUpdate"));

		FOnTimelineEvent FinishedDelegate;
		FinishedDelegate.BindUFunction(this, FName("OnDissolveFinished"));

		DissolveTimeline->AddInterpFloat(CurveToUse, UpdateDelegate, FName("DissolveTrack"));
		DissolveTimeline->SetTimelineFinishedFunc(FinishedDelegate);
		DissolveTimeline->SetTimelineLength(DissolveDuration);
		DissolveTimeline->SetLooping(false);
	}
}

void AT3MidBossMonster::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	UpdateMoveToTargetInterpolation(DeltaTime);
	UpdateAIFocusAndRotation();
	UpdateStaggerRecovery(DeltaTime);
}

// 경직치 자연 회복 — Rate>0인 보스만 동작 (다크나이트는 0이라 early-out)
// LastStaggerEventTime + Delay 이후부터 초당 Rate씩 감소
void AT3MidBossMonster::UpdateStaggerRecovery(float DeltaTime)
{
	if (StaggerRecoveryRate <= 0.f || IsStunned() || IsDead())
	{
		return;
	}

	if (MidBossStats.CurrentStunGauge <= 0.f)
	{
		return;
	}

	const UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	const double Now = World->GetTimeSeconds();
	if (Now - LastStaggerEventTime < StaggerRecoveryDelay)
	{
		return;
	}

	MidBossStats.CurrentStunGauge =
		FMath::Max(0.f, MidBossStats.CurrentStunGauge - StaggerRecoveryRate * DeltaTime);
}

// MoveToTarget 보간 처리 — 지정 방향/속도로 bIsMovingToTarget 동안 AddActorWorldOffset
void AT3MidBossMonster::UpdateMoveToTargetInterpolation(float DeltaTime)
{
	if (!bIsMovingToTarget)
	{
		return;
	}

	MoveToTargetElapsed += DeltaTime;

	if (MoveToTargetElapsed >= MoveToTargetDuration)
	{
		bIsMovingToTarget = false;
	}
	else
	{
		const FVector Delta = MoveToTargetDirection * MoveToTargetSpeed * DeltaTime;
		AddActorWorldOffset(Delta, true);
	}
}

// AIController SetFocus + MovementComponent RotationRate (공격 중/비공격 분기)
void AT3MidBossMonster::UpdateAIFocusAndRotation()
{
	AAIController* AIC = Cast<AAIController>(GetController());
	if (!AIC)
	{
		return;
	}

	if (CombatTarget && !IsStunned() && !IsDead())
	{
		AIC->SetFocus(CombatTarget);

		// 공격 중: 느린 트래킹 (다음 공격 시 스냅 방지) / 비공격: 빠른 트래킹
		if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
		{
			const float Rate = IsExecutingPattern() ? AttackRotationRate : IdleRotationRate;
			MoveComp->RotationRate = FRotator(0.f, Rate, 0.f);
		}
	}
	else
	{
		AIC->ClearFocus(EAIFocusPriority::Gameplay);
	}
}

// ============================================================
// 록온 위젯 표시
// ============================================================

void AT3MidBossMonster::SetLockOnWidgetVisible(bool bVisible)
{
	if (LockOnWidgetComponent)
	{
		LockOnWidgetComponent->SetVisibility(bVisible);
	}
}
