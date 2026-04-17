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
// Gameplay Tag 네이티브 정의 — StateTree 전용 이벤트 태그 (대응 State 없음)
UE_DEFINE_GAMEPLAY_TAG(TAG_Boss_Event_StunRecovered, "Boss.Event.StunRecovered");
UE_DEFINE_GAMEPLAY_TAG(TAG_Boss_Event_ActionCountDepleted, "Boss.Event.ActionCountDepleted");
UE_DEFINE_GAMEPLAY_TAG(TAG_Boss_Event_ParriedByPlayer, "Boss.Event.ParriedByPlayer");

AT3MidBossMonster::AT3MidBossMonster()
{
	PrimaryActorTick.bCanEverTick = true;

	// 회전 설정 — 컨트롤러 회전 직접 적용 비활성화, MovementComponent가 부드럽게 보간
	bUseControllerRotationYaw = false;
	bUseControllerRotationPitch = false;
	bUseControllerRotationRoll = false;

	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		MoveComp->bOrientRotationToMovement = false;       // 이동 방향 회전 OFF (스트레이프 시 타겟 바라보기 위해)
		MoveComp->bUseControllerDesiredRotation = true;    // 컨트롤러 SetFocus 방향으로 부드럽게 보간
		MoveComp->RotationRate = FRotator(0.f, 360.f, 0.f); // 초당 360도 (에디터에서 조절 가능)
	}

	// StateTree 컴포넌트 (standalone 스키마 — AI Controller 없이 동작)
	StateTreeComponent = CreateDefaultSubobject<UStateTreeComponent>(TEXT("StateTreeComponent"));
	StateTreeComponent->SetStartLogicAutomatically(false);

	// MotionWarping 컴포넌트
	MotionWarpingComponent = CreateDefaultSubobject<UMotionWarpingComponent>(TEXT("MotionWarpingComponent"));

	// 무기 컴포넌트
	WeaponComponent = CreateDefaultSubobject<UT3BossWeaponComponent>(TEXT("WeaponComponent"));

	// 디졸브 타임라인 컴포넌트
	DissolveTimeline = CreateDefaultSubobject<UTimelineComponent>(TEXT("DissolveTimeline"));

	// 록온 위젯 컴포넌트
	LockOnWidgetComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("LockOnWidget"));
	LockOnWidgetComponent->SetupAttachment(GetMesh(), TEXT("LockOn_Socket"));
	LockOnWidgetComponent->SetWidgetSpace(EWidgetSpace::Screen);
	LockOnWidgetComponent->SetDrawSize(FVector2D(30.f, 15.f));
	LockOnWidgetComponent->SetVisibility(false);
	LockOnWidgetComponent->SetRelativeLocation(FVector::ZeroVector);

	// 활성화 트리거: 외부 액터 레퍼런스 방식 (ExternalActivationTrigger를 에디터에서 지정)
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

	// 록온 위젯 소켓 재부착 (SetupAttachment는 런타임 보장 안 됨)
	if (LockOnWidgetComponent && GetMesh())
	{
		LockOnWidgetComponent->AttachToComponent(
			GetMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, TEXT("LockOn_Socket"));
		LockOnWidgetComponent->SetRelativeLocation(FVector::ZeroVector);

		UE_LOG(LogDesecration, Log, TEXT("T3_MidBoss: LockOnWidget 소켓 부착 완료 (%s)"),
			*LockOnWidgetComponent->GetAttachSocketName().ToString());
	}

	// 무기 소켓 부착 + 히트 델리게이트 바인딩
	if (WeaponComponent)
	{
		WeaponComponent->AttachToSocket(GetMesh());
		WeaponComponent->OnWeaponHitActor.AddDynamic(this, &AT3MidBossMonster::OnWeaponHit);
	}

	MidBossStats.CurrentHP = MidBossStats.MaxHP;
	MidBossStats.CurrentStunGauge = 0.f;

	// 초기 상태 태그 설정 (기존 bSuperArmor = true 대체)
	AddStateTag(TAG_Boss_State_SuperArmor);

	// 노티파이 보정기 생성 및 초기화
	NotifyModifier = NewObject<UMidBossNotifyModifier>(this);
	NotifyModifier->Initialize(ModifierDataAsset);

	// 디졸브용 Dynamic Material 생성 + Timeline 셋업
	if (bEnableDissolve)
	{
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

	// 외부 활성화 트리거 바인딩 (에디터 직접 배치 시)
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

void AT3MidBossMonster::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// MoveToTarget 보간 처리
	if (bIsMovingToTarget)
	{
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

	// AIController SetFocus — MovementComponent가 RotationRate로 부드럽게 보간
	if (AAIController* AIC = Cast<AAIController>(GetController()))
	{
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
