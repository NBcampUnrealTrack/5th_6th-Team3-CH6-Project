#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "GameSystem/GlobalEnums.h"
#include "T3SaveGame.generated.h"



UCLASS()
class DESECRATION_API UT3SaveGame : public USaveGame
{
	GENERATED_BODY()
	
public:
	//게임 데이터 초기화
	void ResetGameData();
	
	//플레이어의 클래스
	UPROPERTY()
	EPlayerClass PlayerClass;
	
	//저장한 곳의 맵 이름
	UPROPERTY()
	ELevelName SavedLevelName;
};
