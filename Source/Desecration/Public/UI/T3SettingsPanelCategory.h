#pragma once

#include "CoreMinimal.h"
#include "T3SettingsPanel.h"
#include "Blueprint/UserWidget.h"
#include "GameSystem/T3GameInstance.h"
#include "T3SettingsPanelCategory.generated.h"

UCLASS(Abstract)
class DESECRATION_API UT3SettingsPanelCategory : public UUserWidget
{
	GENERATED_BODY()
	
	friend UT3SettingsPanel;
	
protected:
	//부모인 설정 패널에 의해 시작하는 NativeOnInitialized
	virtual void CustomNativeOnInitialized() PURE_VIRTUAL(UT3SettingsPanelCategory::CustomNativeOnInitialized, );
	
	//설정 화면을 열 때마다 실행하는 초기화
	virtual void InitializeSettingsPanel() PURE_VIRTUAL(UT3SettingsPanelCategory::InitializeSettingsPanel, );
	
	//언어 변경에 의한 초기화
	virtual void ReinitializeByChangeLanguage() { }

	//설정 저장하기
	virtual void SaveSettings() { }
	
	//게임 인스턴스
	UPROPERTY()
	TObjectPtr<UT3GameInstance> T3GameInstance;
	
	//부모 위젯인 세팅 패널
	UPROPERTY()
	TSoftObjectPtr<UT3SettingsPanel> SettingsPanel;
	
	//번역 기능에 사용할 네임스페이스 이름
	const FString NAMESPACE_NAME = TEXT("ST_Settings");
};
