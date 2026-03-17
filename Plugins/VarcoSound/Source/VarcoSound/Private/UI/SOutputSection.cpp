// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/SOutputSection.h"
#include "UI/SAudioResultView.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SSeparator.h"
#include "Styling/AppStyle.h"

#define LOCTEXT_NAMESPACE "VarcoSound"

void SOutputSection::Construct(const FArguments& InArgs)
{
	TSharedRef<SVerticalBox> ContentBox = SNew(SVerticalBox);

	// Title (optional)
	if (InArgs._ShowTitle)
	{
		ContentBox->AddSlot()
			.AutoHeight()
			.Padding(0, 8, 0, 4)
			[
				SNew(STextBlock)
				.Text(InArgs._Title)
				.Font(FCoreStyle::GetDefaultFontStyle("Bold", 12))
			];

		ContentBox->AddSlot()
			.AutoHeight()
			.Padding(0, 0, 0, 8)
			[
				SNew(SSeparator)
				.Orientation(Orient_Horizontal)
			];
	}

	// Custom widget container (위에 추가 위젯을 넣을 수 있음)
	ContentBox->AddSlot()
		.AutoHeight()
		[
			SAssignNew(CustomWidgetContainer, SVerticalBox)
		];

	// Audio Result View
	ContentBox->AddSlot()
		.FillHeight(1.0f)
		.Padding(0, 4)
		[
			SAssignNew(AudioResultView, SAudioResultView)
		];

	ChildSlot
	[
		SAssignNew(Container, SVerticalBox)
		+ SVerticalBox::Slot()
		.FillHeight(1.0f)
		[
			ContentBox
		]
	];
}

void SOutputSection::AddCustomWidget(TSharedRef<SWidget> Widget)
{
	if (CustomWidgetContainer.IsValid())
	{
		CustomWidgetContainer->AddSlot()
			.AutoHeight()
			.Padding(0, 2)
			[
				Widget
			];
	}
}

void SOutputSection::ClearContent()
{
	if (CustomWidgetContainer.IsValid())
	{
		CustomWidgetContainer->ClearChildren();
	}

	if (AudioResultView.IsValid())
	{
		AudioResultView->SetSoundWaves(TArray<USoundWave*>());
	}
}

#undef LOCTEXT_NAMESPACE

