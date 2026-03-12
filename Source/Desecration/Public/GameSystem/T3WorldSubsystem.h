#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "GameSystem/T3SaveLostMoney.h"
#include "T3WorldSubsystem.generated.h"

class UT3GameInstance;

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
	int32 GetState(int32 ObjectID) const;

	/**
	 * 지정한 물체를 추가하거나 그 상태를 변경
	 * @param ObjectID 추가 혹은 변경할 물체
	 * @param NewState 물체의 상태
	 */
	UFUNCTION(BlueprintCallable, Category = "Object State")
	void SetOrAddState(int32 ObjectID, int32 NewState);
	
	//현재 등록된 모든 물체의 상태를 반환
	TMap<int32, int32> GetAllStates();
	
	//현재 레벨에서 잃어버린 재화 목록
	UFUNCTION(BlueprintPure, Category = "Lost Money")
	TMap<int32, FLostMoney> GetAllLostMoney();
	
private:
	//여러 물체들의 상태를 저장
	TMap<int32, int32> LevelObjectStates;
	
	//이 레벨에서 잃어버린 재화
	TMap<int32, FLostMoney> LostMoneyList;
};
