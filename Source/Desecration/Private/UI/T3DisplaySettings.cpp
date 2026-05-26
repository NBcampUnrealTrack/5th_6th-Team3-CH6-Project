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
	
	//그래픽 품질 콤보 박스
	for (const FString& Key : GRAPHIC_KEY_STRINGS)
	{
		FString OptionString = UT3GameInstance::GetStringFromTable(NAMESPACE_NAME, Key);
		GraphicQualityComboBox->AddOption(OptionString);
	}
	GraphicQualityComboBox->OnSelectionChanged.AddDynamic(this, &ThisClass::OnSelectionChangedGraphicQualityComboBox);
	
	//전체 화면 체크 박스
	FullscreenCheckBox->OnCheckStateChanged.AddDynamic(this, &ThisClass::OnCheckStateChangedFullscreenCheckBox);
	
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
	
	//그래픽 품질
	{
		const int32 TargetIndex = GameUserSettings->GetOverallScalabilityLevel();
		GraphicQualityComboBox->SetSelectedIndex(TargetIndex);
	}
	
	//전체 화면
	{
		const bool bFullscreen = GameUserSettings->GetFullscreenMode() == EWindowMode::Type::WindowedFullscreen;
		FullscreenCheckBox->SetCheckedState(bFullscreen ? ECheckBoxState::Checked : ECheckBoxState::Unchecked);
	}
	
	//수직 동기화
	{
		const bool bVSync = GameUserSettings->IsVSyncEnabled();
		VSyncCheckBox->SetCheckedState(bVSync ? ECheckBoxState::Checked : ECheckBoxState::Unchecked);
	}
}

void UT3DisplaySettings::ReinitializeByChangeLanguage()
{
	//그래픽 품질 콤보 박스의 선택 기억
	const int32 TempIndex = GraphicQualityComboBox->GetSelectedIndex();
	//콤보 박스를 비우고 다시 채우기
	GraphicQualityComboBox->ClearOptions();
	for (const FString& Key : GRAPHIC_KEY_STRINGS)
	{
		FString OptionString = UT3GameInstance::GetStringFromTable(NAMESPACE_NAME, Key);
		GraphicQualityComboBox->AddOption(OptionString);
	}
	//다시 채운뒤 선택 원복
	GraphicQualityComboBox->SetSelectedIndex(TempIndex);
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

void UT3DisplaySettings::OnCheckStateChangedFullscreenCheckBox(bool bIsChecked)
{
	//true : WindowedFullScreen, false : Windowed
	SetWindowMode(bIsChecked ? EWindowMode::Type::WindowedFullscreen : EWindowMode::Type::Windowed);
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
