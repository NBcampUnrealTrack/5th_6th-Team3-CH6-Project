#include "UI/T3SettingsPanel.h"

#include "Components/Button.h"
#include "Components/ComboBoxString.h"
#include "Components/Slider.h"
#include "GameSystem/T3GameInstance.h"
#include "GameSystem/T3TitleGameMode.h"
#include "Player/T3TitlePlayerController.h"

void UT3SettingsPanel::NativeConstruct()
{
	Super::NativeConstruct();
	
	//게임 모드
	TitleGameMode = Cast<AT3TitleGameMode>(GetWorld()->GetAuthGameMode());
	if (!TitleGameMode)
	{
		UE_LOG(LogTemp, Error, TEXT("%s : TitleGameMode is NULL"), *GetNameSafe(this));
		return;
	}
	
	//게임 인스턴스
	T3GameInstance = Cast<UT3GameInstance>(GetGameInstance());
	if (!TitleGameMode)
	{
		UE_LOG(LogTemp, Error, TEXT("%s : T3GameInstance is NULL"), *GetNameSafe(this));
		return;
	}
	
	//플레이어 컨트롤러
	TitlePlayerController = Cast<AT3TitlePlayerController>(GetOwningPlayer());
	if (!TitlePlayerController)
	{
		UE_LOG(LogTemp, Error, TEXT("%s : TitlePlayerController is NULL"), *GetNameSafe(this));
		return;
	}
	
	//슬라이더 바인딩
	BGMSlider->OnValueChanged.AddDynamic(this, &ThisClass::WhileMovingBGMSlider);
	SESlider->OnValueChanged.AddDynamic(this, &ThisClass::WhileMovingSESlider);
	MouseSensitivitySlider->OnValueChanged.AddDynamic(this, &ThisClass::WhileMovingMouseSensitivitySlider);
	
	//콤보박스 바인딩
	ResolutionComboBox->OnSelectionChanged.AddDynamic(this, &ThisClass::OnSelectResolutionComboBox);
	ScreenModeComboBox->OnSelectionChanged.AddDynamic(this, &ThisClass::OnSelectScreenModeComboBox);
	GraphicQualityComboBox->OnSelectionChanged.AddDynamic(this, &ThisClass::OnSelectGraphicQualityComboBox);
	
	//버튼 바인딩
	ResetButton->OnClicked.AddDynamic(this, &ThisClass::OnClickResetButton);
	ConfirmButton->OnClicked.AddDynamic(this, &ThisClass::OnClickConfirmButton);
}

void UT3SettingsPanel::WhileMovingBGMSlider(float value)
{
	
}

void UT3SettingsPanel::WhileMovingSESlider(float value)
{
	
}

void UT3SettingsPanel::WhileMovingMouseSensitivitySlider(float value)
{
	
}

void UT3SettingsPanel::OnSelectResolutionComboBox(FString SelectedItem, ESelectInfo::Type SelectionType)
{
	
}

void UT3SettingsPanel::OnSelectScreenModeComboBox(FString SelectedItem, ESelectInfo::Type SelectionType)
{
	
}

void UT3SettingsPanel::OnSelectGraphicQualityComboBox(FString SelectedItem, ESelectInfo::Type SelectionType)
{
	
}

void UT3SettingsPanel::OnClickResetButton()
{
	
}

void UT3SettingsPanel::OnClickConfirmButton()
{
	
}
