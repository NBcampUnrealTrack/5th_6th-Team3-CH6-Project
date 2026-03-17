// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/SParameterPanel.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Layout/SExpandableArea.h"
#include "Widgets/Layout/SGridPanel.h"
#include "Widgets/Layout/SUniformGridPanel.h"
#include "Styling/AppStyle.h"

#define LOCTEXT_NAMESPACE "VarcoSound"

void SParameterWidget::Construct(const FArguments& InArgs)
{
	Config = InArgs._Config;
	CurrentValue = Config.DefaultValue;
	OnFloatValueChangedDelegate = InArgs._OnFloatValueChanged;
	OnInt32ValueChangedDelegate = InArgs._OnInt32ValueChanged;

	TSharedPtr<SWidget> SpinBoxWidget;

	if (Config.Type == EParameterType::Float)
	{
		SAssignNew(SpinBoxFloat, SNumericEntryBox<float>)
			.MinValue(Config.MinValue)
			.MaxValue(Config.MaxValue)
			.MinSliderValue(Config.MinValue)
			.MaxSliderValue(Config.MaxValue)
			.Value_Lambda([this]() -> TOptional<float> { return CurrentValue; })
			.OnValueChanged(this, &SParameterWidget::OnSpinBoxFloatValueChanged)
			.AllowSpin(true)
			.Delta(FMath::Pow(10.0f, -Config.Precision));

		SpinBoxWidget = SpinBoxFloat;
	}
	else
	{
		SAssignNew(SpinBoxInt32, SNumericEntryBox<int32>)
			.MinValue(static_cast<int32>(Config.MinValue))
			.MaxValue(static_cast<int32>(Config.MaxValue))
			.MinSliderValue(static_cast<int32>(Config.MinValue))
			.MaxSliderValue(static_cast<int32>(Config.MaxValue))
			.Value_Lambda([this]() -> TOptional<int32> { return FMath::RoundToInt(CurrentValue); })
			.OnValueChanged(this, &SParameterWidget::OnSpinBoxInt32ValueChanged)
			.AllowSpin(true)
			.Delta(1);

		SpinBoxWidget = SpinBoxInt32;
	}

	ChildSlot
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(0, 4)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.FillWidth(1.0f)
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text(Config.Label)
				.ToolTipText(Config.Tooltip)
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(8, 0, 0, 0)
			[
				SNew(SBox)
				.WidthOverride(100.0f)
				[
					SpinBoxWidget.ToSharedRef()
				]
			]
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(0, 2)
		[
			SAssignNew(Slider, SSlider)
			.Value_Lambda([this]() -> float { return CurrentValue; })
			.OnValueChanged(this, &SParameterWidget::OnSliderValueChanged)
			.MinValue(Config.MinValue)
			.MaxValue(Config.MaxValue)
		]
	];
}

void SParameterWidget::SetFloatValue(float NewValue)
{
	CurrentValue = FMath::Clamp(NewValue, Config.MinValue, Config.MaxValue);
}

TOptional<float> SParameterWidget::GetFloatValue() const
{
	return CurrentValue;
}

void SParameterWidget::SetInt32Value(int32 NewValue)
{
	CurrentValue = static_cast<float>(FMath::Clamp(NewValue, static_cast<int32>(Config.MinValue), static_cast<int32>(Config.MaxValue)));
}

TOptional<int32> SParameterWidget::GetInt32Value() const
{
	return FMath::RoundToInt(CurrentValue);
}

void SParameterWidget::OnSliderValueChanged(float NewValue)
{
	CurrentValue = NewValue;

	if (Config.Type == EParameterType::Float)
	{
		OnFloatValueChangedDelegate.ExecuteIfBound(CurrentValue);
	}
	else
	{
		OnInt32ValueChangedDelegate.ExecuteIfBound(FMath::RoundToInt(CurrentValue));
	}
}

void SParameterWidget::OnSpinBoxFloatValueChanged(float NewValue)
{
	CurrentValue = NewValue;
	OnFloatValueChangedDelegate.ExecuteIfBound(CurrentValue);
}

void SParameterWidget::OnSpinBoxInt32ValueChanged(int32 NewValue)
{
	CurrentValue = static_cast<float>(NewValue);
	OnInt32ValueChangedDelegate.ExecuteIfBound(NewValue);
}

FText SParameterWidget::GetValueText() const
{
	if (Config.Type == EParameterType::Float)
	{
		return FText::AsNumber(CurrentValue);
	}
	else
	{
		return FText::AsNumber(FMath::RoundToInt(CurrentValue));
	}
}

//-----------------------------------------------------------------------------
// SParameterPanel
//-----------------------------------------------------------------------------

void SParameterPanel::Construct(const FArguments& InArgs)
{
	ChildSlot
	[
		SAssignNew(ExpandableArea, SExpandableArea)
		.AreaTitle(InArgs._Title)
		.InitiallyCollapsed(!InArgs._InitiallyExpanded)
		.Padding(FMargin(8.0f))
		.BodyContent()
		[
			SAssignNew(ParameterContainer, SVerticalBox)
		]
	];
}

void SParameterPanel::AddParameter(const FName& ParameterName, TSharedRef<SParameterWidget> ParameterWidget)
{
	ParameterMap.Add(ParameterName, ParameterWidget);

	ParameterContainer->AddSlot()
		.AutoHeight()
		.Padding(4, 2)
		[
			ParameterWidget
		];
}

TSharedPtr<SParameterWidget> SParameterPanel::GetParameter(const FName& ParameterName) const
{
	const TSharedPtr<SParameterWidget>* FoundWidget = ParameterMap.Find(ParameterName);
	return FoundWidget ? *FoundWidget : nullptr;
}

void SParameterPanel::SetExpanded(bool bExpanded)
{
	if (ExpandableArea.IsValid())
	{
		ExpandableArea->SetExpanded(bExpanded);
	}
}

bool SParameterPanel::IsExpanded() const
{
	return ExpandableArea.IsValid() ? ExpandableArea->IsExpanded() : false;
}

#undef LOCTEXT_NAMESPACE

