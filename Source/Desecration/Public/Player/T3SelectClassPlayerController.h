#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "T3SelectClassPlayerController.generated.h"

class UT3SelectClassWidget;
class AT3SelectClassGameMode;
class UT3GameInstance;

UCLASS()
class DESECRATION_API AT3SelectClassPlayerController : public APlayerController
{
	GENERATED_BODY()
	
protected:
	virtual void BeginPlay() override;
	
public:
	void TutorialStart(const FText& PlayerName);
	
	//타이틀 화면으로
	void ReturnToTitleLevel();
	
private:
	//게임 인스턴스
	UPROPERTY()
	TObjectPtr<UT3GameInstance> T3GameInstance;
	
	//게임 모드
	UPROPERTY()
	TObjectPtr<AT3SelectClassGameMode> SelectClassGameMode;
	
	//클래스 선택 화면의 위젯
	UPROPERTY(EditDefaultsOnly, Category = "Widget", meta = (AllowPrivateAccess = true))
	TSubclassOf<UT3SelectClassWidget> SelectClassWidgetClass;
	
	UPROPERTY()
	TObjectPtr<UT3SelectClassWidget> SelectClassWidgetInstance;
};
