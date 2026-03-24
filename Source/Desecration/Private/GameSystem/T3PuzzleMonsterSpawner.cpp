// T3PuzzleMonsterSpawner.cpp

#include "GameSystem/T3PuzzleMonsterSpawner.h"
#include "Desecration.h"

AT3PuzzleMonsterSpawner::AT3PuzzleMonsterSpawner()
{
	PrimaryActorTick.bCanEverTick = false;

	// 루트 컴포넌트 (에디터에서 위치 지정용)
	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);
}

void AT3PuzzleMonsterSpawner::RespawnMonster()
{
	// 기존 몬스터 제거
	DestroySpawnedMonster();

	if (!MonsterClass)
	{
		UE_LOG(LogDesecration, Warning, TEXT("T3_PuzzleSpawner: MonsterClass 미설정 — 스폰 스킵"));
		return;
	}

	// 스폰 위치 계산 (이 액터 위치 + 오프셋)
	const FVector SpawnLocation = GetActorLocation() + SpawnOffset;
	const FRotator SpawnRotation = GetActorRotation();

	UE_LOG(LogDesecration, Warning, TEXT("T3_PuzzleSpawner: 스포너 위치=%s, 오프셋=%s, 스폰 목표=%s, 회전=%s"),
		*GetActorLocation().ToString(), *SpawnOffset.ToString(), *SpawnLocation.ToString(), *SpawnRotation.ToString());

	// Deferred 스폰 — BeginPlay 전에 AutoPossessAI 설정 → Controller가 BeginPlay 전에 Possess
	FTransform SpawnTransform(SpawnRotation, SpawnLocation);
	SpawnedMonster = GetWorld()->SpawnActorDeferred<APawn>(
		MonsterClass, SpawnTransform, nullptr, nullptr,
		ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn);

	if (SpawnedMonster)
	{
		SpawnedMonster->AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

		// FinishSpawning → Controller 생성 + Possess + BeginPlay 실행
		// AIC_T3Monster BP의 OnPossess에서 CurrentMonsterBP 세팅 (Possess 후 호출되므로 GetPawn 유효)
		SpawnedMonster->FinishSpawning(SpawnTransform);

		UE_LOG(LogDesecration, Warning, TEXT("T3_PuzzleSpawner: %s 스폰 완료 — 실제 위치=%s"),
			*MonsterClass->GetName(), *SpawnedMonster->GetActorLocation().ToString());
	}
	else
	{
		UE_LOG(LogDesecration, Warning, TEXT("T3_PuzzleSpawner: %s 스폰 실패"), *MonsterClass->GetName());
	}
}

void AT3PuzzleMonsterSpawner::DestroySpawnedMonster()
{
	if (IsValid(SpawnedMonster))
	{
		// Controller 정리 — 루핑 타이머(SightScoreEvent, ForgetEvent)가 CurrentMonsterBP에 접근하므로
		// UnPossess/Destroy 전에 모든 타이머를 즉시 제거해야 None 접근 방지
		if (AController* Controller = SpawnedMonster->GetController())
		{
			GetWorld()->GetTimerManager().ClearAllTimersForObject(Controller);
			Controller->UnPossess();
			Controller->Destroy();
		}

		// Attach된 액터(무기 등) 먼저 제거 — BPI 사망 로직을 안 타도 정리됨
		TArray<AActor*> AttachedActors;
		SpawnedMonster->GetAttachedActors(AttachedActors);
		for (AActor* Attached : AttachedActors)
		{
			if (IsValid(Attached))
			{
				Attached->Destroy();
			}
		}

		SpawnedMonster->Destroy();
		SpawnedMonster = nullptr;

		UE_LOG(LogDesecration, Log, TEXT("T3_PuzzleSpawner: 기존 몬스터 제거 (Attached 포함)"));
	}
}
