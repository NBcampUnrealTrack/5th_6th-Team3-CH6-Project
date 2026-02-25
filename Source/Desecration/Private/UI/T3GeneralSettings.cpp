#include "UI/T3GeneralSettings.h"

#include "Components/ComboBoxString.h"

void UT3GeneralSettings::NativeConstruct()
{
	Super::NativeConstruct();
	
	//콤보 박스에 항목 추가
	for (ELanguage Language : TEnumRange<ELanguage>())
	{
		const FString DisplayName = UEnum::GetDisplayValueAsText(Language).ToString();
		LanguageComboBox->AddOption(DisplayName);
	}
	
	//콤보 박스에 대한 바인딩
	LanguageComboBox->OnSelectionChanged.AddDynamic(this, &ThisClass::OnSelectionChangedLanguageComboBox);
}

void UT3GeneralSettings::OnSelectionChangedLanguageComboBox(FString SelectedItem, ESelectInfo::Type SelectionType)
{

}
