#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "T3SelectClassGameMode.generated.h"

UCLASS()
class DESECRATION_API AT3SelectClassGameMode : public AGameModeBase
{
	GENERATED_BODY()
	
public:
	//시작 데이터 생성하기 (true : 생성 성공)
	bool MakeFirstGameData();
	
	//튜토리얼 시작
	void TutorialStart();
	
	//타이틀 레벨로 돌아가기
	void ReturnToTitleLevel();
};
