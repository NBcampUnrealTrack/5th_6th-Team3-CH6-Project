#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "T3TitlePlayerController.generated.h"

class UT3GameInstance;
class AT3TitleGameMode;
class UT3SettingsPanel;
class UT3TitleLevelWidget;
class UT3SaveGame;

UCLASS()
class DESECRATION_API AT3TitlePlayerController : public APlayerController
{
	GENERATED_BODY()
	
protected:
	virtual void BeginPlay() override;
	
public:
	//클래스 선택 화면으로
	UFUNCTION()
	void MoveToSelectClassLevel();
	
	//마지막 저장 장소에서 계속하기 (저장된 게임 데이터 적용)
	UFUNCTION()
	void MoveToLastSavedLevel();
	
	//저장된 게임 데이터 존재 여부
	bool DoesSavedDataExist();
	
	//세팅 패널 띄움 여부, 타이틀 화면 위젯과 동시에 반대로 적용됨
	void SetActiveSettingsPanel(bool bActive);
	
	//게임 종료
	UFUNCTION()
	void QuitGame();
	
private:
	//타이틀 화면의 위젯 클래스
	UPROPERTY(EditDefaultsOnly, Category = "Widget", meta = (AllowPrivateAccess = true))
	TSubclassOf<UT3TitleLevelWidget> TitleLevelWidgetClass;
	
	//타이틀 화면의 위젯
	UPROPERTY()
	TObjectPtr<UT3TitleLevelWidget> TitleLevelWidgetInstance;
	
	//설정 패널 위젯
	UPROPERTY(EditDefaultsOnly, Category = "Widget", meta = (AllowPrivateAccess = true))
	TSubclassOf<UT3SettingsPanel> SettingsPanelClass;
	
	//설정 패널 위젯 클래스
	UPROPERTY()
	TObjectPtr<UT3SettingsPanel> SettingsPanelInstance;
	
	//게임 모드
	UPROPERTY()
	TObjectPtr<AT3TitleGameMode> TitleGameMode;
	
	//게임 인스턴스
	UPROPERTY()
	TObjectPtr<UT3GameInstance> T3GameInstance;
};
