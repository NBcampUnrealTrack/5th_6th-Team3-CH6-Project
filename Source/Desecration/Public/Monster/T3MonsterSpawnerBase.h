// T3MonsterSpawnerBase.h


#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/ArrowComponent.h"
#include "T3MonsterSpawnerBase.generated.h"

class AT3MonsterBase;
class UStaticMeshComponent;

UCLASS()
class DESECRATION_API AT3MonsterSpawnerBase : public AActor
{
    GENERATED_BODY()

public:
    AT3MonsterSpawnerBase();

public:
    /** 몬스터를 스폰 (이미 있으면 제거 후 다시 생성) */
    UFUNCTION(BlueprintCallable, Category = "Spawner")
    void SpawnMonster();

protected:
    /** 스폰할 몬스터 클래스 */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Spawner")
    TSubclassOf<AT3MonsterBase> MonsterClass;

    /** 현재 스폰된 몬스터 */
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Spawner")
    AT3MonsterBase* CurrentMonster;

    /** 순찰 경로 (필요한 몬스터만 사용) */
    UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Spawner")
    AActor* PatrolRouteRef;

    /** 석상 트리거 (필요한 몬스터만 사용) */
    UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Spawner")
    AActor* StatueTriggerRef;

    /** 스폰 위치와 방향을 시각적으로 표시하는 에디터용 컴포넌트 */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Spawner")
    UArrowComponent* SpawnPoint;

    /** 화살표만 덩그러니 있으니 밋밋해서 스태틱 메시를 추가해서 에디터에만 보이게 함 */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Spawner")
    UStaticMeshComponent* PreviewMesh;
};