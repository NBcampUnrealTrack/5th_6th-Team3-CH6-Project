// Copyright Epic Games, Inc. All Rights Reserved.

#include "Tabs/SAutoVariationTab.h"
#include "Widgets/Text/STextBlock.h"
#include "Components/AudioComponent.h"
#include "SlateOptMacros.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Input/SNumericEntryBox.h"
#include "Widgets/Input/SSlider.h"
#include "Utils/ApiClient.h"
#include "Utils/AudioUtils.h"
#include "Utils/ToastNotification.h"
#include "Utils/AssetSelectionHelpers.h"
#include "Sound/SoundWave.h"
#include "UI/SWaveformDisplay.h"
#include "UI/SAudioResultView.h"
#include "UI/SAudioPlayerControls.h"
#include "Styling/AppStyle.h"
#include "EditorFramework/AssetImportData.h"
#include "PropertyCustomizationHelpers.h"

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION

void SAutoVariationTab::Construct(const FArguments& InArgs)
{
    ApiClient = MakeShareable(new FApiClient());
    InputSoundWave = nullptr;
    NumSampleValue = 3; 
    StrengthValue = 1.0f;
    ChildSlot
    [
        SNew(SScrollBox)
        + SScrollBox::Slot()
        .Padding(FMargin(10.0f))
        [
            SNew(SVerticalBox)

            /* =========================
               Page title section
               ========================= */
            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(0, 10)
            [
                SNew(STextBlock)
                .Text(FText::FromString(TEXT("🎲Quickly create multiple sound variations - perfect for dynamic, randomized audio")))
                .Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
                .ColorAndOpacity(FLinearColor(0.7f, 0.7f, 0.7f, 1.0f))
            ]

            /* =========================
               Audio file selection section
               ========================= */
            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(0, 10)
            [
                SNew(SVerticalBox)
                
                // Input Audio Label and Create Variation button on the same line
                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(0, 5)
                [
                    SNew(SHorizontalBox)
                    
                    // Input Audio Label
                    + SHorizontalBox::Slot()
                    .AutoWidth()
                    .VAlign(VAlign_Center)
                    [
                        SNew(STextBlock)
                        .Text(FText::FromString(TEXT("Input Audio")))
                        .Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
                    ]
                    
                    // Empty space
                    + SHorizontalBox::Slot()
                    .FillWidth(1.0f)
                    [
                        SNew(SSpacer)
                    ]
                    
                    // Create Variation button
                    + SHorizontalBox::Slot()
                    .AutoWidth()
                    .VAlign(VAlign_Center)
                    [
                        SNew(SButton)
                        .Text(FText::FromString(TEXT("Create Variations")))
                        .OnClicked(this, &SAutoVariationTab::OnCreateVariationButtonClicked)
                    ]
                ]
                
                // Input Audio Selector
                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(0, 5)
                [
                    SAssignNew(AssetSelectorWidget, SObjectPropertyEntryBox)
                    .AllowedClass(USoundWave::StaticClass())
                    .ObjectPath(this, &SAutoVariationTab::GetCurrentAssetPath)
                    .OnObjectChanged(this, &SAutoVariationTab::OnAssetSelected)
                    .DisplayThumbnail(true)
                    .DisplayUseSelected(true)
                    .DisplayBrowse(true)
                ]
            ]
            
            /* =========================
               Audio waveform player section
               ========================= */
            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(0, 0)
            [
                SAssignNew(InputAudioPlayerWidget, SAudioPlayerControls)
                .WaveformHeight(100.0f)
                .ShowTitle(false)  // Hide title (to avoid duplicate Input Audio)
            ]
            
            /* =========================
               Parameter settings section (toggleable)
               ========================= */
            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(0, 2)
            [
                SAssignNew(ParametersExpandableArea, SExpandableArea)
                .InitiallyCollapsed(true)  // Initially collapsed state
                .HeaderContent()
                [
                    SNew(STextBlock)
                    .Text(FText::FromString(TEXT("Parameters")))
                    .Font(FCoreStyle::GetDefaultFontStyle("Normal", 10))
                ]
                .BodyContent()
                [
                    SNew(SVerticalBox)
                    
                    /* -------------------------
                       Sample count setting row
                       ------------------------- */
                    + SVerticalBox::Slot()
                    .AutoHeight()
                    .Padding(0, 1)
                    [
                        SNew(SBorder)
                        .BorderImage(FAppStyle::GetBrush("DetailsView.CategoryMiddle"))
                        .BorderBackgroundColor(FLinearColor(0.0f, 0.0f, 0.0f, 0.1f))  // Light background color
                        .Padding(FMargin(6, 4))
                        [
                            SNew(SHorizontalBox)
                            
                            + SHorizontalBox::Slot()
                            .AutoWidth()
                            .VAlign(VAlign_Center)
                            .Padding(0, 0, 10, 0)
                            [
                                SNew(SBox)
                                .WidthOverride(100)
                                [
                                    SNew(STextBlock)
                                    .Text(FText::FromString(TEXT("Samples")))
                                    .Font(FAppStyle::GetFontStyle("PropertyWindow.NormalFont"))
                                ]
                            ]
                            
                            + SHorizontalBox::Slot()
                            .FillWidth(1.0f)
                            .VAlign(VAlign_Center)
                            [
                                SAssignNew(NumSampleSpinBox, SNumericEntryBox<int32>)
                                .Value(this, &SAutoVariationTab::GetNumSampleValue)
                                .OnValueChanged(this, &SAutoVariationTab::OnNumSampleValueChanged)
                                .MinValue(1)
                                .MaxValue(3)
                                .MinSliderValue(1)
                                .MaxSliderValue(3)
                                .AllowSpin(true)
                                .SliderExponent(1.0f)
                            ]
                        ]
                    ]
                    
                    /* -------------------------
                       Variation strength setting row
                       ------------------------- */
                    + SVerticalBox::Slot()
                    .AutoHeight()
                    .Padding(0, 1)
                    [
                        SNew(SBorder)
                        .BorderImage(FAppStyle::GetBrush("DetailsView.CategoryMiddle"))
                        .BorderBackgroundColor(FLinearColor(0.0f, 0.0f, 0.0f, 0.1f))  // Light background color
                        .Padding(FMargin(6, 4))
                        [
                            SNew(SHorizontalBox)
                            
                            + SHorizontalBox::Slot()
                            .AutoWidth()
                            .VAlign(VAlign_Center)
                            .Padding(0, 0, 10, 0)
                            [
                                SNew(SBox)
                                .WidthOverride(100)
                                [
                                    SNew(STextBlock)
                                    .Text(FText::FromString(TEXT("Strength")))
                                    .Font(FAppStyle::GetFontStyle("PropertyWindow.NormalFont"))
                                ]
                            ]
                            
                            + SHorizontalBox::Slot()
                            .FillWidth(0.7f)
                            .VAlign(VAlign_Center)
                            [
                                SAssignNew(StrengthSlider, SSlider)
                                .Value(this, &SAutoVariationTab::GetStrengthValue)
                                .OnValueChanged(this, &SAutoVariationTab::OnStrengthValueChanged)
                                .MinValue(0.0f)
                                .MaxValue(3.0f)
                                .StepSize(0.1f)
                            ]
                            
                            + SHorizontalBox::Slot()
                            .AutoWidth()
                            .VAlign(VAlign_Center)
                            .Padding(8, 0, 0, 0)
                            [
                                SNew(SBox)
                                .WidthOverride(40)
                                [
                                    SAssignNew(StrengthValueText, STextBlock)
                                    .Text_Lambda([this]() { return FText::FromString(FString::Printf(TEXT("%.1f"), StrengthValue)); })
                                    .Justification(ETextJustify::Center)
                                    .Font(FAppStyle::GetFontStyle("PropertyWindow.NormalFont"))
                                ]
                            ]
                        ]
                    ]
                ]
            ]
            
            /* =========================
               Result audio output section (conditional display)
               ========================= */
            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(0, 10)
            [
                SAssignNew(OutputSectionContainer, SVerticalBox)
                .Visibility_Lambda([this]() -> EVisibility
                {
                    // Show while loading, no waveform, and not loading
                    if (OutputAudioResultView.IsValid())
                    {
                        return (OutputAudioResultView->IsLoading() || OutputAudioResultView->HasSoundWaves())
                            ? EVisibility::Visible
                            : EVisibility::Collapsed;
                    }
                    return EVisibility::Collapsed;
                })
                
                // Divider
                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(0, 5)
                [
                    SNew(SBox)
                    .HeightOverride(1.0f)
                    [
                        SNew(SBorder)
                        .BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
                        .BorderBackgroundColor(FLinearColor(0.0f, 0.0f, 0.0f, 0.3f))
                    ]
                ]
                
                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(0, 5)
                [
                    SAssignNew(OutputAudioResultView, SAudioResultView)
                ]
            ]
		]
	];

    // Set tab type to SAudioResultView
    if (OutputAudioResultView.IsValid())
    {
        OutputAudioResultView->SetTabType(EAudioResultViewTabType::AutoVariation);
    }
}

