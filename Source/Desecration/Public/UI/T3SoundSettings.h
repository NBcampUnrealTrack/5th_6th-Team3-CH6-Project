#pragma once

#include "CoreMinimal.h"
#include "T3SettingsPanelCategory.h"
#include "T3SoundSettings.generated.h"

UCLASS()
class DESECRATION_API UT3SoundSettings : public UT3SettingsPanelCategory
{
	GENERATED_BODY()

protected:
	virtual void CustomNativeConstruct() override;
	virtual void InitializeSettingsPanel() override;
	
private:
	//배경음 슬라이더
	UFUNCTION()
	void OnValueChangedBGMSlider(float Value);
	
	//효과음 슬라이더
	UFUNCTION()
	void OnValueChangedSESlider(float Value);
	
	//배경음 슬라이더
	UPROPERTY(meta = (AllowPrivateAccess = true, BindWidget))
	TObjectPtr<USlider> BGMSlider;
	
	//효과음 슬라이더
	UPROPERTY(meta = (AllowPrivateAccess = true, BindWidget))
	TObjectPtr<USlider> SESlider;

	//배경음 사운드 클래스
	UPROPERTY()
	TObjectPtr<USoundClass> SoundClassBGM;
	
	//효과음 사운드 클래스
	UPROPERTY()
	TObjectPtr<USoundClass> SoundClassSE;
};
