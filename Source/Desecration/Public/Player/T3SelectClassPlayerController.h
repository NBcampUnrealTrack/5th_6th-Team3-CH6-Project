#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "T3SelectClassPlayerController.generated.h"

class UT3SelectClassWidget;
class AT3SelectClassGameMode;
class UT3GameInstance;
enum class EPlayerClass;

UCLASS()
class DESECRATION_API AT3SelectClassPlayerController : public APlayerController
{
	GENERATED_BODY()
	
protected:
	virtual void BeginPlay() override;
	
public:
	//튜토리얼 시작
	void TutorialStart(const FString& PlayerName, const EPlayerClass SelectedPlayerClass);
	
	//타이틀 화면으로
	void ReturnToTitleLevel();
	
private:	
	//게임 모드
	UPROPERTY()
	TObjectPtr<AT3SelectClassGameMode> SelectClassGameMode;
	
	//클래스 선택 화면의 위젯
	UPROPERTY(EditDefaultsOnly, Category = "Widget", meta = (AllowPrivateAccess = true))
	TSubclassOf<UT3SelectClassWidget> SelectClassWidgetClass;
	
	UPROPERTY()
	TObjectPtr<UT3SelectClassWidget> SelectClassWidgetInstance;
};
