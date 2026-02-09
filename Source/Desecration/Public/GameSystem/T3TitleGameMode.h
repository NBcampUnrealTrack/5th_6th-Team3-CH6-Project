#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "T3TitleGameMode.generated.h"

class UT3GameInstance;

UCLASS()
class DESECRATION_API AT3TitleGameMode : public AGameModeBase
{
	GENERATED_BODY()
	
protected:
	virtual void BeginPlay() override;
	
public:
	//클래스 선택 화면으로
	void MoveToSelectClassLevel();
	
	//마지막 저장 장소에서 계속하기 (저장된 게임 데이터 적용)
	void MoveToLastSavedLevel();
	
	//게임 종료
	void QuitGame();
	
private:
	//게임 인스턴스
	UPROPERTY()
	TObjectPtr<UT3GameInstance> T3GameInstance;
};
