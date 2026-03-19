// Copyright Epic Games, Inc. All Rights Reserved.

#include "Tabs/SAutoLoopTab.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SSpacer.h"
#include "Utils/ApiClient.h"
#include "Utils/AudioUtils.h"
#include "Utils/AssetSelectionHelpers.h"
#include "Utils/ToastNotification.h"
#include "UI/SAudioPlayerControls.h"
#include "UI/SAudioResultView.h"
#include "Styling/AppStyle.h"
#include "PropertyCustomizationHelpers.h"

void SAutoLoopTab::Construct(const FArguments& InArgs)
{
    ApiClient = MakeShareable(new FApiClient());
    InputSoundWave = nullptr;

    ChildSlot
    [
        SNew(SScrollBox)
        + SScrollBox::Slot()
        .Padding(FMargin(10.0f))
        [
            SNew(SVerticalBox)

            /* =========================
               Page Title Section
               ========================= */
            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(0, 10)
            [
                SNew(STextBlock)
                .Text(FText::FromString(TEXT("↻ Automatically create seamless audio loops from any sound.")))
                .Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
                .ColorAndOpacity(FLinearColor(0.7f, 0.7f, 0.7f, 1.0f))
            ]

            /* =========================
               Input Audio Selection
               ========================= */
            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(0, 10)
            [
                SNew(SVerticalBox)

                // Label and Button Row
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

                    // Spacer
                    + SHorizontalBox::Slot()
                    .FillWidth(1.0f)
                    [
                        SNew(SSpacer)
                    ]

                    // Create Loop Button
                    + SHorizontalBox::Slot()
                    .AutoWidth()
                    .VAlign(VAlign_Center)
                    [
                        SNew(SButton)
                        .Text(FText::FromString(TEXT("Create Loop")))
                        .OnClicked(this, &SAutoLoopTab::OnCreateLoopButtonClicked)
                        .IsEnabled(this, &SAutoLoopTab::CanGenerate)
                    ]
                ]

                // Asset Selector
                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(0, 5)
                [
                    SAssignNew(AssetSelectorWidget, SObjectPropertyEntryBox)
                    .AllowedClass(USoundWave::StaticClass())
                    .ObjectPath(this, &SAutoLoopTab::GetCurrentAssetPath)
                    .OnObjectChanged(this, &SAutoLoopTab::OnAssetSelected)
                    .DisplayThumbnail(true)
                    .DisplayUseSelected(true)
                    .DisplayBrowse(true)
                ]
            ]

            /* =========================
               Audio Player Preview
               ========================= */
            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(0, 0)
            [
                SAssignNew(InputAudioPlayerWidget, SAudioPlayerControls)
                .WaveformHeight(100.0f)
                .ShowTitle(false)
            ]

            /* =========================
               Result Output Section
               ========================= */
            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(0, 10)
            [
                SAssignNew(OutputSectionContainer, SVerticalBox)
                .Visibility_Lambda([this]() -> EVisibility
                {
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

                // Result View
                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(0, 5)
                [
                    SAssignNew(OutputAudioResultView, SAudioResultView)
                ]
            ]
        ]
    ];
    
    // Set result view tab type
    if (OutputAudioResultView.IsValid())
    {
        OutputAudioResultView->SetTabType(EAudioResultViewTabType::AutoLoop);
    }
}

SAutoLoopTab::~SAutoLoopTab()
{
}

void SAutoLoopTab::PauseAllPlayback()
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

void SAutoLoopTab::SetApiKey(const FString& InApiKey)
{
    if (ApiClient.IsValid())
    {
        ApiClient->SetApiKey(InApiKey);
    }
}

FReply SAutoLoopTab::OnCreateLoopButtonClicked()
{
    if (!InputSoundWave || !ApiClient.IsValid())
    {
        return FReply::Handled();
    }

    // Set loading state
    if (OutputAudioResultView.IsValid())
    {
        OutputAudioResultView->SetLoading(true);
		OutputAudioResultView->SetSourceTag(TEXT("Unknown"));
        OutputAudioResultView->SetSoundWaves(TArray<USoundWave*>());
    }

    ApiClient->SendLoopingRequest(
        InputSoundWave, 
        FOnLoopingApiResponse::CreateSP(this, &SAutoLoopTab::OnLoopingApiResponse)
    );
    
    return FReply::Handled();
}

void SAutoLoopTab::OnLoopingApiResponse(const FString& AudioBase64)
{
    if (OutputAudioResultView.IsValid())
    {
        OutputAudioResultView->SetLoading(false);
        OutputAudioResultView->ResetPlaybackState();

        if (!AudioBase64.IsEmpty())
        {
            USoundWave* OutputSoundWave = FAudioUtils::CreateSoundWaveFromBase64(AudioBase64);
            if (OutputSoundWave)
            {
                TArray<USoundWave*> SoundWaves;
                SoundWaves.Add(OutputSoundWave);
                OutputAudioResultView->SetSourceTag(TEXT("Base64"));
                OutputAudioResultView->SetSoundWaves(SoundWaves);
                
                // Auto-play result for looping tab
                OutputAudioResultView->Play();
            }
        }
        else
        {
            VarcoSoundToast::ShowError(FText::FromString(TEXT("Loop generation failed.")));
        }
        
        if (OutputSectionContainer.IsValid())
        {
            OutputSectionContainer->Invalidate(EInvalidateWidget::Layout);
        }
    }
}

bool SAutoLoopTab::CanGenerate() const
{
    return InputSoundWave != nullptr;
}

void SAutoLoopTab::OnAssetSelected(const FAssetData& AssetData)
{
    USoundWave* SelectedSound = FAssetSelectionHelpers::GetSoundWaveFromAssetData(AssetData);
    if (SelectedSound)
    {
        InputSoundWave = SelectedSound;
        UpdateInputAudioPlayer();
        
        // Pass source filename to result view for metadata if needed
        if (OutputAudioResultView.IsValid())
        {
            FString SourceFilePath = FAssetSelectionHelpers::GetSoundWaveSourceFilePath(InputSoundWave);
            OutputAudioResultView->SetSourceFilename(SourceFilePath);
        }
    }
}

FString SAutoLoopTab::GetCurrentAssetPath() const
{
    return FAssetSelectionHelpers::GetSoundWaveAssetPath(InputSoundWave);
}

void SAutoLoopTab::UpdateInputAudioPlayer()
{
    if (InputAudioPlayerWidget.IsValid())
    {
        InputAudioPlayerWidget->SetSoundWave(InputSoundWave);
    }
}

void SAutoLoopTab::ResetPlaybackState()
{
    if (OutputAudioResultView.IsValid())
    {
        OutputAudioResultView->ResetPlaybackState();
    }
}

