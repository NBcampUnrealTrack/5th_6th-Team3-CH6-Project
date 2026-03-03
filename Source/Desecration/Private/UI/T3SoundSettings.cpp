#include "UI/T3SoundSettings.h"

#include "Components/Slider.h"
#include "Sound/SoundClass.h"

void UT3SoundSettings::CustomNativeConstruct()
{
	//사운드 클래스 가져오기
	SoundClassBGM = T3GameInstance->GetSoundClassBGM();
	SoundClassSE = T3GameInstance->GetSoundClassSE();
	if (!SoundClassBGM || !SoundClassSE)
	{
		UE_LOG(LogTemp, Error, TEXT("%s : SoundClassBGM 또는 SoundClassSE를 확인할 수 없음"), *GetNameSafe(this));
		return;
	}
	
	//슬라이더에 대한 바인딩
	BGMSlider->OnValueChanged.AddDynamic(this, &ThisClass::OnValueChangedBGMSlider);
	SESlider->OnValueChanged.AddDynamic(this, &ThisClass::OnValueChangedSESlider);
}

void UT3SoundSettings::InitializeSettingsPanel()
{
	BGMSlider->SetValue(SoundClassBGM->Properties.Volume);
	SESlider->SetValue(SoundClassSE->Properties.Volume);
}

void UT3SoundSettings::OnValueChangedBGMSlider(const float Value)
{
	if (SoundClassBGM)
	{
		SoundClassBGM->Properties.Volume = Value;
	}
}

void UT3SoundSettings::OnValueChangedSESlider(const float Value)
{
	if (SoundClassSE)
	{
		SoundClassSE->Properties.Volume = Value;
	}
}
