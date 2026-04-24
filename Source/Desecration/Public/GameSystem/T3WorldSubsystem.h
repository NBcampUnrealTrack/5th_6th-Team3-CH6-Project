#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "T3WorldSubsystem.generated.h"

class UT3SaveGame;
class UT3GameInstance;
class UT3SaveObjectState;

UCLASS()
class DESECRATION_API UT3WorldSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()
	
public:
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;

	/**
	 * 지정한 물체 또는 몬스터의 상태
	 * @param ObjectID 물체 번호
	 * @return 그 물체의 상태를 반환, 없다면 0을 반환
	 */
	UFUNCTION(BlueprintPure, Category = "Object State")
	int32 GetObjectState(const int32 ObjectID) const;

	/**
	 * 지정한 물체나 몬스터를 추가하거나 그 상태를 변경
	 * @param ObjectID 추가 혹은 변경할 물체 번호
	 * @param NewState 물체의 상태
	 */
	UFUNCTION(BlueprintCallable, Category = "Object State")
	void SetOrAddObjectState(const int32 ObjectID, const int32 NewState) const;
	
private:
	//몬스터, 물체 상태 확인을 위한 저장된 게임 참조
	UPROPERTY()
	TObjectPtr<UT3SaveGame> T3SaveGame;
	
	//게임 인스턴스
	UPROPERTY()
	TObjectPtr<UT3GameInstance> T3GameInstance;
};