SAutoVariationTab::~SAutoVariationTab()
{
    // New component handles its own cleanup
}

void SAutoVariationTab::PauseAllPlayback()
{
    if (InputAudioPlayerWidget.IsValid())
    {
        InputAudioPlayerWidget->Pause();
    }
    if (OutputAudioResultView.IsValid())
    {
        OutputAudioResultView->Pause();
    }
}

void SAutoVariationTab::SetApiKey(const FString& InApiKey)
{
    if (ApiClient.IsValid())
    {
        ApiClient->SetApiKey(InApiKey);
    }
}

void SAutoVariationTab::UpdateInputAudioPlayer()
{
    if (InputAudioPlayerWidget.IsValid())
    {
        InputAudioPlayerWidget->SetSoundWave(InputSoundWave);
    }
}

FReply SAutoVariationTab::OnCreateVariationButtonClicked()
{
    if (InputSoundWave && ApiClient.IsValid())
    {
        if (OutputAudioResultView.IsValid())
        {
            // Start loading: clear waveforms and show loading indicator
            OutputAudioResultView->SetLoading(true);
			OutputAudioResultView->SetSourceTag(TEXT("Unknown"));
            OutputAudioResultView->SetSoundWaves(TArray<USoundWave*>());
        }

        UE_LOG(LogTemp, Log, TEXT("Sending Variation request for SoundWave: %s"), *InputSoundWave->GetName());
        ApiClient->SendVariationRequest(InputSoundWave, NumSampleValue, StrengthValue, FOnVariationApiResponse::CreateSP(this, &SAutoVariationTab::OnVariationApiResponse));
    }
    return FReply::Handled();
}

