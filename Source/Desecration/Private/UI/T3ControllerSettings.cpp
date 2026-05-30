#include "UI/T3ControllerSettings.h"

#include "Components/CheckBox.h"
#include "Components/ComboBoxString.h"
#include "Components/Image.h"
#include "GameSystem/T3SaveUserSettings.h"
#include "UI/T3Slider.h"

void UT3ControllerSettings::OnParentConstruct()
{
	//수직 회전 반전
	InvertVerticalCheckBox->OnCheckStateChanged.AddDynamic(this, &UT3ControllerSettings::OnCheckStateChangedInvertVerticalCheckBox);
	
	//회전 속도
	CameraSpeedSlider->GetOnValueChangedEvent().AddDynamic(this, &UT3ControllerSettings::OnValueChangedCameraSpeedSlider);
}

void UT3ControllerSettings::InitializeSettingsPanel()
{
	const TObjectPtr<UT3SaveUserSettings> CurrentSettings = T3GameInstance->GetCurrentSettings();
	InvertVerticalCheckBox->SetCheckedState(CurrentSettings->bInvertVertical ? ECheckBoxState::Checked : ECheckBoxState::Unchecked);
	CameraSpeedSlider->SetValue(CurrentSettings->CameraSpeed);
}

void UT3ControllerSettings::SaveSettings()
{
	T3GameInstance->SaveUserSettings();
}

void UT3ControllerSettings::OnCheckStateChangedInvertVerticalCheckBox(bool bIsChecked)
{
	T3GameInstance->GetCurrentSettings()->bInvertVertical = bIsChecked;
}

void UT3ControllerSettings::OnValueChangedCameraSpeedSlider(float Value)
{
	T3GameInstance->GetCurrentSettings()->CameraSpeed = Value;
}
