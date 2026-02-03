#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "T3TitlePlayerController.generated.h"

class UT3SettingsPanel;
class UT3TitleLevelWidget;

UCLASS()
class DESECRATION_API AT3TitlePlayerController : public APlayerController
{
	GENERATED_BODY()
	
protected:
	virtual void BeginPlay() override;
	
public:
	//세팅 패널 띄움 여부, 타이틀 화면 위젯과 동시에 반대로 적용됨
	void SetActiveSettingsPanel(bool bActive);
	
private:
	//타이틀 화면의 위젯
	UPROPERTY(EditDefaultsOnly, Category = "Widget", meta = (AllowPrivateAccess = true))
	TSubclassOf<UT3TitleLevelWidget> TitleLevelWidgetClass;
	
	UPROPERTY()
	TObjectPtr<UT3TitleLevelWidget> TitleLevelWidgetInstance;
	
	//설정 패널 위젯
	UPROPERTY(EditDefaultsOnly, Category = "Widget", meta = (AllowPrivateAccess = true))
	TSubclassOf<UT3SettingsPanel> SettingsPanelClass;
	
	UPROPERTY()
	TObjectPtr<UT3SettingsPanel> SettingsPanelInstance;
};
