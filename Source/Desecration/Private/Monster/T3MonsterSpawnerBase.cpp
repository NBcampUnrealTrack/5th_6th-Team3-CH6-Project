// T3MonsterSpawnerBase.cpp


#include "Monster/T3MonsterSpawnerBase.h"
#include "Monster/T3MonsterBase.h"

#include "Engine/World.h"


AT3MonsterSpawnerBase::AT3MonsterSpawnerBase()
{
	PrimaryActorTick.bCanEverTick = false;

	SpawnPoint = CreateDefaultSubobject<UArrowComponent>(TEXT("SpawnPoint"));
	SpawnPoint->SetupAttachment(RootComponent);
	SpawnPoint->SetRelativeLocation(FVector(0.0f, 0.0f, 90.0f)); // 지면에서 약간 띄워서 스폰
	SpawnPoint->ArrowSize = 1.5f;
	SpawnPoint->bHiddenInGame = true;

	PreviewMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PreviewMesh"));
	PreviewMesh->SetupAttachment(SpawnPoint);
	PreviewMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	PreviewMesh->bHiddenInGame = true;

	CurrentMonster = nullptr;
}

void AT3MonsterSpawnerBase::SpawnMonster()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}
	if (!MonsterClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("[%s] MonsterClass is not set."), *GetName());
		return;
	}

	if (IsValid(CurrentMonster))
	{
		// 몬스터 제거 전에 정리 작업(무기 액터 제거 등)
		CurrentMonster->CleanupBeforeDestroy();

		// 몬스터 제거
		CurrentMonster->Destroy();
		CurrentMonster = nullptr;
	}

	const FTransform SpawnTransform = SpawnPoint->GetComponentTransform();

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	AT3MonsterBase* SpawnedMonster = World->SpawnActor<AT3MonsterBase>(MonsterClass, SpawnTransform, SpawnParams);

	if (!IsValid(SpawnedMonster))
	{
		UE_LOG(LogTemp, Warning, TEXT("[%s] Failed to spawn monster."), *GetName());
		return;
	}

	CurrentMonster = SpawnedMonster;
	CurrentMonster->ReceivePatrolRoute(PatrolRouteRef);
	CurrentMonster->ReceiveStatueTrigger(StatueTriggerRef);
	CurrentMonster->CurrentStage = CurrentStage;
	CurrentMonster->UpdateByStage();
}

