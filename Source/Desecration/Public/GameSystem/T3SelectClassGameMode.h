#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "T3SelectClassGameMode.generated.h"

class UT3GameInstance;

UCLASS()
class DESECRATION_API AT3SelectClassGameMode : public AGameModeBase
{
	GENERATED_BODY()
	
protected:
	virtual void BeginPlay() override;
	
public:
	//시작 데이터 생성하기 (true : 생성 성공)
	void MakeFirstGameData(const FText& PlayerName, const EPlayerClass SelectedPlayerClass);
	
	//튜토리얼 시작
	void TutorialStart();
	
	//타이틀 레벨로 돌아가기
	void ReturnToTitleLevel();
	
private:
	//게임 인스턴스
	UPROPERTY()
	TObjectPtr<UT3GameInstance> T3GameInstance;
};
