#pragma once

#include "CoreMinimal.h"
#include "T3SettingsPanelCategory.h"
#include "T3GeneralSettings.generated.h"

class UComboBoxString;

UCLASS()
class DESECRATION_API UT3GeneralSettings : public UT3SettingsPanelCategory 
{
	GENERATED_BODY()
	
public:
	virtual void NativeConstruct() override;
	
private:
	//언어 선택 콤보 박스의 내용 변경
	UFUNCTION()
	void OnSelectionChangedLanguageComboBox(FString SelectedItem, ESelectInfo::Type SelectionType);
	
	//언어 선택 콤보 박스
	UPROPERTY(meta = (AllowPrivateAccess = true, BindWidget))
	TObjectPtr<UComboBoxString> LanguageComboBox;
	
};
