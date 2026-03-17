// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Input/SNumericEntryBox.h"
#include "Widgets/Input/SSlider.h"

/**
 * 파라미터 타입 정의
 */
enum class EParameterType : uint8
{
	Float,
	Int32
};

/**
 * 파라미터 설정 구조체
 */
struct FParameterConfig
{
	FText Label;
	FText Tooltip;
	EParameterType Type;
	float MinValue;
	float MaxValue;
	float DefaultValue;
	int32 Precision;  // Float에서 소수점 자리수

	FParameterConfig()
		: Label(FText::GetEmpty())
		, Tooltip(FText::GetEmpty())
		, Type(EParameterType::Float)
		, MinValue(0.0f)
		, MaxValue(1.0f)
		, DefaultValue(0.5f)
		, Precision(2)
	{}
};

DECLARE_DELEGATE_OneParam(FOnFloatParameterChanged, float);
DECLARE_DELEGATE_OneParam(FOnInt32ParameterChanged, int32);

/**
 * 재사용 가능한 파라미터 위젯 (Slider + SpinBox)
 */
class VARCOSOUND_API SParameterWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SParameterWidget)
		: _Config()
	{}
		SLATE_ARGUMENT(FParameterConfig, Config)
		SLATE_EVENT(FOnFloatParameterChanged, OnFloatValueChanged)
		SLATE_EVENT(FOnInt32ParameterChanged, OnInt32ValueChanged)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	// 값 설정/가져오기
	void SetFloatValue(float NewValue);
	TOptional<float> GetFloatValue() const;
	void SetInt32Value(int32 NewValue);
	TOptional<int32> GetInt32Value() const;

private:
	// Internal callbacks
	void OnSliderValueChanged(float NewValue);
	void OnSpinBoxFloatValueChanged(float NewValue);
	void OnSpinBoxInt32ValueChanged(int32 NewValue);

	FText GetValueText() const;

	// Config
	FParameterConfig Config;

	// Current value (stored as float)
	float CurrentValue;

	// Delegates
	FOnFloatParameterChanged OnFloatValueChangedDelegate;
	FOnInt32ParameterChanged OnInt32ValueChangedDelegate;

	// UI Widgets
	TSharedPtr<SSlider> Slider;
	TSharedPtr<SNumericEntryBox<float>> SpinBoxFloat;
	TSharedPtr<SNumericEntryBox<int32>> SpinBoxInt32;
};

/**
 * 파라미터 패널 (여러 파라미터를 ExpandableArea로 감싸는 컨테이너)
 */
class VARCOSOUND_API SParameterPanel : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SParameterPanel)
		: _Title(FText::FromString(TEXT("Parameters")))
		, _InitiallyExpanded(false)
	{}
		SLATE_ARGUMENT(FText, Title)
		SLATE_ARGUMENT(bool, InitiallyExpanded)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	/**
	 * 파라미터 위젯 추가
	 * @param ParameterName 파라미터 식별자
	 * @param ParameterWidget 추가할 파라미터 위젯
	 */
	void AddParameter(const FName& ParameterName, TSharedRef<SParameterWidget> ParameterWidget);

	/**
	 * 파라미터 위젯 가져오기
	 */
	TSharedPtr<SParameterWidget> GetParameter(const FName& ParameterName) const;

	/**
	 * ExpandableArea 확장/축소 상태 설정
	 */
	void SetExpanded(bool bExpanded);
	bool IsExpanded() const;

private:
	TSharedPtr<class SExpandableArea> ExpandableArea;
	TSharedPtr<class SVerticalBox> ParameterContainer;

	// 파라미터 맵 (이름 -> 위젯)
	TMap<FName, TSharedPtr<SParameterWidget>> ParameterMap;
};

