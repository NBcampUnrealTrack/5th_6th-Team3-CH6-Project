#include "Player/T3TitlePlayerController.h"

#include "UI/T3SettingsPanel.h"
#include "UI/T3TitleLevelWidget.h"

void AT3TitlePlayerController::BeginPlay()
{
	Super::BeginPlay();
	
	//위젯 생성
	TitleLevelWidgetInstance = CreateWidget<UT3TitleLevelWidget>(this, TitleLevelWidgetClass);
	if (!TitleLevelWidgetInstance)
	{
		UE_LOG(LogTemp, Error, TEXT("%s : Failed to create TitleLevelWidget"), *GetNameSafe(this));
		return;
	}
	
	SettingsPanelInstance = CreateWidget<UT3SettingsPanel>(this, SettingsPanelClass);
	if (!SettingsPanelInstance)
	{
		UE_LOG(LogTemp, Error, TEXT("%s : Failed to create SettingsPanel"), *GetNameSafe(this));
		return;
	}
	
	//설정 패널은 숨김 상태로 시작
	SettingsPanelInstance->SetVisibility(ESlateVisibility::Collapsed);
}

void AT3TitlePlayerController::SetActiveSettingsPanel(bool bActive)
{
	//타이틀 레벨 위젯과 설정 패널 위젯은 서로 반대로 적용
	TitleLevelWidgetInstance->SetVisibility(bActive ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
	SettingsPanelInstance->SetVisibility(bActive ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	
	//설정 패널 활성화시 패널 내용 초기화
	//TODO : 패널 내용 초기화
}
