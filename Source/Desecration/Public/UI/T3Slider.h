#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Slider.h"
#include "T3Slider.generated.h"

class UProgressBar;

//슬라이더와 프로그래스 바를 합친 유저 위젯 (슬라이더는 부모가 아니라 클래스 멤버)
UCLASS()
class DESECRATION_API UT3Slider : public UUserWidget
{
	GENERATED_BODY()
	
protected:
	virtual void NativeOnInitialized() override;
	
public:
	//슬라이더의 값 지정
	void SetValue(const float Value);
	
	//슬라이더의 OnValueChanged 이벤트
	//* 주의 : Clear 또는 RemoveAll 사용 금지
	FOnFloatValueChangedEvent& GetOnValueChangedEvent() const;
	
private:
	//슬라이더 동작시 프로그래스 바도 똑같이 움직인다. 
	UFUNCTION()
	void OnValueChangedOriginalSlider(const float Value);
	
	//슬라이더 처럼 보이는 프로그레스 바
	UPROPERTY(meta = (AllowPrivateAccess = true, BindWidget))
	TObjectPtr<UProgressBar> ProgressBarLikeSlider;
	
	//실제로 작동할 슬라이더
	UPROPERTY(VisibleDefaultsOnly, Category = "Slider", meta = (AllowPrivateAccess = true, BindWidget))
	TObjectPtr<USlider> OriginalSlider;
	
	//슬라이더의 최솟값
	UPROPERTY(EditAnywhere, Category = "Slider", meta = (AllowPrivateAccess = true))
	float SliderMinValue = 0.0f;
	
	//슬라이더의 최댓값
	UPROPERTY(EditAnywhere, Category = "Slider", meta = (AllowPrivateAccess = true))
	float SliderMaxValue = 1.0f;
};
