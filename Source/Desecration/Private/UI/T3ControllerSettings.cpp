#include "UI/T3ControllerSettings.h"

#include "Components/CheckBox.h"
#include "Components/ComboBoxString.h"
#include "Components/Image.h"
#include "GameSystem/T3SaveUserSettings.h"
#include "UI/T3Slider.h"

void UT3ControllerSettings::CustomNativeConstruct()
{	
	//컨트롤러 선택 콤보 박스
	for (const FString Key : CONTROLLER_KEY_STRINGS)
	{		
		const FString OptionString = UT3GameInstance::GetStringFromTable(NAMESPACE_NAME, Key);
		ControllerComboBox->AddOption(OptionString);
	}
	ControllerComboBox->OnSelectionChanged.AddDynamic(this, &UT3ControllerSettings::OnSelectionChangedControllerComboBox);
	
	//수직 회전 반전
	InvertVerticalCheckBox->OnCheckStateChanged.AddDynamic(this, &UT3ControllerSettings::OnCheckStateChangedInvertVerticalCheckBox);
	
	//회전 속도
	CameraSpeedSlider->GetOnValueChangedEvent().AddDynamic(this, &UT3ControllerSettings::OnValueChangedCameraSpeedSlider);
}

void UT3ControllerSettings::InitializeSettingsPanel()
{
	const TObjectPtr<UT3SaveUserSettings> CurrentSettings = T3GameInstance->GetCurrentSettings();
	if (LayoutTextures.IsValidIndex(CurrentSettings->UsingController))
	{
		ControlGuideImage->SetBrushFromTexture(LayoutTextures[CurrentSettings->UsingController]);
	}
	ControllerComboBox->SetSelectedIndex(CurrentSettings->UsingController);
	InvertVerticalCheckBox->SetCheckedState(CurrentSettings->bInvertVertical ? ECheckBoxState::Checked : ECheckBoxState::Unchecked);
	CameraSpeedSlider->SetValue(CurrentSettings->CameraSpeed);
}

void UT3ControllerSettings::ReinitializeByChangeLanguage()
{
	//콤보 박스 리셋
	const int32 TempIndex = ControllerComboBox->GetSelectedIndex();
	ControllerComboBox->ClearOptions();
	for (const FString Key : CONTROLLER_KEY_STRINGS)
	{		
		const FString OptionString = UT3GameInstance::GetStringFromTable(NAMESPACE_NAME, Key);
		ControllerComboBox->AddOption(OptionString);
	}
	ControllerComboBox->SetSelectedIndex(TempIndex);
}

void UT3ControllerSettings::SaveSettings()
{
	T3GameInstance->SaveUserSettings();
}

void UT3ControllerSettings::OnSelectionChangedControllerComboBox(FString SelectedItem, ESelectInfo::Type SelectionType)
{
	//코드에 의한 변경은 무시
	if (SelectionType == ESelectInfo::Type::Direct)
	{
		return;
	}

	//컨트롤러 변경
	const int32 SelectedIndex = ControllerComboBox->GetSelectedIndex();
	T3GameInstance->GetCurrentSettings()->UsingController = SelectedIndex;
}

void UT3ControllerSettings::OnCheckStateChangedInvertVerticalCheckBox(bool bIsChecked)
{
	T3GameInstance->GetCurrentSettings()->bInvertVertical = bIsChecked;
}

void UT3ControllerSettings::OnValueChangedCameraSpeedSlider(float Value)
{
	T3GameInstance->GetCurrentSettings()->CameraSpeed = Value;
}
