#pragma once

#include "CoreMinimal.h"
#include "T3SaveGameParent.h"
#include "T3SaveObjectState.generated.h"

UCLASS()
class DESECRATION_API UT3SaveObjectState : public UT3SaveGameParent
{
	GENERATED_BODY()
	
public:
	virtual void ResetGameData() override;
	
	//모든 레벨의 물체 상태
	UPROPERTY()
	TMap<int32, int32> LevelObjectStates;
};
