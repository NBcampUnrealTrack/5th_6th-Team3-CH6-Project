// T3MidBossSpawner.cpp — 중간보스 스포너 구현

#include "Monster/T3MidBossSpawner.h"
#include "Monster/T3MidBossMonster.h"
#include "Desecration.h"

#include "Components/ArrowComponent.h"
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
}

AT3MidBossMonster* AT3MidBossSpawner::SpawnAndPrepareBoss()
{
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

	// 활성화 트리거 연결 (BeginPlay 이후이므로 BindExternalTrigger 사용)
	if (ActivationTrigger)
	{
		SpawnedBoss->BindExternalTrigger(ActivationTrigger);
	}

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
	UE_LOG(LogDesecration, Log, TEXT("T3_MidBossSpawner: [%s] 보스 사망 감지 → OnBossDied 브로드캐스트"),
		*GetName());

	OnBossDied.Broadcast();
}
