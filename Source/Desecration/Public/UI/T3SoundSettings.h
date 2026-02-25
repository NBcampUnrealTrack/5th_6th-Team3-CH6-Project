#pragma once

#include "CoreMinimal.h"
#include "T3SettingsPanelCategory.h"
#include "T3SoundSettings.generated.h"

UCLASS()
class DESECRATION_API UT3SoundSettings : public UT3SettingsPanelCategory
{
	GENERATED_BODY()
	
public:
	virtual void NativeConstruct() override;
	
private:
	//배경음 슬라이더
	UFUNCTION()
	void OnValueChangedBGMSlider(float value);
	
	//효과음 슬라이더
	UFUNCTION()
	void OnValueChangedSESlider(float value);
	
	//배경음 슬라이더
	UPROPERTY(meta = (AllowPrivateAccess = true, BindWidget))
	TObjectPtr<USlider> BGMSlider;
	
	//효과음 슬라이더
	UPROPERTY(meta = (AllowPrivateAccess = true, BindWidget))
	TObjectPtr<USlider> SESlider;

	//배경음 사운드 클래스
	UPROPERTY(EditDefaultsOnly, Category = "Sound Class", meta = (AllowPrivateAccess = true))
	TObjectPtr<USoundClass> BGMSoundClass;
	
	//효과음 사운드 클래스
	UPROPERTY(EditDefaultsOnly, Category = "Sound Class", meta = (AllowPrivateAccess = true))
	TObjectPtr<USoundClass> SESoundClass;
};
