#include "UI/T3GeneralSettings.h"

#include "Components/ComboBoxString.h"
#include "Kismet/KismetInternationalizationLibrary.h"

void UT3GeneralSettings::CustomNativeConstruct()
{	
	//콤보 박스에 항목 추가
	for (const ELanguage Language : TEnumRange<ELanguage>())
	{
		if (Language == ELanguage::None)
		{
			break;
		}
		
		const FString OptionString = ELanguageToTranslatedString(Language);
		LanguageComboBox->AddOption(OptionString);
	}
	
	//콤보 박스에 대한 바인딩
	LanguageComboBox->OnSelectionChanged.AddDynamic(this, &ThisClass::OnSelectionChangedLanguageComboBox);
}

void UT3GeneralSettings::InitializeSettingsPanel()
{
	//언어 설정
	FString IETF = UKismetInternationalizationLibrary::GetCurrentCulture();
	ELanguage Language = IETFToELanguage(IETF);
	LanguageComboBox->SetSelectedIndex(static_cast<int32>(Language));
}

void UT3GeneralSettings::OnSelectionChangedLanguageComboBox(FString SelectedItem, ESelectInfo::Type SelectionType)
{
	//코드에 의한 변경은 무시
	if (SelectionType == ESelectInfo::Type::Direct)
	{
		return;
	}
	
	//선택한 언어에 대한 IETF 코드 획득
	const ELanguage SelectedLanguage = static_cast<ELanguage>(LanguageComboBox->FindOptionIndex(SelectedItem));
	const FString IETF = ELanguageToIETF(SelectedLanguage);
	
	//언어 변경
	UKismetInternationalizationLibrary::SetCurrentCulture(IETF);
	
	//세팅 패널을 포함한 전체에 적용
	if (UT3SettingsPanel* Panel = SettingsPanel.Get())
	{
		Panel->ApplyChangeLanguage();
	}
}

FString UT3GeneralSettings::ELanguageToTranslatedString(const ELanguage Language)
{
	switch (Language)
	{
	case ELanguage::English:
		return TEXT("English");
		
	case ELanguage::Korean:
		return TEXT("한국어");
		
	default:
		break;
	}
	
	return FString();
}

FString UT3GeneralSettings::ELanguageToIETF(const ELanguage Language)
{
	return UEnum::GetDisplayValueAsText(Language).ToString();
}

ELanguage UT3GeneralSettings::IETFToELanguage(const FString& IETF)
{
	for (const ELanguage Language : TEnumRange<ELanguage>())
	{
		if (Language == ELanguage::None)
		{
			break;
		}

		if (FString Target = ELanguageToIETF(Language); Target == IETF)
		{
			return Language;
		}
	}
	
	return ELanguage::None;
}
