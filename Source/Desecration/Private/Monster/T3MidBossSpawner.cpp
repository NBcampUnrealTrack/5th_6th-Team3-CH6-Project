// T3MidBossSpawner.cpp — 중간보스 스포너 구현

#include "Monster/T3MidBossSpawner.h"
#include "Monster/T3MidBossMonster.h"
#include "GameSystem/T3WorldSubsystem.h"
#include "Desecration.h"

#include "Components/ArrowComponent.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"

AT3MidBossSpawner::AT3MidBossSpawner()
{
	PrimaryActorTick.bCanEverTick = false;

	// 루트 컴포넌트
	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	// 스폰 위치/방향 표시
	SpawnPoint = CreateDefaultSubobject<UArrowComponent>(TEXT("SpawnPoint"));
	SpawnPoint->SetupAttachment(Root);
	SpawnPoint->SetRelativeLocation(FVector(0.f, 0.f, 90.f));
	SpawnPoint->ArrowSize = 2.0f;
	SpawnPoint->ArrowColor = FColor::Red;
	SpawnPoint->bHiddenInGame = true;

	// 에디터 프리뷰 메시
	PreviewMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PreviewMesh"));
	PreviewMesh->SetupAttachment(SpawnPoint);
	PreviewMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	PreviewMesh->bHiddenInGame = true;

	// 내장 활성화 트리거 박스
	ActivationTriggerBox = CreateDefaultSubobject<UBoxComponent>(TEXT("ActivationTriggerBox"));
	ActivationTriggerBox->SetupAttachment(Root);
	ActivationTriggerBox->SetBoxExtent(FVector(300.f, 300.f, 150.f));
	ActivationTriggerBox->SetCollisionProfileName(TEXT("Trigger"));
	ActivationTriggerBox->SetHiddenInGame(true);
}

void AT3MidBossSpawner::BeginPlay()
{
	Super::BeginPlay();

	// ObjectID가 0이면 저장 기능 사용 안 함
	if (ObjectID == 0)
	{
		return;
	}

	// WorldSubsystem에서 저장된 상태 확인
	if (const UT3WorldSubsystem* WorldSubsystem = GetWorld()->GetSubsystem<UT3WorldSubsystem>())
	{
		const int32 SavedState = WorldSubsystem->GetObjectState(ObjectID);
		if (SavedState >= 1)
		{
			// 이미 처치된 보스 — 스폰 스킵, 즉시 사망 이벤트 브로드캐스트
			bBossAlreadyDefeated = true;

			UE_LOG(LogDesecration, Log,
				TEXT("T3_MidBossSpawner: [%s] ObjectID:%d 이미 처치됨 (State:%d) → 스폰 스킵, OnBossDied 브로드캐스트"),
				*GetName(), ObjectID, SavedState);

			OnBossDied.Broadcast();
		}
	}
}

AT3MidBossMonster* AT3MidBossSpawner::SpawnAndPrepareBoss()
{
	// 이미 처치된 보스면 스폰하지 않음
	if (bBossAlreadyDefeated)
	{
		UE_LOG(LogDesecration, Log, TEXT("T3_MidBossSpawner: [%s] 이미 처치된 보스 — 스폰 거부"), *GetName());
		return nullptr;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	if (!BossClass)
	{
		UE_LOG(LogDesecration, Warning, TEXT("T3_MidBossSpawner: [%s] BossClass 미설정"), *GetName());
		return nullptr;
	}

	// 기존 보스가 있으면 제거
	if (IsValid(SpawnedBoss))
	{
		UE_LOG(LogDesecration, Log, TEXT("T3_MidBossSpawner: 기존 보스 제거 후 재스폰"));
		SpawnedBoss->Destroy();
		SpawnedBoss = nullptr;
	}

	// 스폰
	const FTransform SpawnTransform = SpawnPoint->GetComponentTransform();
	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	SpawnedBoss = World->SpawnActor<AT3MidBossMonster>(BossClass, SpawnTransform, SpawnParams);
	if (!IsValid(SpawnedBoss))
	{
		UE_LOG(LogDesecration, Error, TEXT("T3_MidBossSpawner: [%s] 보스 스폰 실패"), *GetName());
		return nullptr;
	}

	// 스테이지 주입
	SpawnedBoss->BossStage = BossStage;

	// 스탯 주입 (CurrentHP를 MaxHP에 맞춤)
	StatsOverride.CurrentHP = StatsOverride.MaxHP;
	StatsOverride.CurrentStunGauge = 0.f;
	SpawnedBoss->MidBossStats = StatsOverride;

	// 활성화 트리거 연결 — 내장 BoxComponent → 스포너 자체를 트리거 액터로 전달
	SpawnedBoss->BindExternalTrigger(this);

	// 사망 델리게이트 중계 바인딩
	SpawnedBoss->OnMidBossDeath.AddDynamic(this, &AT3MidBossSpawner::HandleBossDeath);

	// 스폰 완료 브로드캐스트
	OnBossSpawned.Broadcast(SpawnedBoss);

	UE_LOG(LogDesecration, Log,
		TEXT("T3_MidBossSpawner: [%s] 보스 스폰 완료 — Stage:%d, HP:%.0f, ATK:%.0f"),
		*GetName(), BossStage, StatsOverride.MaxHP, StatsOverride.AttackPower);

	return SpawnedBoss;
}

void AT3MidBossSpawner::HandleBossDeath()
{
	UE_LOG(LogDesecration, Log, TEXT("T3_MidBossSpawner: [%s] 보스 사망 감지 → 상태 저장 + OnBossDied 브로드캐스트"),
		*GetName());

	SaveBossDefeatedState();
	OnBossDied.Broadcast();
}

void AT3MidBossSpawner::SaveBossDefeatedState()
{
	if (ObjectID == 0)
	{
		return;
	}

	if (UT3WorldSubsystem* WorldSubsystem = GetWorld()->GetSubsystem<UT3WorldSubsystem>())
	{
		WorldSubsystem->SetOrAddObjectState(ObjectID, 1);
		UE_LOG(LogDesecration, Log,
			TEXT("T3_MidBossSpawner: [%s] ObjectID:%d → State:1 (처치됨) 저장 완료"),
			*GetName(), ObjectID);
	}
}
