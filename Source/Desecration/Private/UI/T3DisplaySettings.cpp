#include "UI/T3DisplaySettings.h"

#include "Components/CheckBox.h"
#include "Components/ComboBoxString.h"
#include "GameFramework/GameUserSettings.h"

void UT3DisplaySettings::OnParentConstruct()
{	
	if (!GEngine)
	{
		UE_LOG(LogTemp, Error, TEXT("%s : GEngine이 null"), *GetNameSafe(this));
		return;
	}
	GameUserSettings = GEngine->GetGameUserSettings();
	if (!GameUserSettings)
	{
		UE_LOG(LogTemp, Error, TEXT("%s : GEngine의 GameUserSettings가 null"), *GetNameSafe(this));
		return;
	}
	
	//해상도 콤보 박스
	for (const FResolution& Resolution : RESOLUTION_LIST)
	{
		ResolutionComboBox->AddOption(Resolution.Name);
	}
	ResolutionComboBox->OnSelectionChanged.AddDynamic(this, &ThisClass::OnSelectionChangedResolutionComboBox);
	
	//화면 모드 콤보 박스
	for (const FString& Key : SCREEN_MODE_KEY_STRINGS)
	{
		FString OptionString = UT3GameInstance::GetStringFromTable(NAMESPACE_NAME, Key);
		ScreenModeComboBox->AddOption(OptionString);
	}
	ScreenModeComboBox->OnSelectionChanged.AddDynamic(this, &ThisClass::OnSelectionChangedScreenModeComboBox);
	
	//그래픽 품질 콤보 박스
	for (const FString& Key : GRAPHIC_KEY_STRINGS)
	{
		FString OptionString = UT3GameInstance::GetStringFromTable(NAMESPACE_NAME, Key);
		GraphicQualityComboBox->AddOption(OptionString);
	}
	GraphicQualityComboBox->OnSelectionChanged.AddDynamic(this, &ThisClass::OnSelectionChangedGraphicQualityComboBox);
	
	//수직 동기화 체크 박스
	VSyncCheckBox->OnCheckStateChanged.AddDynamic(this, &ThisClass::OnCheckStateChangedVSyncCheckBox);
}

void UT3DisplaySettings::InitializeSettingsPanel()
{
	//해상도
	{
		const FIntPoint CurrentResolution = GameUserSettings->GetScreenResolution();
		const FString TargetOption = FString::Printf(TEXT("%d x %d"), CurrentResolution.X, CurrentResolution.Y);
		ResolutionComboBox->SetSelectedOption(TargetOption);
	}
	
	//화면 모드
	{
		const int32 TargetIndex = GameUserSettings->GetFullscreenMode();
		ScreenModeComboBox->SetSelectedIndex(TargetIndex);
	}
	
	//그래픽 품질
	{
		const int32 TargetIndex = GameUserSettings->GetOverallScalabilityLevel();
		GraphicQualityComboBox->SetSelectedIndex(TargetIndex);
	}
	
	//수직 동기화
	{
		const bool bVSync = GameUserSettings->IsVSyncEnabled();
		VSyncCheckBox->SetCheckedState(bVSync ? ECheckBoxState::Checked : ECheckBoxState::Unchecked);
	}
}

void UT3DisplaySettings::ReinitializeByChangeLanguage()
{
	//콤보 박스의 선택 기억 (해상도 콤보박스 제외)
	const int32 TempIndex1 = ScreenModeComboBox->GetSelectedIndex();
	const int32 TempIndex2 = GraphicQualityComboBox->GetSelectedIndex();
	//콤보 박스를 비우고 다시 채우기
	ScreenModeComboBox->ClearOptions();
	GraphicQualityComboBox->ClearOptions();
	for (const FString& Key : SCREEN_MODE_KEY_STRINGS)
	{
		FString OptionString = UT3GameInstance::GetStringFromTable(NAMESPACE_NAME, Key);
		ScreenModeComboBox->AddOption(OptionString);
	}
	for (const FString& Key : GRAPHIC_KEY_STRINGS)
	{
		FString OptionString = UT3GameInstance::GetStringFromTable(NAMESPACE_NAME, Key);
		GraphicQualityComboBox->AddOption(OptionString);
	}
	//다시 채운뒤 선택 원복
	ScreenModeComboBox->SetSelectedIndex(TempIndex1);
	GraphicQualityComboBox->SetSelectedIndex(TempIndex2);
}

void UT3DisplaySettings::OnSelectionChangedResolutionComboBox(FString SelectedItem, ESelectInfo::Type SelectionType)
{
	//코드에 의한 변경은 무시
	if (SelectionType == ESelectInfo::Type::Direct)
	{
		return;
	}
	
	//선택한 해상도를 적용
	const int32 SelectedIndex = ResolutionComboBox->GetSelectedIndex();
	const FIntPoint SelectedResolution = RESOLUTION_LIST[SelectedIndex].SizeInfo;
	SetResolution(SelectedResolution);
}

void UT3DisplaySettings::OnSelectionChangedScreenModeComboBox(FString SelectedItem, ESelectInfo::Type SelectionType)
{
	//코드에 의한 변경은 무시
	if (SelectionType == ESelectInfo::Type::Direct)
	{
		return;
	}
	
	const int32 SelectedIndex = ScreenModeComboBox->GetSelectedIndex();
	SetWindowMode(static_cast<EWindowMode::Type>(SelectedIndex));
}

void UT3DisplaySettings::OnSelectionChangedGraphicQualityComboBox(FString SelectedItem, ESelectInfo::Type SelectionType)
{
	//코드에 의한 변경은 무시
	if (SelectionType == ESelectInfo::Type::Direct)
	{
		return;
	}
	
	const int32 SelectedIndex = GraphicQualityComboBox->GetSelectedIndex();
	SetGraphicQuality(SelectedIndex);
}

void UT3DisplaySettings::OnCheckStateChangedVSyncCheckBox(bool bIsChecked)
{
	GameUserSettings->SetVSyncEnabled(bIsChecked);
	GameUserSettings->ApplySettings(true);
	
#if WITH_EDITOR
	//수직 동기화 여부를 에디터에서 파악할 수 있도록 로그로 표시
	UE_LOG(LogTemp, Warning, TEXT("수직 동기화 : %hhd"), bIsChecked);
#endif
}

void UT3DisplaySettings::SetResolution(const FInt32Point Resolution)
{
	GameUserSettings->SetScreenResolution(Resolution);
	GameUserSettings->ApplySettings(true);
}

void UT3DisplaySettings::SetWindowMode(const EWindowMode::Type WindowMode)
{
	GameUserSettings->SetFullscreenMode(WindowMode);
	GameUserSettings->ApplySettings(true);
	
#if WITH_EDITOR
	//화면 모드 변경은 에디터에서 알 수 없기 때문에 로그로 표시
	const FString LogOutValue = UEnum::GetValueAsString(WindowMode);
	UE_LOG(LogTemp, Warning, TEXT("화면 모드 변경 : %s"), *LogOutValue);
#endif
}

void UT3DisplaySettings::SetGraphicQuality(const int32 GraphicQuality)
{
	GameUserSettings->SetOverallScalabilityLevel(GraphicQuality);
	GameUserSettings->ApplySettings(true);
}