void SAutoVariationTab::OnVariationApiResponse(const TArray<FString>& AudioBase64Array)
{
	if (OutputAudioResultView.IsValid())
	{
		// When response arrives, clear loading
		OutputAudioResultView->SetLoading(false);
		// Initialize playback state before new result
		OutputAudioResultView->ResetPlaybackState();
		if (AudioBase64Array.Num() > 0)
		{
			TArray<USoundWave*> SoundWaves;
			for (const FString& AudioBase64 : AudioBase64Array)
			{
				USoundWave* OutputSoundWave = FAudioUtils::CreateSoundWaveFromBase64(AudioBase64);
				if (OutputSoundWave)
				{
					SoundWaves.Add(OutputSoundWave);
				}
			}
				OutputAudioResultView->SetSourceTag(TEXT("Base64"));
				OutputAudioResultView->SetSoundWaves(SoundWaves);
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("Received empty audio data from variation API."));
			OutputAudioResultView->SetSoundWaves(TArray<USoundWave*>());
			VarcoSoundToast::ShowError(FText::FromString(TEXT("Variation creation failed: empty response or server error")));
		}
		
		// Invalidate UI visibility to update
		if (OutputSectionContainer.IsValid())
		{
			OutputSectionContainer->Invalidate(EInvalidateWidget::Layout);
		}
	}
}



FString SAutoVariationTab::GetCurrentAssetPath() const
{
    return FAssetSelectionHelpers::GetSoundWaveAssetPath(InputSoundWave);
}

void SAutoVariationTab::OnAssetSelected(const FAssetData& AssetData)
{
    USoundWave* SelectedSound = FAssetSelectionHelpers::GetSoundWaveFromAssetData(AssetData);
    if (SelectedSound)
    {
        InputSoundWave = SelectedSound;
        UpdateInputAudioPlayer();
        
        // Set source filename to SAudioResultView
        if (OutputAudioResultView.IsValid())
        {
            FString SourceFilePath = FAssetSelectionHelpers::GetSoundWaveSourceFilePath(InputSoundWave);
            OutputAudioResultView->SetSourceFilename(SourceFilePath);
        }
    }
}

TOptional<int32> SAutoVariationTab::GetNumSampleValue() const
{
    return NumSampleValue;
}

void SAutoVariationTab::OnNumSampleValueChanged(int32 NewValue)
{
    NumSampleValue = FMath::Clamp(NewValue, 1, 3);
}

float SAutoVariationTab::GetStrengthValue() const
{
    return StrengthValue;
}

void SAutoVariationTab::OnStrengthValueChanged(float NewValue)
{
    StrengthValue = NewValue;
}

END_SLATE_FUNCTION_BUILD_OPTIMIZATION 