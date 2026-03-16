#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "GameSystem/T3SaveLostMoney.h"
#include "T3WorldSubsystem.generated.h"

class UT3SaveGame;

UCLASS()
class DESECRATION_API UT3WorldSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()
	
public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	/**
	 * 지정한 물체의 상태
	 * @param ObjectID 물체 번호
	 * @return 그 물체의 상태를 반환, 없다면 0을 반환
	 */
	UFUNCTION(BlueprintPure, Category = "Object State")
	int32 GetObjectState(const int32 ObjectID) const;

	/**
	 * 지정한 물체를 추가하거나 그 상태를 변경
	 * @param ObjectID 추가 혹은 변경할 물체 번호
	 * @param NewState 물체의 상태
	 */
	UFUNCTION(BlueprintCallable, Category = "Object State")
	void SetOrAddObjectState(const int32 ObjectID, const int32 NewState) const;

	/**
	 * 지정한 적의 상태
	 * @param EnemyID 적 번호 
	 * @return 그 적의 상태를 반환, 없다면 0을 반환
	 */
	UFUNCTION(BlueprintCallable, Category = "Object State")
	int32 GetEnemyState(const int32 EnemyID) const;

	/**
	 * 지정한 적을 추가하거나 그 상태를 변경
	 * @param EnemyID 추가 혹은 변경할 적 번호
	 * @param NewState 적 상태
	 */
	UFUNCTION(BlueprintCallable, Category = "Object State")
	void SetOrAddEnemyState(const int32 EnemyID, const int32 NewState) const;
	
	//현재 레벨에서 잃어버린 재화 목록
	UFUNCTION(BlueprintPure, Category = "Lost Money")
	TMap<int32, FLostMoney> GetAllLostMoney();
	
private:
	//몬스터, 물체 상태 확인을 위한 저장된 게임 참조
	UPROPERTY()
	TSoftObjectPtr<UT3SaveGame> T3SaveGame;
	
	//이 레벨에서 잃어버린 재화
	TMap<int32, FLostMoney> LostMoneyList;
};
