#pragma once

#include "CoreMinimal.h"
#include "T3SettingsPanelCategory.h"
#include "T3ControllerSettings.generated.h"

class UCheckBox;

UCLASS()
class DESECRATION_API UT3ControllerSettings : public UT3SettingsPanelCategory
{
	GENERATED_BODY()
	
public:
	virtual void NativeConstruct() override;
	
private:
	//게임에 사용할 컨트롤러를 선택하는 콤보 박스
	UPROPERTY(meta = (AllowPrivateAccess = true, BindWidget))
	TObjectPtr<UComboBoxString> ControllerComboBox;
	
	//수직 회전 반전 사용 여부
	UPROPERTY(meta = (AllowPrivateAccess = true, BindWidget))
	TObjectPtr<UCheckBox> VerticalReverseCheckBox;
	
	//회전 감도
	UPROPERTY(meta = (AllowPrivateAccess = true, BindWidget))
	TObjectPtr<USlider> CameraSpeedSlider;
};
