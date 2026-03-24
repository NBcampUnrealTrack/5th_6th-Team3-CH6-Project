// T3PuzzleMonsterSpawner.h
// 퍼즐 단계 전환 시 몬스터 Destroy → Spawn 관리

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "T3PuzzleMonsterSpawner.generated.h"

// ============================================================
// AT3PuzzleMonsterSpawner
// ============================================================

UCLASS()
class DESECRATION_API AT3PuzzleMonsterSpawner : public AActor
{
	GENERATED_BODY()

public:
	AT3PuzzleMonsterSpawner();

	// ============================================================
	// 스폰 설정 (에디터에서 지정)
	// ============================================================

	// 스폰할 몬스터 클래스
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner")
	TSubclassOf<APawn> MonsterClass;

	// 스폰 오프셋 (이 액터 위치 기준)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner")
	FVector SpawnOffset = FVector::ZeroVector;

	// ============================================================
	// 인터페이스 (퍼즐에서 호출)
	// ============================================================

	// 기존 몬스터 파괴 후 새로 스폰
	UFUNCTION(BlueprintCallable, Category = "Spawner")
	void RespawnMonster();

	// 스폰된 몬스터 파괴
	UFUNCTION(BlueprintCallable, Category = "Spawner")
	void DestroySpawnedMonster();

private:
	// 현재 스폰된 몬스터
	UPROPERTY()
	TObjectPtr<APawn> SpawnedMonster;
};
