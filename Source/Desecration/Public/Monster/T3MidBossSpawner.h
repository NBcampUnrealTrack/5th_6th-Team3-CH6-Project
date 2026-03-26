// T3MidBossSpawner.h — 중간보스 스포너 (스테이지별 스탯 주입 + 트리거 연결)

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Monster/T3MidBossTypes.h"
#include "T3MidBossSpawner.generated.h"

class AT3MidBossMonster;
class UArrowComponent;
class UStaticMeshComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMidBossSpawnerBossSpawned, AT3MidBossMonster*, SpawnedBoss);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnMidBossSpawnerBossDied);

/**
 * 중간보스 스포너
 * - 맵마다 하나씩 배치, BossStage/StatsOverride만 다르게 설정
 * - SpawnAndPrepareBoss() 호출 시 보스 스폰 → 스탯 주입 → 트리거 바인딩
 * - BossRoomLock(BP)이 이 스포너의 델리게이트에 바인딩하여 사망 이벤트 수신
 */
UCLASS()
class DESECRATION_API AT3MidBossSpawner : public AActor
{
	GENERATED_BODY()

public:
	AT3MidBossSpawner();

	// ============================================================
	// 스폰 실행
	// ============================================================

	/** 보스 스폰 + 스탯/스테이지 주입 + 트리거 연결 */
	UFUNCTION(BlueprintCallable, Category = "MidBossSpawner")
	AT3MidBossMonster* SpawnAndPrepareBoss();

	// ============================================================
	// 에디터 설정
	// ============================================================

	/** 스폰할 보스 클래스 (BP_MidBoss) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "MidBossSpawner|Config")
	TSubclassOf<AT3MidBossMonster> BossClass;

	/** 스폰 위치/방향 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "MidBossSpawner|Config")
	TObjectPtr<UArrowComponent> SpawnPoint;

	/** 에디터 프리뷰 메시 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "MidBossSpawner|Config")
	TObjectPtr<UStaticMeshComponent> PreviewMesh;

	// ============================================================
	// 스테이지 & 스탯 주입
	// ============================================================

	/** 이 맵의 보스 스테이지 (패턴 해금 기준) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MidBossSpawner|Stage")
	int32 BossStage = 1;

	/** 스테이지별 스탯 오버라이드 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MidBossSpawner|Stage")
	FMidBossStats StatsOverride;

	// ============================================================
	// 외부 참조 (에디터 스포이드)
	// ============================================================

	/** 보스 활성화 트리거 (TriggerBox/TriggerVolume 등) */
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "MidBossSpawner|References")
	TObjectPtr<AActor> ActivationTrigger;

	// ============================================================
	// 델리게이트 (BossRoomLock 등 외부 시스템용)
	// ============================================================

	/** 보스 스폰 완료 시 (보스 레퍼런스 전달) */
	UPROPERTY(BlueprintAssignable, Category = "MidBossSpawner|Events")
	FOnMidBossSpawnerBossSpawned OnBossSpawned;

	/** 보스 사망 중계 (보스의 OnMidBossDeath → 이 델리게이트로 전달) */
	UPROPERTY(BlueprintAssignable, Category = "MidBossSpawner|Events")
	FOnMidBossSpawnerBossDied OnBossDied;

	// ============================================================
	// 런타임
	// ============================================================

	/** 현재 스폰된 보스 */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "MidBossSpawner|Runtime")
	TObjectPtr<AT3MidBossMonster> SpawnedBoss;

private:
	/** 보스 사망 시 중계 핸들러 */
	UFUNCTION()
	void HandleBossDeath();
};
