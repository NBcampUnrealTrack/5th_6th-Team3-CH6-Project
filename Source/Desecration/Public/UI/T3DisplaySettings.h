#pragma once

#include "CoreMinimal.h"
#include "T3SettingsPanelCategory.h"
#include "T3DisplaySettings.generated.h"

UCLASS()
class DESECRATION_API UT3DisplaySettings : public UT3SettingsPanelCategory
{
	GENERATED_BODY()
	
public:
	virtual void NativeConstruct() override;
	
private:
	//해상도 설정
	UPROPERTY(meta = (AllowPrivateAccess = true, BindWidget))
	TObjectPtr<UComboBoxString> ResolutionComboBox;
	
	//화면 모드 설정
	UPROPERTY(meta = (AllowPrivateAccess = true, BindWidget))
	TObjectPtr<UComboBoxString> ScreenModeComboBox;
	
	//그래픽 품질 설정
	UPROPERTY(meta = (AllowPrivateAccess = true, BindWidget))
	TObjectPtr<UComboBoxString> GraphicQualityComboBox;
};
