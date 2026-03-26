#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "T3SaveGameParent.generated.h"

UCLASS(Abstract)
class DESECRATION_API UT3SaveGameParent : public USaveGame
{
	GENERATED_BODY()
	
public:
	//사용하는 모든 값을 초기화
	virtual void ResetGameData() PURE_VIRTUAL(UT3SaveGameParent::ResetGameData, );
};
