#include "UI/T3Slider.h"

#include "Components/ProgressBar.h"

void UT3Slider::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	//슬라이더 초기화
	OriginalSlider->SetMinValue(SliderMinValue);
	OriginalSlider->SetMaxValue(SliderMaxValue);
	OriginalSlider->OnValueChanged.AddDynamic(this, &ThisClass::OnValueChangedOriginalSlider);
}

void UT3Slider::SetValue(const float Value)
{
	OriginalSlider->SetValue(Value);
}

FOnFloatValueChangedEvent UT3Slider::GetOnValueChangedEvent() const
{
	return OriginalSlider->OnValueChanged;
}

void UT3Slider::OnValueChangedOriginalSlider(const float Value)
{
	ProgressBarLikeSlider->SetPercent(OriginalSlider->GetNormalizedValue());
}
