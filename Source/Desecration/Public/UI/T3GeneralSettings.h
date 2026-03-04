#pragma once

#include "CoreMinimal.h"
#include "T3SettingsPanelCategory.h"
#include "T3GeneralSettings.generated.h"

//언어 (추후 언어 추가시 반드시 IETF 코드를 DisplayName으로 포함할 것)
UENUM()
enum class ELanguage
{
	English UMETA(DisplayName = "en"),
	Korean UMETA(DisplayName = "ko"),
	None UMETA(Hidden)
};
ENUM_RANGE_BY_COUNT(ELanguage, ELanguage::None);

class UComboBoxString;

UCLASS()
class DESECRATION_API UT3GeneralSettings : public UT3SettingsPanelCategory 
{
	GENERATED_BODY()

protected:
	virtual void CustomNativeConstruct() override;
	virtual void InitializeSettingsPanel() override;
	
private:
	//언어 선택 콤보 박스의 내용 변경
	UFUNCTION()
	void OnSelectionChangedLanguageComboBox(FString SelectedItem, ESelectInfo::Type SelectionType);
	
	//ELanguage를 각국 언어로 번역된 String으로
	FString ELanguageToTranslatedString(ELanguage Language);
	
	//ELanguage를 IETF 코드로
	FString ELanguageToIETF(ELanguage Language);
	
	//IETF 코드를 ELanguage로
	ELanguage IETFToELanguage(const FString& IETF);
	
	//언어 선택 콤보 박스
	UPROPERTY(meta = (AllowPrivateAccess = true, BindWidget))
	TObjectPtr<UComboBoxString> LanguageComboBox;
};
