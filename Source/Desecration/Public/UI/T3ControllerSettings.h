#pragma once

#include "CoreMinimal.h"
#include "T3SettingsPanelCategory.h"
#include "T3ControllerSettings.generated.h"

class UT3Slider;
class UImage;
class UCheckBox;

UCLASS()
class DESECRATION_API UT3ControllerSettings : public UT3SettingsPanelCategory
{
	GENERATED_BODY()

protected:
	virtual void OnParentConstruct() override;
	virtual void InitializeSettingsPanel() override;
	virtual void SaveSettings() override;
	
private:
	//수직 회전 반전 사용 여부
	UFUNCTION()
	void OnCheckStateChangedInvertVerticalCheckBox(bool bIsChecked);
	
	//회전 감도
	UFUNCTION()
	void OnValueChangedCameraSpeedSlider(float Value);
	
	//수직 회전 반전 사용 여부
	UPROPERTY(meta = (AllowPrivateAccess = true, BindWidget))
	TObjectPtr<UCheckBox> InvertVerticalCheckBox;
	
	//회전 감도
	UPROPERTY(meta = (AllowPrivateAccess = true, BindWidget))
	TObjectPtr<UT3Slider> CameraSpeedSlider;

	//컨트롤러 선택에 사용할 번역 키 값
	const TArray<FString> CONTROLLER_KEY_STRINGS = { TEXT("KeyboardAndMouse"), TEXT("Gamepad") };
};
