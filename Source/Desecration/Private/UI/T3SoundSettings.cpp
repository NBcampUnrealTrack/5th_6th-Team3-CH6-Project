#include "UI/T3SoundSettings.h"

#include "Components/Slider.h"
#include "Sound/SoundClass.h"

void UT3SoundSettings::NativeConstruct()
{
	Super::NativeConstruct();
	
	if (!BGMSoundClass || !SESoundClass)
	{
		UE_LOG(LogTemp, Error, TEXT("%s : BGMSoundClass이나 SESoundClass가 null"), *GetNameSafe(this));
		return;
	}
	
	//슬라이더에 대한 바인딩
	BGMSlider->OnValueChanged.AddDynamic(this, &ThisClass::OnValueChangedBGMSlider);
	SESlider->OnValueChanged.AddDynamic(this, &ThisClass::OnValueChangedSESlider);
}

void UT3SoundSettings::OnValueChangedBGMSlider(float value)
{
	if (BGMSoundClass)
	{
		BGMSoundClass->Properties.Volume = value;
	}
}

void UT3SoundSettings::OnValueChangedSESlider(float value)
{
	if (SESoundClass)
	{
		SESoundClass->Properties.Volume = value;
	}
}
