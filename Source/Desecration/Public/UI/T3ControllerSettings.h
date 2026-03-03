#pragma once

#include "CoreMinimal.h"
#include "T3SettingsPanelCategory.h"
#include "T3ControllerSettings.generated.h"

class UCheckBox;

UCLASS()
class DESECRATION_API UT3ControllerSettings : public UT3SettingsPanelCategory
{
	GENERATED_BODY()

protected:
	virtual void CustomNativeConstruct() override;
	virtual void InitializeSettingsPanel() override;
	virtual void SaveSettings() override;
	
private:
	//컨트롤러 선택 콤보 박스
	UFUNCTION()
	void OnSelectionChangedControllerComboBox(FString SelectedItem, ESelectInfo::Type SelectionType);
	
	//수직 회전 반전 사용 여부
	UFUNCTION()
	void OnCheckStateChangedInvertVerticalCheckBox(bool bIsChecked);
	
	//회전 감도
	UFUNCTION()
	void OnValueChangedCameraSpeedSlider(float Value);
	
	//게임에 사용할 컨트롤러를 선택하는 콤보 박스
	UPROPERTY(meta = (AllowPrivateAccess = true, BindWidget))
	TObjectPtr<UComboBoxString> ControllerComboBox;
	
	//수직 회전 반전 사용 여부
	UPROPERTY(meta = (AllowPrivateAccess = true, BindWidget))
	TObjectPtr<UCheckBox> InvertVerticalCheckBox;
	
	//회전 감도
	UPROPERTY(meta = (AllowPrivateAccess = true, BindWidget))
	TObjectPtr<USlider> CameraSpeedSlider;

	//컨트롤러 선택에 사용할 번역 키 값
	const TArray<FString> CONTROLLER_KEY_STRINGS = { TEXT("KeyboardAndMouse"), TEXT("Gamepad") };
};
