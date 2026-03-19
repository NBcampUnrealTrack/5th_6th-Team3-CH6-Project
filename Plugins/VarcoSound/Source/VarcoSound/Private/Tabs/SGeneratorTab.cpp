// Copyright Epic Games, Inc. All Rights Reserved.

#include "Tabs/SGeneratorTab.h"
#include "VarcoSound.h"

#include "Components/AudioComponent.h"
#include "SlateOptMacros.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Input/SCheckBox.h"
#include "Utils/ApiClient.h"
#include "Utils/AudioUtils.h"
#include "Utils/ToastNotification.h"
#include "Utils/ImagePreviewHelpers.h"
#include "Sound/SoundWave.h"
#include "UI/SAudioResultView.h"
#include "Styling/AppStyle.h"
#include "Engine/Texture2D.h"
#include "Engine/Texture.h"
#include "PropertyCustomizationHelpers.h"
#include "EditorFramework/AssetImportData.h"
#include "Styling/SlateBrush.h"
#include "Misc/Base64.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Input/SMultiLineEditableTextBox.h"
#include "Widgets/Input/SNumericEntryBox.h"
#include "Widgets/Layout/SExpandableArea.h"
#include "Widgets/Input/STextComboBox.h"
#include "Misc/FileHelper.h"
#include "HAL/PlatformFileManager.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonReader.h"
#include "Dom/JsonObject.h"
#include "Policies/CondensedJsonPrintPolicy.h"
#if WITH_EDITOR
#include "Utils/ImageCaptureUtils.h"
#endif
#include "Widgets/Images/SThrobber.h"

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION

void SGeneratorTab::Construct(const FArguments& InArgs)
{
    ApiClient = MakeShareable(new FApiClient());
    CurrentSelectedTab = EOptionTabType::PromptBooster; // Default selection: Prompt booster
    SelectedPromptIndex = -1; // Initial value: not selected
    HoveredPromptIndex = -1; // Initial value: not hovered
    SelectedImage2SfxLayerIndex = -1; // Initial value: not selected
    HoveredImage2SfxLayerIndex = -1; // Initial value: not hovered
    NumSamplesValue = 2; // Initial value: 2
    bStereoEnabled = true; // Initial value: On

    ChildSlot
    [
        SNew(SScrollBox)
        + SScrollBox::Slot()
        .Padding(FMargin(10.f, 10.f, 10.f, 0.f))
        [
            SNew(SVerticalBox)

            // Title Text
            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(0, 10)
            [
                SNew(STextBlock)
                .Text(FText::FromString(TEXT("⚡️Generate unique sound effects from text descriptions using AI.")))
                .Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
                .ColorAndOpacity(FLinearColor(0.7f, 0.7f, 0.7f, 1.0f))
            ]

            // Prompt Label
            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(0, 5)
            [
                SNew(STextBlock)
                .Text(FText::FromString(TEXT("Prompt")))
                .Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
            ]

            // Input Field and Generate Button in the same line
            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(0, 5)
            [
                SNew(SHorizontalBox)
                
                // Text Input (Left, Fills most of the space) - Change to multi-line
                + SHorizontalBox::Slot()
                .FillWidth(1.0f)
                .VAlign(VAlign_Top)
                [
                    SNew(SBox)
                    .MinDesiredHeight(40.0f)
                    .MaxDesiredHeight(120.0f)
                    [
                        SAssignNew(PromptTextBox, SMultiLineEditableTextBox)
                        .HintText(FText::FromString(TEXT("Describe the sound you want to create...")))
                        .AutoWrapText(true)
                        .AllowMultiLine(true)
                        .OnKeyDownHandler(this, &SGeneratorTab::OnPromptKeyDown)
                    ]
                ]
                
                // Small space between
                + SHorizontalBox::Slot()
                .AutoWidth()
                .Padding(5, 0)
                
                // Generate Button (Right)
                + SHorizontalBox::Slot()
                .AutoWidth()
                .VAlign(VAlign_Top)
                [
                    SNew(SButton)
                    .Text(FText::FromString(TEXT("Generate")))
                    .OnClicked(this, &SGeneratorTab::OnGenerateButtonClicked)
                ]
            ]
            
            // Option tabs (below prompt input field, no spacing)
            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(0, 0, 0, 5) // Add spacing at the bottom
            [
                SNew(SHorizontalBox)
                
                // Prompt booster button
                + SHorizontalBox::Slot()
                .AutoWidth()
                .Padding(0, 0, 1, 0)
                [
                    SNew(SButton)
                    .Text(FText::FromString(TEXT("Prompt booster")))
                    .OnClicked(this, &SGeneratorTab::OnOptionTabClicked, EOptionTabType::PromptBooster)
                    .ButtonColorAndOpacity(this, &SGeneratorTab::GetOptionTabBackgroundColor, EOptionTabType::PromptBooster)
                    .ForegroundColor(this, &SGeneratorTab::GetOptionTabTextColor, EOptionTabType::PromptBooster)
                ]
                
                /* Multi-layering button (future implementation)
                + SHorizontalBox::Slot()
                .AutoWidth()
                .Padding(1, 0, 1, 0)
                [
                    SNew(SButton)
                    .Text(FText::FromString(TEXT("Multi-layering")))
                    .OnClicked(this, &SGeneratorTab::OnOptionTabClicked, EOptionTabType::MultiLayering)
                    .ButtonColorAndOpacity(this, &SGeneratorTab::GetOptionTabBackgroundColor, EOptionTabType::MultiLayering)
                    .ForegroundColor(this, &SGeneratorTab::GetOptionTabTextColor, EOptionTabType::MultiLayering)
                ]
                */
                 
                // Image Prompter button
                + SHorizontalBox::Slot()
                .AutoWidth()
                .Padding(1, 0, 0, 0)
                [
                    SNew(SButton)
                    .Text(FText::FromString(TEXT("Image to Sound")))
                    .OnClicked(this, &SGeneratorTab::OnOptionTabClicked, EOptionTabType::ImagePrompter)
                    .ButtonColorAndOpacity(this, &SGeneratorTab::GetOptionTabBackgroundColor, EOptionTabType::ImagePrompter)
                    .ForegroundColor(this, &SGeneratorTab::GetOptionTabTextColor, EOptionTabType::ImagePrompter)
                ]
                
                // Fill remaining space
                + SHorizontalBox::Slot()
                .FillWidth(1.0f)
            ]
            
            // Parameters section (toggleable)
            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(0, 2)
            [
                SAssignNew(GenParametersExpandableArea, SExpandableArea)
                .InitiallyCollapsed(true)  // Initially collapsed
                .HeaderContent()
                [
                    SNew(STextBlock)
                    .Text(FText::FromString(TEXT("Parameters")))
                    .Font(FCoreStyle::GetDefaultFontStyle("Normal", 10))
                ]
                .BodyContent()
                [
                    SNew(SVerticalBox)
                    
                    // Add separator (top of Parameters)
                    + SVerticalBox::Slot()
                    .AutoHeight()
                    .Padding(0, 0, 0, 5)
                    [
                         SNew(SBox)
                        .HeightOverride(1.0f)
                        [
                            SNew(SBorder)
                            .BorderImage(FAppStyle::GetBrush("DetailsView.CategoryMiddle"))
                            .BorderBackgroundColor(FLinearColor(0.0f, 0.0f, 0.0f, 0.5f))
                        ]
                    ]
                    
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
                                SAssignNew(NumSamplesSpinBox, SNumericEntryBox<int32>)
                                .Value(this, &SGeneratorTab::GetNumSamplesValue)
                                .OnValueChanged(this, &SGeneratorTab::OnNumSamplesValueChanged)
                                .MinValue(1)
                                .MaxValue(5)
                                .MinSliderValue(1)
                                .MaxSliderValue(5)
                                .AllowSpin(true)
                                .SliderExponent(1.0f)
                            ]
                        ]
                    ]
                    
                    /* -------------------------
                       Stereo setting row
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
                                    .Text(FText::FromString(TEXT("Stereo")))
                                    .Font(FAppStyle::GetFontStyle("PropertyWindow.NormalFont"))
                                ]
                            ]
                            
                            + SHorizontalBox::Slot()
                            .AutoWidth()
                            .VAlign(VAlign_Center)
                            [
                                SAssignNew(StereoCheckBox, SCheckBox)
                                .IsChecked(this, &SGeneratorTab::GetStereoCheckState)
                                .OnCheckStateChanged(this, &SGeneratorTab::OnStereoCheckStateChanged)
                            ]
                            
                            + SHorizontalBox::Slot()
                            .AutoWidth()
                            .VAlign(VAlign_Center)
                            .Padding(5, 0, 0, 0)
                            [
                                SNew(STextBlock)
                                .Text_Lambda([this]() 
                                { 
                                    return bStereoEnabled ? FText::FromString(TEXT("On")) : FText::FromString(TEXT("Off")); 
                                })
                                .Font(FAppStyle::GetFontStyle("PropertyWindow.NormalFont"))
                            ]
                        ]
                    ]
                ]
            ]
            
            // Add separator (below Parameters)
            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(0, 5)
            .HAlign(HAlign_Fill)
            [
                 SNew(SBox)
                .HeightOverride(1.0f)
                .HAlign(HAlign_Fill)
                .Visibility_Lambda([this]() -> EVisibility
                {
                    return ShouldShowPromptBoosterSeparator() ? EVisibility::Visible : EVisibility::Collapsed;
                })
                [
                    SNew(SBorder)
                    .BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush")) // Thin line
                    .BorderBackgroundColor(FLinearColor(0.0f, 0.0f, 0.0f, 0.3f))
                ]
            ]
            
            // Option tab content area
            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(0, 2)
            [
                SAssignNew(OptionContentContainer, SVerticalBox)
                
                // Prompt booster content (default display)
                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(0, 2)
                [
                    SNew(SVerticalBox)

                    // Loading display (waiting for suggestions)
                    + SVerticalBox::Slot()
                    .AutoHeight()
                    .Padding(0, 2)
                    [
                        SAssignNew(PromptBoosterLoadingContainer, SVerticalBox)
                        .Visibility_Lambda([this]() -> EVisibility
                        {
                            return (CurrentSelectedTab == EOptionTabType::PromptBooster && bIsPromptBoosterLoading)
                                ? EVisibility::Visible
                                : EVisibility::Collapsed;
                        })
                        + SVerticalBox::Slot()
                        .AutoHeight()
                        [
                            SNew(SHorizontalBox)
                            + SHorizontalBox::Slot()
                            .AutoWidth()
                            .VAlign(VAlign_Center)
                            [
                                SNew(SThrobber)
                                .NumPieces(3)
                            ]
                            + SHorizontalBox::Slot()
                            .AutoWidth()
                            .VAlign(VAlign_Center)
                            .Padding(8,0)
                            [
                                SNew(STextBlock)
                                .Text(FText::FromString(TEXT("Waiting for AI suggestions...")))
                                .Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
                                .ColorAndOpacity(FLinearColor(0.8f, 0.8f, 0.8f, 1.0f))
                            ]
                        ]
                    ]

                    // Suggestions list
                    + SVerticalBox::Slot()
                    .AutoHeight()
                    .Padding(0, 2)
                [
                    SAssignNew(PromptSuggestionsContainer, SVerticalBox)
                    .Visibility_Lambda([this]() -> EVisibility
                    {
                        return (CurrentSelectedTab == EOptionTabType::PromptBooster && CurrentPromptSuggestions.Num() > 0) 
                            ? EVisibility::Visible 
                            : EVisibility::Collapsed;
                    })
                    ]
                ]

                // Image Prompter content
                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(0, 5)
                [
                    SAssignNew(ImagePrompterContainer, SVerticalBox)
                    .Visibility_Lambda([this]() -> EVisibility
                    {
                        return (CurrentSelectedTab == EOptionTabType::ImagePrompter) 
                            ? EVisibility::Visible 
                            : EVisibility::Collapsed;
                    })

                    // Input Image label + source selection + capture button (one line)
                    + SVerticalBox::Slot()
                    .AutoHeight()
                    .Padding(0, 5)
                    [
                        SNew(SHorizontalBox)
                        + SHorizontalBox::Slot()
                        .AutoWidth()
                        .VAlign(VAlign_Center)
                        [
                            SNew(STextBlock)
                            .Text(FText::FromString(TEXT("Input Image")))
                        ]

                        + SHorizontalBox::Slot()
                        .AutoWidth()
                        .Padding(8, 0)
                        .VAlign(VAlign_Center)
                        [
                            SAssignNew(ImageSourceCombo, STextComboBox)
                            .OptionsSource(&ImageSourceOptions)
                            .OnSelectionChanged(this, &SGeneratorTab::OnImageSourceChanged)
                        ]

                        + SHorizontalBox::Slot()
                        .AutoWidth()
                        .Padding(8, 0)
                        .VAlign(VAlign_Center)
                        [
                            SNew(SButton)
                            .Text(this, &SGeneratorTab::GetCaptureButtonText)
                            .OnClicked(this, &SGeneratorTab::OnCaptureButtonClicked)
                            .Visibility(this, &SGeneratorTab::GetCaptureButtonVisibility)
                            [
                                SNew(SHorizontalBox)
                                + SHorizontalBox::Slot()
                                .AutoWidth()
                                .VAlign(VAlign_Center)
                                [
                                    SNew(STextBlock)
                                    .Text(this, &SGeneratorTab::GetCaptureButtonText)
                                ]
                            ]
                        ]
                    ]

                    // Image asset selection widget
                    + SVerticalBox::Slot()
                    .AutoHeight()
                    .Padding(0, 5)
                    [
                        SAssignNew(ImageAssetSelectorWidget, SObjectPropertyEntryBox)
                        .AllowedClass(UTexture2D::StaticClass())
                        .ObjectPath(TAttribute<FString>::CreateSP(this, &SGeneratorTab::GetCurrentImageAssetPath))
                        .OnObjectChanged(this, &SGeneratorTab::OnImageAssetSelected)
                        .DisplayThumbnail(true)
                        .DisplayUseSelected(true)
                        .DisplayBrowse(true)
                        .Visibility(this, &SGeneratorTab::GetImageAssetPickerVisibility)
                    ]

                    // Image preview (vertical fixed, horizontal ratio maintained)
                    + SVerticalBox::Slot()
                    .AutoHeight()
                    .Padding(0, 5)
                    [
                        SNew(SBox)
                        .WidthOverride(this, &SGeneratorTab::GetImagePreviewWidth)
                        .HeightOverride(this, &SGeneratorTab::GetImagePreviewHeight)
                        [
                            SAssignNew(ImagePreviewWidget, SImage)
                        ]
                    ]

                    // Image2Sfx layer list (scrollable)
                    + SVerticalBox::Slot()
                    .AutoHeight()
                    .Padding(0, 5)
                    [
                        SNew(SBox)
                        .MaxDesiredHeight(250.0f) // Maximum 300px, dynamic if items are few
                        [
                            SAssignNew(Image2SfxLayersScrollBox, SScrollBox)
                            .Visibility_Lambda([this]() -> EVisibility
                            {
                                return (CurrentSelectedTab == EOptionTabType::ImagePrompter && (bIsImage2SfxLoading || CurrentImage2SfxLayers.Num() > 0))
                                    ? EVisibility::Visible
                                    : EVisibility::Collapsed;
                            })
                .OnUserScrolled_Lambda([](float){})
                        ]
                    ]
                ]
            ]
            
            /* =========================
               Output audio section (always visible, visibility toggled by tab)
               ========================= */
            + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(0, 10)
                [
                    SAssignNew(OutputSectionContainer, SVerticalBox)
                    .Visibility_Lambda([this]() -> EVisibility
                    {
                        return ShouldShowOutputSection() ? EVisibility::Visible : EVisibility::Collapsed;
                    })
                    
                    // Separator
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
                    
                    // PromptBooster result view
                    + SVerticalBox::Slot()
                    .AutoHeight()
                    .Padding(0, 5)
                    [
                        SAssignNew(AudioResultView_PromptBooster, SAudioResultView)
                        .Visibility_Lambda([this]() -> EVisibility
                        {
                            return (CurrentSelectedTab == EOptionTabType::PromptBooster) ? EVisibility::Visible : EVisibility::Collapsed;
                        })
                    ]
                    
                    // MultiLayering section (unimplemented text UI)
                    + SVerticalBox::Slot()
                    .AutoHeight()
                    .Padding(0, 5)
                    [
                        SAssignNew(MultiLayeringContainer, SVerticalBox)
                        .Visibility_Lambda([this]() -> EVisibility
                        {
                            return (CurrentSelectedTab == EOptionTabType::MultiLayering) ? EVisibility::Visible : EVisibility::Collapsed;
                        })
                        
                        // Unimplemented guidance text
                        + SVerticalBox::Slot()
                        .AutoHeight()
                        [
                            SNew(STextBlock)
                            .Text(FText::FromString(TEXT("Multi-layering is not implemented yet.")))
                            .ColorAndOpacity(FLinearColor(0.8f, 0.6f, 0.2f, 1.0f))
                        ]
                    ]
                    
                    // ImagePrompter result view
                    + SVerticalBox::Slot()
                    .AutoHeight()
                    .Padding(0, 5)
                    [
                        SAssignNew(AudioResultView_ImagePrompter, SAudioResultView)
                        .Visibility_Lambda([this]() -> EVisibility
                        {
                            return (CurrentSelectedTab == EOptionTabType::ImagePrompter) ? EVisibility::Visible : EVisibility::Collapsed;
                        })
                    ]
                ]
        ]
        // === NEW: Generation History Section ===
        + SScrollBox::Slot()
        .Padding(FMargin(0.0f, 0.0f, 0.0f, 0.0f)) // Remove left/right padding (only parent padding)
        [
            SNew(SVerticalBox)
            
            // Separator & Header
            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(0, 0, 0, 0) // Remove bottom header spacing (list is flush)
            [
                SNew(SVerticalBox)
                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(0, 0, 0, 10)
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
                .Padding(10, 0, 0, 5) // Text is slightly indented
                .VAlign(VAlign_Center)
                [
                    SNew(STextBlock)
                    .Text(FText::FromString(TEXT("Generation History")))
                    .Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
                    .ColorAndOpacity(FLinearColor(0.8f, 0.8f, 0.8f, 1.0f))
                ]
            ]

            // History List Container
            + SVerticalBox::Slot()
            .AutoHeight()
            [
                SAssignNew(HistoryListContainer, SScrollBox)
                .Orientation(Orient_Vertical)
            ]
        ]
	];

    // Set tab type
    if (AudioResultView_PromptBooster.IsValid())
    {
        AudioResultView_PromptBooster->SetTabType(EAudioResultViewTabType::Generator);
    }
    if (AudioResultView_MultiLayering.IsValid())
    {
        AudioResultView_MultiLayering->SetTabType(EAudioResultViewTabType::Generator);
    }
    if (AudioResultView_ImagePrompter.IsValid())
    {
        AudioResultView_ImagePrompter->SetTabType(EAudioResultViewTabType::Generator);
    }

    // Initialize preview brush
    ImagePreviewBrush = MakeShared<FSlateBrush>();

    // Initialize Image Source options
    InitImageSourceOptions();
}

void SGeneratorTab::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
    SCompoundWidget::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);
}

SGeneratorTab::~SGeneratorTab()
{
    // Release temporary texture root for preview
    if (CapturedPreviewTexture.IsValid())
    {
        if (UTexture2D* Tex = CapturedPreviewTexture.Get())
        {
            if (Tex->IsRooted())
            {
                Tex->RemoveFromRoot();
            }
        }
        CapturedPreviewTexture.Reset();
    }
}

FReply SGeneratorTab::OnGenerateButtonClicked()
{
    FString PromptText = PromptTextBox->GetText().ToString();
    if (ApiClient.IsValid())
    {
        switch (CurrentSelectedTab)
        {
        case EOptionTabType::PromptBooster:
            if (!PromptText.IsEmpty())
            {
                SendPromptBoosterRequest(PromptText);
            }
            else
            {
                VarcoSoundToast::ShowError(FText::FromString(TEXT("Please enter a prompt.")));
            }
            break;
        case EOptionTabType::ImagePrompter:
        {
            // Apply default prompt value
            FString UsedPrompt = PromptText.TrimStartAndEnd();
            if (UsedPrompt.IsEmpty())
            {
                UsedPrompt = TEXT("sfx, foley, ambience");
            }

            FString ImageBase64;
            bool bHaveImage = false;

            // First: use captured buffer
            if (bHasPendingCapturedImage && PendingCapturedPng.Num() > 0)
            {
                // Check captured image size (50 MB limit)
                const int64 ImageSize = PendingCapturedPng.Num();
                if (ImageSize > 50 * 1024 * 1024)
                {
                    VarcoSoundToast::ShowError(FText::Format(
                        NSLOCTEXT("VarcoSound", "CapturedImageTooLarge", 
                            "Captured image is too large ({0} MB). Maximum size is 50 MB."),
                        FText::AsNumber(ImageSize / 1024.0 / 1024.0)));
                    break;
                }
                
                ImageBase64 = FBase64::Encode(PendingCapturedPng);
                bHaveImage = true;
            }
            else if (CurrentImageSourceType == EImageSourceType::ContentBrowser)
            {
                bHaveImage = ConvertImageAssetToBase64(ImageBase64);
            }

            if (bHaveImage)
            {
                // 1st step: transition to waiting for recommended layer (hide waveform area), layer area shows loading
                if (auto View = GetResultView(EOptionTabType::ImagePrompter))
                {
                    View->SetLoading(false);
                    View->SetSoundWaves(TArray<USoundWave*>());
                    View->SetPromptText(UsedPrompt);
                }
                SetImage2SfxLoading(true);
                ApiClient->SendImage2SfxRequest(UsedPrompt, ImageBase64, FOnImage2SfxApiResponse::CreateSP(this, &SGeneratorTab::OnImage2SfxApiResponse));
            }
            else
            {
                VarcoSoundToast::ShowError(FText::FromString(TEXT("No image found. Please capture or select an asset.")));
            }
            break;
        }
        case EOptionTabType::MultiLayering:
        default:
        {
            // Not implemented: only show message and do not call API
            VarcoSoundToast::ShowInfo(FText::FromString(TEXT("Multi-layering feature is not implemented yet.")));
            break;
        }
        }
    }
    return FReply::Handled();
}

void SGeneratorTab::OnPromptTextCommitted(const FText& Text, ETextCommit::Type CommitType)
{
    // Multi-line Enter commit is removed (only execute with Generate button)
}

FReply SGeneratorTab::OnPromptKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent)
{
    // When Enter key is pressed
    if (InKeyEvent.GetKey() == EKeys::Enter)
    {
        // If Shift is not pressed, execute Generate
        if (!InKeyEvent.IsShiftDown())
        {
            OnGenerateButtonClicked();
            return FReply::Handled();
        }
        // Shift+Enter is for new line (default behavior)
    }
    
    return FReply::Unhandled();
}

TOptional<int32> SGeneratorTab::GetNumSamplesValue() const
{
    return NumSamplesValue;
}

void SGeneratorTab::OnNumSamplesValueChanged(int32 NewValue)
{
    NumSamplesValue = FMath::Clamp(NewValue, 1, 3);
}

ECheckBoxState SGeneratorTab::GetStereoCheckState() const
{
    return bStereoEnabled ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
}

void SGeneratorTab::OnStereoCheckStateChanged(ECheckBoxState NewState)
{
    bStereoEnabled = (NewState == ECheckBoxState::Checked);
}

void SGeneratorTab::OnApiResponse(const TArray<FString>& AudioBase64Array)
{
    GeneratedSoundWaves.Empty();

    // [Fix] If response is empty (includes error), immediately turn off loading and exit
    if (AudioBase64Array.Num() == 0)
    {
        if (auto View = GetResultView(LastGenRequestTab))
        {
            View->SetLoading(false);
        }
        VarcoSoundToast::ShowError(FText::FromString(TEXT("Creation failed: server error or empty result.")));
        return;
    }
    
    // If Stereo option is enabled, apply Mono2Stereo conversion
    if (bStereoEnabled)
    {
        // Mono2Stereo must be processed asynchronously
        // Collect results for each Base64 by sending Mono2Stereo request
        int32 TotalCount = AudioBase64Array.Num();
        TSharedPtr<TArray<FString>> ConvertedArray = MakeShared<TArray<FString>>();
        TSharedPtr<int32> CompletedCount = MakeShared<int32>(0);
        
        for (const FString& Base64String : AudioBase64Array)
        {
            ApiClient->SendMono2StereoRequest(Base64String, FOnMono2StereoApiResponse::CreateLambda(
                [this, ConvertedArray, CompletedCount, TotalCount](const TArray<FString>& StereoResult)
                {
                    if (StereoResult.Num() > 0)
                    {
                        ConvertedArray->Add(StereoResult[0]);
                    }
                    
                    (*CompletedCount)++;
                    
                    // When all conversions are complete, display final result
                    if (*CompletedCount >= TotalCount)
                    {
                        for (const FString& ConvertedBase64 : *ConvertedArray)
                        {
                            USoundWave* NewSoundWave = FAudioUtils::CreateSoundWaveFromBase64(ConvertedBase64);
                            if (NewSoundWave)
                            {
                                GeneratedSoundWaves.Add(NewSoundWave);
                            }
                        }
                        
                        if (LastGenRequestTab != EOptionTabType::PromptBooster)
                        {
                        if (auto View = GetResultView(LastGenRequestTab))
                        {
                            View->SetLoading(false);
							View->SetSourceTag(TEXT("Base64"));
                            View->SetSoundWaves(GeneratedSoundWaves);
                            }
                        }
                        
                        if (GeneratedSoundWaves.Num() == 0)
                        {
                            VarcoSoundToast::ShowError(FText::FromString(TEXT("Stereo conversion failed")));
                        }
                        else
                        {
                            // Add history
                            FString LayerName, LayerCategory, HistoryPrompt;
                            TSharedPtr<FSlateBrush> Thumbnail = nullptr;

                            if (LastGenRequestTab == EOptionTabType::ImagePrompter)
                            {
                                if (CurrentImage2SfxLayers.IsValidIndex(SelectedImage2SfxLayerIndex))
                                {
                                    const auto& Layer = CurrentImage2SfxLayers[SelectedImage2SfxLayerIndex];
                                    LayerName = Layer.Name;
                                    LayerCategory = Layer.Category;
                                    HistoryPrompt = Layer.Prompt;
                                }
                            }
                            else
                            {
                                // PromptBooster
                                if (CurrentPromptSuggestions.IsValidIndex(SelectedPromptIndex))
                                {
                                    HistoryPrompt = CurrentPromptSuggestions[SelectedPromptIndex];
                                }
                                else
                                {
                                    HistoryPrompt = PromptTextBox->GetText().ToString();
                                }
                            }
                            
                            if (HistoryPrompt.IsEmpty()) HistoryPrompt = TEXT("Generated Audio");
                            
                            AddHistoryItem(GeneratedSoundWaves, HistoryPrompt, LastGenRequestTab, bStereoEnabled, LayerName, LayerCategory, Thumbnail, LastUserPromptText);
                        }
                    }
                }));
        }
    }
    else
    {
        // If Stereo option is disabled, use existing logic
        for (const FString& Base64String : AudioBase64Array)
        {
            USoundWave* NewSoundWave = FAudioUtils::CreateSoundWaveFromBase64(Base64String);
            if (NewSoundWave)
            {
                GeneratedSoundWaves.Add(NewSoundWave);
            }
        }

        if (LastGenRequestTab != EOptionTabType::PromptBooster && LastGenRequestTab != EOptionTabType::ImagePrompter)
        {
        if (auto View = GetResultView(LastGenRequestTab))
        {
            View->SetLoading(false);
			View->SetSourceTag(TEXT("Base64"));
            View->SetSoundWaves(GeneratedSoundWaves);
            }
        }
        if (LastGenRequestTab != EOptionTabType::PromptBooster && LastGenRequestTab != EOptionTabType::ImagePrompter)
        {
            if (auto View = GetResultView(LastGenRequestTab))
            {
                View->SetLoading(false);
				View->SetSourceTag(TEXT("Base64"));
                View->SetSoundWaves(GeneratedSoundWaves);
            }
        }
        if (GeneratedSoundWaves.Num() == 0)
        {
            VarcoSoundToast::ShowError(FText::FromString(TEXT("Creation failed: server error or empty result.")));
        }
        else
        {
            // Add history
            FString LayerName, LayerCategory, HistoryPrompt;
            TSharedPtr<FSlateBrush> Thumbnail = nullptr;

            if (LastGenRequestTab == EOptionTabType::ImagePrompter)
            {
                if (CurrentImage2SfxLayers.IsValidIndex(SelectedImage2SfxLayerIndex))
                {
                    const auto& Layer = CurrentImage2SfxLayers[SelectedImage2SfxLayerIndex];
                    LayerName = Layer.Name;
                    LayerCategory = Layer.Category;
                    HistoryPrompt = Layer.Description.IsEmpty() ? Layer.Prompt : Layer.Description;
                }
            }
            else
            {
                // PromptBooster
                if (CurrentPromptSuggestions.IsValidIndex(SelectedPromptIndex))
                {
                    HistoryPrompt = CurrentPromptSuggestions[SelectedPromptIndex];
                }
                else
                {
                    HistoryPrompt = PromptTextBox->GetText().ToString();
                }
            }
            
            if (HistoryPrompt.IsEmpty()) HistoryPrompt = TEXT("Generated Audio");
            
            AddHistoryItem(GeneratedSoundWaves, HistoryPrompt, LastGenRequestTab, bStereoEnabled, LayerName, LayerCategory, Thumbnail, LastUserPromptText);
        }
    }

    // Release ImagePrompter loading
    if (LastGenRequestTab == EOptionTabType::ImagePrompter)
    {
        bIsImagePrompterGenerating = false;
        LoadingImageLayerIndices.Empty();
        if (ImagePrompterLoadingContainer.IsValid())
        {
            ImagePrompterLoadingContainer->Invalidate(EInvalidateWidget::Layout);
        }
    }
}

FReply SGeneratorTab::OnLoadButtonClicked()
{
    TArray<FString> OutFilePaths;
    if (FAudioUtils::OpenAudioFileDialog(OutFilePaths))
    {
        if (OutFilePaths.Num() > 0)
        {
            GeneratedSoundWaves.Empty();
            USoundWave* LoadedSoundWave = FAudioUtils::CreateSoundWaveFromFile(OutFilePaths[0]);
            if (LoadedSoundWave)
            {
                GeneratedSoundWaves.Add(LoadedSoundWave);
                if (auto View = GetResultView(CurrentSelectedTab))
				{
					View->SetSourceTag(TEXT("SaveWav"));
					View->SetSoundWaves(GeneratedSoundWaves);
				}
            }
        }
    }
    return FReply::Handled();
}

FSlateColor SGeneratorTab::GetPromptSuggestionButtonColor(int32 Index) const
{
    if (CompletedPromptIndices.Contains(Index))
    {
        return FLinearColor(0.15f, 0.55f, 0.25f, 1.0f); // Completed prompt is green
    }
    if (SelectedPromptIndex == Index)
    {
        return FLinearColor(0.1f, 0.2f, 0.4f, 1.0f); // Selected prompt is blue
    }
    else if (HoveredPromptIndex == Index)
    {
        return FLinearColor(0.1f, 0.2f, 0.4f, 1.0f); // Hovered prompt is dark blue background
    }
    return FLinearColor(0.4f, 0.4f, 0.4f, 1.0f); // Default gray background
}

FReply SGeneratorTab::OnOptionTabClicked(EOptionTabType TabType)
{
    // When tab is switched, only pause current playback, keep state (waveform)
    PauseAllPlayback();

    CurrentSelectedTab = TabType;
    
    // Invalidate selected tab container
    if (TabType == EOptionTabType::PromptBooster)
    {
        if (PromptSuggestionsContainer.IsValid())
        {
            PromptSuggestionsContainer->Invalidate(EInvalidateWidget::Layout);
        }
    }
    else if (TabType == EOptionTabType::ImagePrompter)
    {
        if (ImagePrompterContainer.IsValid())
        {
            ImagePrompterContainer->Invalidate(EInvalidateWidget::Layout);
        }
    }

    if (OutputSectionContainer.IsValid())
    {
        OutputSectionContainer->Invalidate(EInvalidateWidget::Layout);
    }

    if (PromptTextBox.IsValid() && (TabType == EOptionTabType::PromptBooster || TabType == EOptionTabType::ImagePrompter))
    {
        PromptTextBox->SetText(FText::GetEmpty());
    }
    
    return FReply::Handled();
}

FSlateColor SGeneratorTab::GetOptionTabTextColor(EOptionTabType TabType) const
{
    if (CurrentSelectedTab == TabType)
    {
        return FLinearColor::White; // Selected tab is white text
    }
    return FLinearColor(0.7f, 0.7f, 0.7f, 1.0f); // Unselected tab is gray text
}

FSlateColor SGeneratorTab::GetOptionTabBackgroundColor(EOptionTabType TabType) const
{
    if (CurrentSelectedTab == TabType)
    {
        return FLinearColor(0.2f, 0.4f, 0.8f, 1.0f);
    }
    return FLinearColor(0.3f, 0.3f, 0.3f, 1.0f);
}

void SGeneratorTab::SendPromptBoosterRequest(const FString& Prompt)
{
    if (ApiClient.IsValid())
    {
        LastUserPromptText = Prompt;
        bIsPromptBoosterLoading = true;
        if (PromptBoosterLoadingContainer.IsValid())
        {
            PromptBoosterLoadingContainer->Invalidate(EInvalidateWidget::Layout);
        }
        ApiClient->SendPromptBoosterRequest(Prompt, FOnPromptBoosterApiResponse::CreateSP(this, &SGeneratorTab::OnPromptBoosterResponse));
    }
}

void SGeneratorTab::OnPromptBoosterResponse(const TArray<FString>& PromptSuggestions)
{
    bIsPromptBoosterLoading = false;
    if (PromptBoosterLoadingContainer.IsValid())
    {
        PromptBoosterLoadingContainer->Invalidate(EInvalidateWidget::Layout);
    }

    CurrentPromptSuggestions = PromptSuggestions;
    CompletedPromptIndices.Empty(); // Initialize completed state when new suggestions are received
    SelectedPromptIndex = -1; // Initialize selected state when new suggestions are received
    HoveredPromptIndex = -1; // Initialize hovered state when new suggestions are received
    
    // Update UI: create prompt suggestion list
    if (PromptSuggestionsContainer.IsValid())
    {
        PromptSuggestionsContainer->ClearChildren();
        
        if (CurrentPromptSuggestions.Num() > 0)
        {
            // Add title
            PromptSuggestionsContainer->AddSlot()
            .AutoHeight()
            .Padding(0, 2)
            [
                SNew(STextBlock)
                .Text(FText::FromString(TEXT("🚀 AI Prompt Suggestions:")))
                .Font(FCoreStyle::GetDefaultFontStyle("Bold", 9))
                .ColorAndOpacity(FLinearColor(0.8f, 0.8f, 0.8f, 1.0f))
            ];
            
            // Display each suggestion as a button
            for (int32 Index = 0; Index < CurrentPromptSuggestions.Num(); ++Index)
            {
                PromptSuggestionsContainer->AddSlot()
                .AutoHeight()
                .Padding(0, 1)
                [
                    SNew(SButton)
                    .OnClicked(this, &SGeneratorTab::OnPromptSuggestionClicked, Index)
                    .ButtonStyle(FCoreStyle::Get(), "FlatButton")
                    .ButtonColorAndOpacity(this, &SGeneratorTab::GetPromptSuggestionButtonColor, Index)
                    .ForegroundColor(FLinearColor::White)
                    .HAlign(HAlign_Fill)
                    .ContentPadding(FMargin(8, 4))
                    .OnHovered(this, &SGeneratorTab::OnPromptSuggestionHovered, Index)
                    .OnUnhovered(this, &SGeneratorTab::OnPromptSuggestionUnhovered)
                    [
                        SNew(SHorizontalBox)
                         
                        + SHorizontalBox::Slot()
                        .FillWidth(1.0f)
                        .VAlign(VAlign_Center)
                    [
                        SNew(STextBlock)
                        .Text_Lambda([this, Index]() -> FText
                        {
                            const bool bCompleted = CompletedPromptIndices.Contains(Index);
                            FString DisplayText = CurrentPromptSuggestions.IsValidIndex(Index) ? CurrentPromptSuggestions[Index] : FString();
                            if (bCompleted)
                            {
                                return FText::FromString(FString::Printf(TEXT("✔️ %s"), *DisplayText));
                            }
                            if (SelectedPromptIndex == Index)
                            {
                                return FText::FromString(DisplayText);
                            }
                            return FText::FromString(DisplayText);
                        })
                        .ColorAndOpacity(FLinearColor::White)
                        .Justification(ETextJustify::Left)
                        ]

                        // Loading spinner
                        + SHorizontalBox::Slot()
                        .AutoWidth()
                        .VAlign(VAlign_Center)
                        .Padding(5, 0, 0, 0)
                        [
                            SNew(SBox)
                            .WidthOverride(16)
                            .HeightOverride(16)
                            .Visibility_Lambda([this, Index]() -> EVisibility
                            {
                                return LoadingPromptIndices.Contains(Index) ? EVisibility::Visible : EVisibility::Collapsed;
                            })
                            [
                                SNew(SThrobber)
                                .NumPieces(1)
                                .Animate(SThrobber::Horizontal)
                            ]
                        ]
                    ]
                ];
            }
        }
        
        // Invalidate container to update visibility
        PromptSuggestionsContainer->Invalidate(EInvalidateWidget::Layout);
        
        // Automatically generate with the first suggestion
        if (CurrentPromptSuggestions.Num() > 0)
        {
            SelectedPromptIndex = 0; // Set the first prompt as selected
            OnPromptSuggestionClicked(0);
        }
    }
}

FReply SGeneratorTab::OnPromptSuggestionClicked(int32 Index)
{
    if (CurrentPromptSuggestions.IsValidIndex(Index))
    {
        // Block duplicate requests for already completed prompts
        if (CompletedPromptIndices.Contains(Index))
        {
            return FReply::Handled();
        }

        // Maximum simultaneous requests limit (5)
        if (LoadingPromptIndices.Num() >= 5 && !LoadingPromptIndices.Contains(Index))
        {
            VarcoSoundToast::ShowWarning(FText::FromString(TEXT("Maximum 5 prompts can be generated simultaneously.")));
            return FReply::Handled();
        }

        // Ignore items that are already loading
        if (LoadingPromptIndices.Contains(Index))
        {
            return FReply::Handled();
        }

        // Update selected prompt index
        SelectedPromptIndex = Index;
        
        // Update completed state (even if it's just a try) and UI
        CompletedPromptIndices.Add(Index);
        if (PromptSuggestionsContainer.IsValid())
        {
            PromptSuggestionsContainer->Invalidate(EInvalidateWidget::Layout);
        }

        // Add loading state
        LoadingPromptIndices.Add(Index);
        
        FString SelectedPrompt = CurrentPromptSuggestions[Index];
        
        // Call API with selected prompt
        if (ApiClient.IsValid())
        {
            LastGenRequestTab = EOptionTabType::PromptBooster;
            
            // Pass index via lambda capture
            ApiClient->SendGenerateRequest(SelectedPrompt, NumSamplesValue, FOnGenApiResponse::CreateLambda(
                [this, Index, SelectedPrompt](const TArray<FString>& AudioBase64Array)
                {
                    // Remove loading state
                    LoadingPromptIndices.Remove(Index);
                    
                    // Update UI (remove spinner)
                    if (PromptSuggestionsContainer.IsValid())
                    {
                        PromptSuggestionsContainer->Invalidate(EInvalidateWidget::Layout);
                    }

                    GeneratedSoundWaves.Empty();

                    // [Fix] If response is empty (includes error), immediately turn off loading and exit
                    if (AudioBase64Array.Num() == 0)
                    {
                        CompletedPromptIndices.Remove(Index);
                        if (PromptSuggestionsContainer.IsValid())
                        {
                            PromptSuggestionsContainer->Invalidate(EInvalidateWidget::Layout);
                        }
                        VarcoSoundToast::ShowError(NSLOCTEXT("VarcoSound", "GenerationFailed", "Generation failed: Server error or empty result."));
                        return;
                    }
                    
                    // If Stereo option is enabled, apply Mono2Stereo conversion
                    if (bStereoEnabled)
                    {
                        // Mono2Stereo must be processed asynchronously
                        int32 TotalCount = AudioBase64Array.Num();
                        TSharedPtr<TArray<FString>> ConvertedArray = MakeShared<TArray<FString>>();
                        TSharedPtr<int32> CompletedCount = MakeShared<int32>(0);
                        
                        // Note: this capture is not safe (can be destroyed during async), WeakPtr is recommended, but currently using this directly.
                        // Currently using this directly. In reality, TWeakPtr<SGeneratorTab> should be used.
                        // For now, follow the existing pattern.
                        
                        for (const FString& Base64String : AudioBase64Array)
            {
                            ApiClient->SendMono2StereoRequest(Base64String, FOnMono2StereoApiResponse::CreateLambda(
                                [this, ConvertedArray, CompletedCount, TotalCount, SelectedPrompt, Index](const TArray<FString>& StereoResult)
                                {
                                    if (StereoResult.Num() > 0)
                                    {
                                        ConvertedArray->Add(StereoResult[0]);
                                    }
                                    
                                    (*CompletedCount)++;
                                    
                                    // When all conversions are complete, display final result
                                    if (*CompletedCount >= TotalCount)
                                    {
                                        TArray<TObjectPtr<USoundWave>> LocalGeneratedWaves;
                                        for (const FString& ConvertedBase64 : *ConvertedArray)
                                        {
                                            USoundWave* NewSoundWave = FAudioUtils::CreateSoundWaveFromBase64(ConvertedBase64);
                                            if (NewSoundWave)
                                            {
                                                LocalGeneratedWaves.Add(NewSoundWave);
                                            }
                                        }
                                        if (LocalGeneratedWaves.Num() == 0)
                                        {
                                            CompletedPromptIndices.Remove(Index);
                                            if (PromptSuggestionsContainer.IsValid())
                                            {
                                                PromptSuggestionsContainer->Invalidate(EInvalidateWidget::Layout);
                                            }
                                            VarcoSoundToast::ShowError(FText::FromString(TEXT("Stereo conversion failed")));
                                        }
                                        else
                                        {
                                            AddHistoryItem(LocalGeneratedWaves, SelectedPrompt, EOptionTabType::PromptBooster, true, TEXT(""), TEXT(""), nullptr, LastUserPromptText);
                                        }
                                    }
                                }));
                        }
                    }
                    else
                    {
                        // Mono processing
                        TArray<TObjectPtr<USoundWave>> LocalGeneratedWaves;
                        for (const FString& Base64String : AudioBase64Array)
                        {
                            USoundWave* NewSoundWave = FAudioUtils::CreateSoundWaveFromBase64(Base64String);
                            if (NewSoundWave)
                            {
                                LocalGeneratedWaves.Add(NewSoundWave);
                            }
                        }

                        if (LocalGeneratedWaves.Num() == 0)
                        {
                            CompletedPromptIndices.Remove(Index);
                            if (PromptSuggestionsContainer.IsValid())
                            {
                                PromptSuggestionsContainer->Invalidate(EInvalidateWidget::Layout);
                            }
                            VarcoSoundToast::ShowError(FText::FromString(TEXT("Creation failed: server error or empty result.")));
                        }
                        else
                        {
                            AddHistoryItem(LocalGeneratedWaves, SelectedPrompt, EOptionTabType::PromptBooster, false, TEXT(""), TEXT(""), nullptr, LastUserPromptText);
                        }
                    }
                }
            ));
        }
        else
        {
            // If API client is not initialized, revert completion/loading state and exit
            CompletedPromptIndices.Remove(Index);
            LoadingPromptIndices.Remove(Index);
            if (PromptSuggestionsContainer.IsValid())
            {
                PromptSuggestionsContainer->Invalidate(EInvalidateWidget::Layout);
            }
            VarcoSoundToast::ShowError(FText::FromString(TEXT("API client is not initialized.")));
        }
        
        // Invalidate prompt suggestion container to update UI
        if (PromptSuggestionsContainer.IsValid())
        {
            PromptSuggestionsContainer->Invalidate(EInvalidateWidget::Layout);
        }
    }
    return FReply::Handled();
}

FReply SGeneratorTab::OnPromptSuggestionMouseUp(const FGeometry& Geometry, const FPointerEvent& MouseEvent, int32 Index)
{
    if (MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
    {
        return OnPromptSuggestionClicked(Index);
    }
    return FReply::Unhandled();
}

void SGeneratorTab::OnPromptSuggestionHovered(int32 Index)
{
    HoveredPromptIndex = Index;
    if (PromptSuggestionsContainer.IsValid())
    {
        PromptSuggestionsContainer->Invalidate(EInvalidateWidget::Layout);
    }
}

void SGeneratorTab::OnPromptSuggestionUnhovered()
{
    HoveredPromptIndex = -1;
    if (PromptSuggestionsContainer.IsValid())
    {
        PromptSuggestionsContainer->Invalidate(EInvalidateWidget::Layout);
    }
}

void SGeneratorTab::OnImageAssetSelected(const FAssetData& AssetData)
{
    UTexture2D* SelectedTexture = Cast<UTexture2D>(AssetData.GetAsset());
    if (SelectedTexture && IsValid(SelectedTexture))
    {
        // Release and remove captured preview texture (when user replaces with asset)
        if (CapturedPreviewTexture.IsValid())
        {
            if (UTexture2D* Tex = CapturedPreviewTexture.Get())
            {
                if (Tex->IsRooted()) { Tex->RemoveFromRoot(); }
            }
            CapturedPreviewTexture.Reset();
        }
        InputTexture = SelectedTexture;
        UpdateImagePreview();
    }
    else
    {
        InputTexture = nullptr;
        UpdateImagePreview();
    }
}

FString SGeneratorTab::GetCurrentImageAssetPath() const
{
	// Defense code for binding call during live coding
	if (!this) { return FString(); }
	UTexture2D* LocalTexture = InputTexture;
	if (!LocalTexture) { return FString(); }
	if (!IsValid(LocalTexture)) { return FString(); }
	return LocalTexture->GetPathName();
}

void SGeneratorTab::UpdateImagePreview()
{
    FImagePreviewHelpers::UpdateImagePreview(InputTexture, ImagePreviewBrush, ImagePreviewWidget, ImagePreviewFixedHeight);
    
    // Cleanup temporary texture if input is null
    if (!InputTexture || !IsValid(InputTexture))
    {
        FImagePreviewHelpers::CleanupCapturedTexture(CapturedPreviewTexture);
    }
}

FOptionalSize SGeneratorTab::GetImagePreviewHeight() const
{
    return FImagePreviewHelpers::GetImagePreviewHeight(ImagePreviewFixedHeight);
}

FOptionalSize SGeneratorTab::GetImagePreviewWidth() const
{
    return FImagePreviewHelpers::GetImagePreviewWidth(ImagePreviewBrush, ImagePreviewFixedHeight);
}

bool SGeneratorTab::ConvertImageAssetToBase64(FString& OutBase64) const
{
    OutBase64.Empty();
    if (!InputTexture || !IsValid(InputTexture))
    {
        return false;
    }

    FString SourceFilePath;
    if (InputTexture->AssetImportData)
    {
        SourceFilePath = InputTexture->AssetImportData->GetFirstFilename();
    }

    if (!SourceFilePath.IsEmpty() && FPaths::FileExists(SourceFilePath))
    {
        // Check file size first
        IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
        int64 FileSize = PlatformFile.FileSize(*SourceFilePath);
        
        if (FileSize > 50 * 1024 * 1024) // 50 MB
        {
            VarcoSoundToast::ShowError(FText::Format(
                NSLOCTEXT("VarcoSound", "ImageTooLarge", 
                    "Image is too large ({0} MB). Maximum size is 50 MB."),
                FText::AsNumber(FileSize / 1024.0 / 1024.0)));
            UE_LOG(LogTemp, Error, TEXT("Image too large: %.2f MB"), FileSize / 1024.0 / 1024.0);
            return false;
        }
        
        TArray<uint8> FileData;
        if (FFileHelper::LoadFileToArray(FileData, *SourceFilePath))
        {
            OutBase64 = FBase64::Encode(FileData);
            return true;
        }
    }

    VarcoSoundToast::ShowError(NSLOCTEXT("VarcoSound", "ImagePathNotFound", "Could not find image source file path or loading failed."));
    UE_LOG(LogTemp, Error, TEXT("Could not find image source file path or loading failed."));
    return false;
}

void SGeneratorTab::OnImage2SfxApiResponse(const FString& ResponseContent)
{
    // Print response to log
    UE_LOG(LogTemp, Warning, TEXT("Image2Sfx response: %s"), *ResponseContent);
    
    // JSON parsing and UI update
    TArray<FImage2SfxLayer> ParsedLayers;
    if (ParseImage2SfxResponse(ResponseContent, ParsedLayers))
    {
        CurrentImage2SfxLayers = ParsedLayers;
        CompletedImageLayerIndices.Empty(); // Initialize completed state when new layer list is received
        SelectedImage2SfxLayerIndex = -1; // Initialize selection when new response is received
        HoveredImage2SfxLayerIndex = -1; // Initialize hovering state when new response is received
        UpdateImage2SfxLayersUI();
    }
    	else
	{
		UE_LOG(LogTemp, Error, TEXT("Image2Sfx response parsing failed"));
		VarcoSoundToast::ShowError(FText::FromString(TEXT("Image2Sfx parsing failed: response format error")));
	}

    // End loading
    SetImage2SfxLoading(false);
}

bool SGeneratorTab::ParseImage2SfxResponse(const FString& ResponseContent, TArray<FImage2SfxLayer>& OutLayers)
{
    OutLayers.Empty();
    
    auto ParseLayersFromArray = [&OutLayers](const TArray<TSharedPtr<FJsonValue>>& LayersArray) -> bool
    {
        for (const TSharedPtr<FJsonValue>& LayerValue : LayersArray)
        {
            const TSharedPtr<FJsonObject>* LayerObject;
            if (LayerValue->TryGetObject(LayerObject) && LayerObject && LayerObject->IsValid())
            {
                FImage2SfxLayer Layer;
                (*LayerObject)->TryGetStringField(TEXT("category"), Layer.Category);
                (*LayerObject)->TryGetStringField(TEXT("name"), Layer.Name);
                (*LayerObject)->TryGetStringField(TEXT("description"), Layer.Description);
                (*LayerObject)->TryGetStringField(TEXT("prompt"), Layer.Prompt);

                OutLayers.Add(Layer);
            }
        }
        return OutLayers.Num() > 0;
    };
    
    // Remove markdown code block
    FString CleanedContent = RemoveMarkdownCodeBlock(ResponseContent);
    UE_LOG(LogTemp, Warning, TEXT("Step 1 - Cleaned response content: %s"), *CleanedContent);
    
    // Method 0: check if top level JSON is array (new API)
    {
        TArray<TSharedPtr<FJsonValue>> DirectArray;
        TSharedRef<TJsonReader<>> DirectReader = TJsonReaderFactory<>::Create(CleanedContent);
        if (FJsonSerializer::Deserialize(DirectReader, DirectArray) && DirectArray.Num() > 0)
        {
            if (ParseLayersFromArray(DirectArray))
            {
                UE_LOG(LogTemp, Warning, TEXT("Image2Sfx direct array parsing success: %d layers"), OutLayers.Num());
                return true;
            }
        }
    }
    
    FString ResultString;
    
    // Method 1: try normal JSON parsing
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(CleanedContent);
    
    if (FJsonSerializer::Deserialize(Reader, JsonObject) && JsonObject.IsValid())
    {
        UE_LOG(LogTemp, Warning, TEXT("2nd step - external JSON parsing success"));
        if (JsonObject->TryGetStringField(TEXT("result"), ResultString))
        {
            UE_LOG(LogTemp, Warning, TEXT("3rd step - result field extraction success"));
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("result field not found"));
            return false;
        }
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("External JSON parsing failed, try alternative method"));
        
        // Method 2: extract directly from "result":" pattern (simplified method)
        int32 ResultStartIndex = CleanedContent.Find(TEXT("\"result\":\""));
        if (ResultStartIndex != INDEX_NONE)
        {
            ResultStartIndex += 10; // "result":"" length
            
            // Find end point - find last "}"
            int32 ResultEndIndex = CleanedContent.Find(TEXT("\"}"), ESearchCase::CaseSensitive, ESearchDir::FromEnd);
            if (ResultEndIndex == INDEX_NONE)
            {
                // \"} pattern not found, find last "
                ResultEndIndex = CleanedContent.Find(TEXT("\""), ESearchCase::CaseSensitive, ESearchDir::FromEnd);
                if (ResultEndIndex <= ResultStartIndex)
                {
                    ResultEndIndex = CleanedContent.Len() - 1;
                }
            }
            
            if (ResultEndIndex > ResultStartIndex)
            {
                ResultString = CleanedContent.Mid(ResultStartIndex, ResultEndIndex - ResultStartIndex);
                UE_LOG(LogTemp, Warning, TEXT("Directly extracted result: %s"), *ResultString);
            }
            else
            {
                UE_LOG(LogTemp, Error, TEXT("result end point not found"));
                return false;
            }
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("result field not found"));
            return false;
        }
    }
    
    // result string整理
    ResultString = RemoveMarkdownCodeBlock(ResultString);
    UE_LOG(LogTemp, Warning, TEXT("Cleaned result string: %s"), *ResultString);
    
    // result internal JSON parsing
    TSharedPtr<FJsonObject> ResultJsonObject;
    TSharedRef<TJsonReader<>> ResultReader = TJsonReaderFactory<>::Create(ResultString);
    
    if (!FJsonSerializer::Deserialize(ResultReader, ResultJsonObject) || !ResultJsonObject.IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("result JSON parsing failed: %s"), *ResultString);
        return false;
    }
    UE_LOG(LogTemp, Warning, TEXT("4th step - result JSON parsing success"));
    
    // find layers array (supports multiple formats)
    const TArray<TSharedPtr<FJsonValue>>* LayersArray = nullptr;
    
    // Format 1: direct layers array
    if (ResultJsonObject->TryGetArrayField(TEXT("layers"), LayersArray))
    {
        UE_LOG(LogTemp, Warning, TEXT("Direct layers array format detected"));
    }
    // Format 2: output_format.layers structure
    else
    {
        const TSharedPtr<FJsonObject>* OutputFormatPtr;
        if (ResultJsonObject->TryGetObjectField(TEXT("output_format"), OutputFormatPtr) &&
            (*OutputFormatPtr)->TryGetArrayField(TEXT("layers"), LayersArray))
        {
            UE_LOG(LogTemp, Warning, TEXT("output_format.layers format detected"));
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("layers array not found"));
            return false;
        }
    }
     
    if (LayersArray && ParseLayersFromArray(*LayersArray))
    {
        UE_LOG(LogTemp, Warning, TEXT("Image2Sfx parsing complete: %d layers"), OutLayers.Num());
        return true;
    }
    
    UE_LOG(LogTemp, Error, TEXT("Image2Sfx layer parsing failed"));
    return false;
}

FString SGeneratorTab::RemoveMarkdownCodeBlock(const FString& Content)
{
    FString Result = Content;
    
    // Remove markdown code block starting with ```json
    if (Result.StartsWith(TEXT("```json")) || Result.StartsWith(TEXT("```")))
    {
        int32 FirstNewlineIndex = Result.Find(TEXT("\n"));
        if (FirstNewlineIndex != INDEX_NONE)
        {
            Result = Result.RightChop(FirstNewlineIndex + 1);
        }
        else
        {
            // If there is no line break, remove only the ``` part
            if (Result.StartsWith(TEXT("```json")))
            {
                Result = Result.RightChop(7); // "```json" length
            }
            else if (Result.StartsWith(TEXT("```")))
            {
                Result = Result.RightChop(3); // "```" length
            }
        }
    }
    
    // Remove the end ```
    if (Result.EndsWith(TEXT("```")))
    {
        Result = Result.LeftChop(3);
    }
    
    // JSON escape character processing (after markdown removal)
    Result = Result.Replace(TEXT("\\n"), TEXT("\n"));
    Result = Result.Replace(TEXT("\\\""), TEXT("\""));
    Result = Result.Replace(TEXT("\\\\"), TEXT("\\"));
    
    // Remove additional whitespace
    return Result.TrimStartAndEnd();
}

void SGeneratorTab::UpdateImage2SfxLayersUI()
{
    if (!Image2SfxLayersScrollBox.IsValid())
    {
        return;
    }
    
    Image2SfxLayersScrollBox->ClearChildren();
    
    if (bIsImage2SfxLoading)
    {
        Image2SfxLayersScrollBox->AddSlot()
        .Padding(0, 5)
        [
            SNew(SHorizontalBox)
            + SHorizontalBox::Slot()
            .AutoWidth()
            .VAlign(VAlign_Center)
            [
                SNew(SThrobber)
            ]
            + SHorizontalBox::Slot()
            .AutoWidth()
            .Padding(8,0)
            .VAlign(VAlign_Center)
            [
                SNew(STextBlock)
                .Text(FText::FromString(TEXT("Loading layers...")))
                .Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
                .ColorAndOpacity(FLinearColor(0.8f, 0.8f, 0.8f, 1.0f))
            ]
        ];
    }
    else if (CurrentImage2SfxLayers.Num() > 0)
    {
        // Add title
        Image2SfxLayersScrollBox->AddSlot()
        .Padding(0, 5)
        [
            SNew(STextBlock)
            .Text(FText::FromString(TEXT("🎵 AI Generated Sound Layers:")))
            .Font(FCoreStyle::GetDefaultFontStyle("Bold", 9))
            .ColorAndOpacity(FLinearColor(0.8f, 0.8f, 0.8f, 1.0f))
        ];
        
        // Display each layer as a button
        for (int32 Index = 0; Index < CurrentImage2SfxLayers.Num(); ++Index)
        {
            const FImage2SfxLayer& Layer = CurrentImage2SfxLayers[Index];
            
            Image2SfxLayersScrollBox->AddSlot()
            .Padding(0, 2)
            [
                SNew(SButton)
                .OnClicked(this, &SGeneratorTab::OnImage2SfxLayerClicked, Index)
                .ButtonStyle(FCoreStyle::Get(), "FlatButton")
                .ButtonColorAndOpacity(this, &SGeneratorTab::GetImage2SfxLayerButtonColor, Index)
                .ForegroundColor(FLinearColor::White)
                .HAlign(HAlign_Fill)
                .ContentPadding(FMargin(8, 6))
                .OnHovered(this, &SGeneratorTab::OnImage2SfxLayerHovered, Index)
                .OnUnhovered(this, &SGeneratorTab::OnImage2SfxLayerUnhovered)
                [
                    SNew(SHorizontalBox)
                    
                    // Text block (name/description)
                    + SHorizontalBox::Slot()
                    .FillWidth(1.0f)
                    .VAlign(VAlign_Center)
                [
                    SNew(SVerticalBox)
                    
                    // Layer name and category
                    + SVerticalBox::Slot()
                    .AutoHeight()
                        [
                            SNew(STextBlock)
                            .Text_Lambda([this, Index]() -> FText
                            {
                                if (CurrentImage2SfxLayers.IsValidIndex(Index))
                                {
                                    const FImage2SfxLayer& Layer = CurrentImage2SfxLayers[Index];
                                    const bool bCompleted = CompletedImageLayerIndices.Contains(Index);
                                    FString Prefix = bCompleted ? TEXT("✔️ ") : TEXT("");
                                    FString DisplayText = FString::Printf(TEXT("%s%s [%s]"),
                                        *Prefix,
                                        *Layer.Name,
                                        *Layer.Category);
                                    return FText::FromString(DisplayText);
                                }
                                return FText::GetEmpty();
                            })
                            .Font(FCoreStyle::GetDefaultFontStyle("Bold", 9))
                            .ColorAndOpacity(FLinearColor::White)
                    ]
                    
                    // Description
                    + SVerticalBox::Slot()
                    .AutoHeight()
                    .Padding(0, 2, 0, 0)
                    [
                        SNew(STextBlock)
                        .Text_Lambda([this, Index]() -> FText
                        {
                            if (CurrentImage2SfxLayers.IsValidIndex(Index))
                            {
                                return FText::FromString(CurrentImage2SfxLayers[Index].Description);
                            }
                            return FText::GetEmpty();
                        })
                        .Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
                        .ColorAndOpacity(FLinearColor(0.8f, 0.8f, 0.8f, 1.0f))
                        .AutoWrapText(true)
                        ]
                    ]

                    // Loading spinner (right side of item)
                    + SHorizontalBox::Slot()
                    .AutoWidth()
                    .VAlign(VAlign_Center)
                    .HAlign(HAlign_Right)
                    .Padding(8,0,0,0)
                    [
                        SNew(SBox)
                        .WidthOverride(16)
                        .HeightOverride(16)
                        .Visibility_Lambda([this, Index]() -> EVisibility
                        {
                            return LoadingImageLayerIndices.Contains(Index) ? EVisibility::Visible : EVisibility::Collapsed;
                        })
                        [
                            SNew(SThrobber)
                            .NumPieces(1)
                            .Animate(SThrobber::Horizontal)
                        ]
                    ]
                ]
            ];
        }
    }
    
    // Invalidate container to update visibility
    Image2SfxLayersScrollBox->Invalidate(EInvalidateWidget::Layout);
}

FReply SGeneratorTab::OnImage2SfxLayerClicked(int32 Index)
{
    if (CurrentImage2SfxLayers.IsValidIndex(Index))
    {
        // Block duplicate requests for already completed layers
        if (CompletedImageLayerIndices.Contains(Index))
        {
            return FReply::Handled();
        }

        // Update selected layer index
        SelectedImage2SfxLayerIndex = Index;
        
        const FImage2SfxLayer& SelectedLayer = CurrentImage2SfxLayers[Index];
        const FString LayerNameForHist = SelectedLayer.Name;
        const FString LayerCategoryForHist = SelectedLayer.Category;
        
        // Call creation API using the Description of the selected layer as the prompt
        if (ApiClient.IsValid())
        {
            GeneratedSoundWaves.Empty();
            const FString GeneratePrompt = SelectedLayer.Description.IsEmpty() ? SelectedLayer.Prompt : SelectedLayer.Description;
            bIsImagePrompterGenerating = true;

            // Update request ID and save
            ImageLayerRequestIdCounter++;
            ImageLayerRequestId.Add(Index, ImageLayerRequestIdCounter);

            // Update completed state (even if only attempted) and UI update
            CompletedImageLayerIndices.Add(Index);
            LoadingImageLayerIndices.Add(Index);
            LastGenRequestTab = EOptionTabType::ImagePrompter;
            const int32 RequestIdForThisCall = ImageLayerRequestIdCounter;
            ApiClient->SendGenerateRequest(GeneratePrompt, NumSamplesValue, FOnGenApiResponse::CreateLambda(
                [this, Index, RequestIdForThisCall, GeneratePrompt, LayerNameForHist, LayerCategoryForHist](const TArray<FString>& AudioBase64Array)
                {
                    // Ignore old response
                    if (!ImageLayerRequestId.Contains(Index) || ImageLayerRequestId[Index] != RequestIdForThisCall)
                    {
                        return;
                    }

                    // Remove loading state
                    LoadingImageLayerIndices.Remove(Index);

                    GeneratedSoundWaves.Empty();

                    if (AudioBase64Array.Num() == 0)
                    {
                        CompletedImageLayerIndices.Remove(Index);
                        if (Image2SfxLayersScrollBox.IsValid())
                        {
                            Image2SfxLayersScrollBox->Invalidate(EInvalidateWidget::Layout);
                        }
                        VarcoSoundToast::ShowError(FText::FromString(TEXT("Creation failed: server error or empty result")));
                        return;
                    }

                    if (bStereoEnabled)
                    {
                        int32 TotalCount = AudioBase64Array.Num();
                        TSharedPtr<TArray<FString>> ConvertedArray = MakeShared<TArray<FString>>();
                        TSharedPtr<int32> CompletedCount = MakeShared<int32>(0);

                        for (const FString& Base64String : AudioBase64Array)
                        {
                            ApiClient->SendMono2StereoRequest(Base64String, FOnMono2StereoApiResponse::CreateLambda(
                                [this, ConvertedArray, CompletedCount, TotalCount, Index, RequestIdForThisCall, GeneratePrompt, LayerNameForHist, LayerCategoryForHist](const TArray<FString>& StereoResult)
                                {
                                    // Ignore old response
                                    if (!ImageLayerRequestId.Contains(Index) || ImageLayerRequestId[Index] != RequestIdForThisCall)
                                    {
                                        return;
                                    }

                                    if (StereoResult.Num() > 0)
                                    {
                                        ConvertedArray->Add(StereoResult[0]);
                                    }

                                    (*CompletedCount)++;

                                    if (*CompletedCount >= TotalCount)
                                    {
                                        TArray<TObjectPtr<USoundWave>> LocalGeneratedWaves;
                                        for (const FString& ConvertedBase64 : *ConvertedArray)
                                        {
                                            USoundWave* NewSoundWave = FAudioUtils::CreateSoundWaveFromBase64(ConvertedBase64);
                                            if (NewSoundWave)
                                            {
                                                LocalGeneratedWaves.Add(NewSoundWave);
                                            }
                                        }

                                        if (LocalGeneratedWaves.Num() == 0)
                                        {
                                            CompletedImageLayerIndices.Remove(Index);
                                            if (Image2SfxLayersScrollBox.IsValid())
                                            {
                                                Image2SfxLayersScrollBox->Invalidate(EInvalidateWidget::Layout);
                                            }
                                            VarcoSoundToast::ShowError(FText::FromString(TEXT("Stereo conversion failed")));
                                        }
                                        else
                                        {
                                            AddHistoryItem(LocalGeneratedWaves, GeneratePrompt, EOptionTabType::ImagePrompter, true, LayerNameForHist, LayerCategoryForHist);
                                        }
                                    }
                                }));
                        }
                    }
                    else
                    {
                        TArray<TObjectPtr<USoundWave>> LocalGeneratedWaves;
                        for (const FString& Base64String : AudioBase64Array)
                        {
                            USoundWave* NewSoundWave = FAudioUtils::CreateSoundWaveFromBase64(Base64String);
                            if (NewSoundWave)
                            {
                                LocalGeneratedWaves.Add(NewSoundWave);
                            }
                        }

                        if (LocalGeneratedWaves.Num() == 0)
                        {
                            CompletedImageLayerIndices.Remove(Index);
                            if (Image2SfxLayersScrollBox.IsValid())
                            {
                                Image2SfxLayersScrollBox->Invalidate(EInvalidateWidget::Layout);
                            }
                            VarcoSoundToast::ShowError(FText::FromString(TEXT("Creation failed: server error or empty result")));
                        }
                        else
                        {
                            AddHistoryItem(LocalGeneratedWaves, GeneratePrompt, EOptionTabType::ImagePrompter, false, LayerNameForHist, LayerCategoryForHist);
                        }
                    }

                    // Release loading & UI update
                    if (ImagePrompterLoadingContainer.IsValid())
                    {
                        ImagePrompterLoadingContainer->Invalidate(EInvalidateWidget::Layout);
                    }
                }
            ));
        }
        else
        {
            // If API client is not initialized, revert completion/loading state and exit
            CompletedImageLayerIndices.Remove(Index);
            LoadingImageLayerIndices.Remove(Index);
            if (Image2SfxLayersScrollBox.IsValid())
            {
                Image2SfxLayersScrollBox->Invalidate(EInvalidateWidget::Layout);
            }
            VarcoSoundToast::ShowError(FText::FromString(TEXT("API client is not initialized")));
        }
        
        // Invalidate scroll box to update UI
        if (Image2SfxLayersScrollBox.IsValid())
        {
            Image2SfxLayersScrollBox->Invalidate(EInvalidateWidget::Layout);
        }
    }
    return FReply::Handled();
}

FSlateColor SGeneratorTab::GetImage2SfxLayerButtonColor(int32 Index) const
{
    if (CompletedImageLayerIndices.Contains(Index))
    {
        return FLinearColor(0.15f, 0.55f, 0.25f, 1.0f); // Completed layers are green series
    }
    if (SelectedImage2SfxLayerIndex == Index)
    {
        return FLinearColor(0.15f, 0.65f, 0.3f, 1.0f); // Selected layers are green background
    }
    else if (HoveredImage2SfxLayerIndex == Index)
    {
        return FLinearColor(0.1f, 0.2f, 0.4f, 1.0f); // Hovered layers are blue background
    }
    return FLinearColor(0.4f, 0.4f, 0.4f, 1.0f); // Default gray background
}

void SGeneratorTab::OnImage2SfxLayerHovered(int32 Index)
{
    HoveredImage2SfxLayerIndex = Index;
    if (Image2SfxLayersScrollBox.IsValid())
    {
        Image2SfxLayersScrollBox->Invalidate(EInvalidateWidget::Layout);
    }
}

void SGeneratorTab::OnImage2SfxLayerUnhovered()
{
    HoveredImage2SfxLayerIndex = -1;
    if (Image2SfxLayersScrollBox.IsValid())
    {
        Image2SfxLayersScrollBox->Invalidate(EInvalidateWidget::Layout);
    }
}

void SGeneratorTab::PauseAllPlayback()
{
	if (AudioResultView_PromptBooster.IsValid()) { AudioResultView_PromptBooster->Pause(); }
	if (AudioResultView_MultiLayering.IsValid()) { AudioResultView_MultiLayering->Pause(); }
	if (AudioResultView_ImagePrompter.IsValid()) { AudioResultView_ImagePrompter->Pause(); }
}

void SGeneratorTab::SetApiKey(const FString& InApiKey)
{
	if (ApiClient.IsValid())
	{
		ApiClient->SetApiKey(InApiKey);
	}
}

TSharedPtr<SAudioResultView> SGeneratorTab::GetResultView(EOptionTabType Tab) const
{
	switch (Tab)
	{
		case EOptionTabType::PromptBooster:
			return AudioResultView_PromptBooster;
		case EOptionTabType::MultiLayering:
			return AudioResultView_MultiLayering;
		case EOptionTabType::ImagePrompter:
			return AudioResultView_ImagePrompter;
		default:
			return AudioResultView_PromptBooster;
	}
}

bool SGeneratorTab::ShouldShowOutputSection() const
{
    // PromptBooster now only uses history, so hide the main result section.
    if (CurrentSelectedTab == EOptionTabType::PromptBooster)
    {
        return false;
    }

    if (CurrentSelectedTab == EOptionTabType::MultiLayering)
    {
        // Always show unimplemented guidance text
        return true;
    }
    TSharedPtr<SAudioResultView> View = GetResultView(CurrentSelectedTab);
    return View.IsValid() && (View->IsLoading() || View->HasSoundWaves());
}

bool SGeneratorTab::ShouldShowPromptBoosterSeparator() const
{
    return CurrentSelectedTab == EOptionTabType::PromptBooster && CurrentPromptSuggestions.Num() > 0;
}

END_SLATE_FUNCTION_BUILD_OPTIMIZATION 
 
void SGeneratorTab::InitImageSourceOptions()
{
    ImageSourceOptions.Empty();
    ImageSourceOptions.Add(MakeShared<FString>(TEXT("Content Browser")));
    ImageSourceOptions.Add(MakeShared<FString>(TEXT("Viewport")));
    ImageSourceOptions.Add(MakeShared<FString>(TEXT("Selected Actors")));

    CurrentImageSourceType = EImageSourceType::ContentBrowser;
    if (ImageSourceCombo.IsValid())
    {
        ImageSourceCombo->RefreshOptions();
        ImageSourceCombo->SetSelectedItem(ImageSourceOptions[0]);
    }
}

void SGeneratorTab::OnImageSourceChanged(TSharedPtr<FString> NewSelection, ESelectInfo::Type SelectInfo)
{
    if (!NewSelection.IsValid()) { return; }
    const FString& Sel = *NewSelection.Get();
    if (Sel == TEXT("Viewport"))
    {
        CurrentImageSourceType = EImageSourceType::Viewport;
    }
    else if (Sel == TEXT("Selected Actors"))
    {
        CurrentImageSourceType = EImageSourceType::SelectedActors;
    }
    else
    {
        CurrentImageSourceType = EImageSourceType::ContentBrowser;
    }
    if (ImagePrompterContainer.IsValid())
    {
        ImagePrompterContainer->Invalidate(EInvalidateWidget::Layout);
    }
}

FReply SGeneratorTab::OnCaptureButtonClicked()
{
#if WITH_EDITOR
    TArray<uint8> Png;
    bool bOk = false;
    if (CurrentImageSourceType == EImageSourceType::Viewport)
    {
        FViewportCaptureOptions Opts;
        bOk = CaptureEditorViewportToPng(Opts, Png);
    }
    else if (CurrentImageSourceType == EImageSourceType::SelectedActors)
    {
        FActorCaptureOptions Opts;
        bOk = CaptureSelectedActorsToPng(Opts, Png);
    }
    if (bOk)
    {
        UpdateImagePreviewFromPng(Png);
        PendingCapturedPng = Png;
        bHasPendingCapturedImage = true;
        return FReply::Handled();
    }
#endif
    VarcoSoundToast::ShowError(FText::FromString(TEXT("Capture failed or no selected actors")));
    return FReply::Handled();
}

bool SGeneratorTab::UpdateImagePreviewFromPng(const TArray<uint8>& PngBytes)
{
    return FImagePreviewHelpers::UpdateImagePreviewFromPng(
        PngBytes,
        CapturedPreviewTexture,
        InputTexture,
        ImagePreviewBrush,
        ImagePreviewWidget,
        ImagePreviewFixedHeight);
}

void SGeneratorTab::RequestImage2SfxWithBase64(const FString& Prompt, const FString& ImageBase64)
{
    if (!ApiClient.IsValid()) { return; }
    if (auto View = GetResultView(EOptionTabType::ImagePrompter))
    {
        View->SetLoading(false);
        View->SetSoundWaves(TArray<USoundWave*>());
        View->SetPromptText(Prompt);
    }
    SetImage2SfxLoading(true);
    ApiClient->SendImage2SfxRequest(Prompt, ImageBase64, FOnImage2SfxApiResponse::CreateSP(this, &SGeneratorTab::OnImage2SfxApiResponse));
}

FText SGeneratorTab::GetCaptureButtonText() const
{
    switch (CurrentImageSourceType)
    {
    case EImageSourceType::Viewport: return FText::FromString(TEXT("Capture Viewport"));
    case EImageSourceType::SelectedActors: return FText::FromString(TEXT("Capture Selected"));
    default: return FText::FromString(TEXT("Capture"));
    }
}

EVisibility SGeneratorTab::GetCaptureButtonVisibility() const
{
    return (CurrentImageSourceType == EImageSourceType::Viewport || CurrentImageSourceType == EImageSourceType::SelectedActors)
        ? EVisibility::Visible
        : EVisibility::Collapsed;
}

EVisibility SGeneratorTab::GetImageAssetPickerVisibility() const
{
    return (CurrentImageSourceType == EImageSourceType::ContentBrowser) ? EVisibility::Visible : EVisibility::Collapsed;
}

void SGeneratorTab::SetImage2SfxLoading(bool bInLoading)
{
    bIsImage2SfxLoading = bInLoading;
    if (!Image2SfxLayersScrollBox.IsValid()) { return; }
    UpdateImage2SfxLayersUI();
}

void SGeneratorTab::AddHistoryItem(const TArray<TObjectPtr<USoundWave>>& NewSoundWaves, const FString& Prompt, EOptionTabType TabType, bool bStereo, const FString& LayerName, const FString& LayerCategory, TSharedPtr<FSlateBrush> Thumbnail, const FString& UserInputPrompt)
{
    const int32 MaxHistoryCount = 30; // Maximum history save count (memory management)

    // Initialize latest flag: turn off latest display for existing items before adding new item
    for (auto& Item : HistoryData)
    {
        if (Item.IsValid())
        {
            Item->bIsLatest = false;
            Item->HighlightColor = FLinearColor::Transparent;
        }
    }

    TSharedPtr<FGenerationHistoryItem> NewItem = MakeShared<FGenerationHistoryItem>();
    NewItem->Timestamp = FDateTime::Now();
    NewItem->SoundWaves = NewSoundWaves;
    NewItem->FinalPrompt = Prompt;
    NewItem->SourceTab = TabType;
    NewItem->UserInputPrompt = UserInputPrompt;
    NewItem->LayerName = LayerName;
    NewItem->LayerCategory = LayerCategory;
    NewItem->ImageThumbnail = Thumbnail;
    NewItem->bWasStereo = bStereo;
    NewItem->bIsExpanded = true; // Latest item is expanded
    NewItem->bIsLatest = true;
    NewItem->HighlightColor = FLinearColor(0.3f, 1.0f, 0.6f, 1.0f); // Green series
    
    // 1. Add to data array (latest first)
    HistoryData.Insert(NewItem, 0);
    
    // 2. Remove old data if maximum count is exceeded (Queue method)
    if (HistoryData.Num() > MaxHistoryCount)
    {
        HistoryData.RemoveAt(HistoryData.Num() - 1);
    }
    
    if (HistoryListContainer.IsValid())
    {
        TSharedPtr<SAudioResultView> HistoryResultView;
        TSharedPtr<SVerticalBox> BodyContainer;
        
        // 1. Pre-create body container (for lambda capture)
        SAssignNew(BodyContainer, SVerticalBox)
        .Visibility(EVisibility::Collapsed) // Default: collapsed
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(0, 5)
        [
            SAssignNew(HistoryResultView, SAudioResultView)
        ];
        
        // 2. Set result view data
        if (HistoryResultView.IsValid())
        {
            TArray<USoundWave*> SoundWavesPtrs;
            for (auto& WeakPtr : NewSoundWaves)
            {
                if (WeakPtr) SoundWavesPtrs.Add(WeakPtr);
            }
			HistoryResultView->SetSourceTag(TEXT("Base64"));
            HistoryResultView->SetSoundWaves(SoundWavesPtrs);
            HistoryResultView->SetPromptText(Prompt);
            HistoryResultView->SetTabType(EAudioResultViewTabType::Generator);

            // Refresh (Regenerate) function binding
            HistoryResultView->OnRegenerateRequested.BindLambda([this, Prompt, TabType, NewItem, HistoryResultView]()
            {
                if (!ApiClient.IsValid()) return;

                // 1. Transition to HistoryResultView loading state
                HistoryResultView->SetLoading(true);

                // 2. Re-request using current parameter values (user request: "use current settings", but Stereo uses history value)
                //    Prompt uses the prompt of the corresponding history item
                FString RefreshPrompt = NewItem->FinalPrompt;
                int32 CurrentNumSamples = NumSamplesValue; // Current UI setting value
                bool bUseStereo = NewItem->bWasStereo; // Use value at the time of history save

                // 3. Stereo processing logic branch
                if (bUseStereo)
                {
                    // Stereo logic: Generate -> Mono2Stereo -> Add
                    ApiClient->SendGenerateRequest(RefreshPrompt, CurrentNumSamples, FOnGenApiResponse::CreateLambda(
                        [this, NewItem, HistoryResultView](const TArray<FString>& AudioBase64Array)
                        {
                            // 각 결과에 대해 Mono2Stereo 요청
                            int32 TotalCount = AudioBase64Array.Num();
                            if (TotalCount == 0)
                            {
                                HistoryResultView->SetLoading(false);
                                VarcoSoundToast::ShowError(FText::FromString(TEXT("Regeneration failed: No audio returned")));
                                return;
                            }

                            TSharedPtr<TArray<FString>> ConvertedArray = MakeShared<TArray<FString>>();
                            TSharedPtr<int32> CompletedCount = MakeShared<int32>(0);

                            for (const FString& Base64String : AudioBase64Array)
                            {
                                ApiClient->SendMono2StereoRequest(Base64String, FOnMono2StereoApiResponse::CreateLambda(
                                    [this, NewItem, HistoryResultView, ConvertedArray, CompletedCount, TotalCount](const TArray<FString>& StereoResult)
                                    {
                                        if (StereoResult.Num() > 0)
                                        {
                                            ConvertedArray->Add(StereoResult[0]);
                                        }
                                        
                                        (*CompletedCount)++;
                                        
                                        // When all conversions are complete
                                        if (*CompletedCount >= TotalCount)
                                        {
                                            TArray<USoundWave*> NewWaves;
                                            for (const FString& ConvertedBase64 : *ConvertedArray)
                                            {
                                                USoundWave* Wave = FAudioUtils::CreateSoundWaveFromBase64(ConvertedBase64);
                                                if (Wave)
                                                {
                                                    NewWaves.Add(Wave);
                                                }
                                            }

                                            // Add to existing item
                                            for (USoundWave* Wave : NewWaves)
                                            {
                                                NewItem->SoundWaves.Add(Wave);
                                            }

                                            // Update UI
                                            TArray<USoundWave*> AllWaves;
                                            for (auto& WeakPtr : NewItem->SoundWaves)
                                            {
                                                if (WeakPtr) AllWaves.Add(WeakPtr);
                                            }

                                            HistoryResultView->SetLoading(false);
											HistoryResultView->SetSourceTag(TEXT("Base64"));
                                            HistoryResultView->SetSoundWaves(AllWaves);
                                            
                                            if (NewWaves.Num() > 0)
                                            {
                                                VarcoSoundToast::ShowInfo(FText::FromString(FString::Printf(TEXT("Added %d stereo variations"), NewWaves.Num())));
                                            }
                                            else
                                            {
                                                VarcoSoundToast::ShowError(FText::FromString(TEXT("Stereo conversion failed during regeneration")));
                                            }
                                        }
                                    }
                                ));
                            }
                        }
                    ));
                }
                else
                {
                    // Mono logic: Generate -> Add
                    ApiClient->SendGenerateRequest(RefreshPrompt, CurrentNumSamples, FOnGenApiResponse::CreateLambda(
                        [this, NewItem, HistoryResultView](const TArray<FString>& NewAudioBase64Array)
                        {
                            TArray<USoundWave*> NewWaves;
                            for (const FString& Base64 : NewAudioBase64Array)
                            {
                                USoundWave* Wave = FAudioUtils::CreateSoundWaveFromBase64(Base64);
                                if (Wave)
                                {
                                    NewWaves.Add(Wave);
                                }
                            }

                            // 4. Add to existing history item
                            for (USoundWave* Wave : NewWaves)
                            {
                                NewItem->SoundWaves.Add(Wave); // TObjectPtr automatic conversion
                            }

                            // 5. Update view
                            TArray<USoundWave*> AllWaves;
                            for (auto& WeakPtr : NewItem->SoundWaves)
                            {
                                if (WeakPtr) AllWaves.Add(WeakPtr);
                            }

                            HistoryResultView->SetLoading(false);
							HistoryResultView->SetSourceTag(TEXT("Base64"));
                            HistoryResultView->SetSoundWaves(AllWaves);
                            
                            // Success toast
                            if (NewWaves.Num() > 0)
                            {
                                VarcoSoundToast::ShowInfo(FText::FromString(FString::Printf(TEXT("Added %d variations"), NewWaves.Num())));
                            }
                            else
                            {
                                 VarcoSoundToast::ShowError(FText::FromString(TEXT("Regeneration failed")));
                            }
                        }
                    ));
                }
            });
        }
        
        // 3. SExpandableArea (standard collapse/expand widget)
        const bool bStartExpanded = NewItem->bIsExpanded;

        TSharedRef<SExpandableArea> ItemWidget = SNew(SExpandableArea)
        .InitiallyCollapsed(!bStartExpanded)
        .BorderImage(FAppStyle::GetBrush("DetailsView.CategoryTop")) // Detail panel header style
        .BodyBorderImage(FAppStyle::GetBrush("NoBrush")) // Remove body background (internal widget handles)
        .BodyBorderBackgroundColor(FLinearColor::Transparent)
        .HeaderPadding(FMargin(4.0f, 2.0f)) // Adjust header internal padding
        .Padding(0.0f) // Remove overall external padding
        .OnAreaExpansionChanged_Lambda([NewItem](bool bExpanded)
        {
            NewItem->bIsExpanded = bExpanded;
        })
        .HeaderContent()
        [
            SNew(SHorizontalBox)
            
            // 1. Prompt (title) - most prominently highlighted
            + SHorizontalBox::Slot()
            .FillWidth(1.0f)
            .VAlign(VAlign_Center)
            .Padding(0, 0, 10, 0)
            [
                SNew(STextBlock)
                .Text_Lambda([WeakItem = TWeakPtr<FGenerationHistoryItem>(NewItem)]() -> FText
                {
                    FString BasePrompt = TEXT("Generated Audio");
                    if (const TSharedPtr<FGenerationHistoryItem> Pinned = WeakItem.Pin())
                    {
                        BasePrompt = Pinned->FinalPrompt.IsEmpty() ? TEXT("Generated Audio") : Pinned->FinalPrompt;
                    }
                    const int32 MaxLen = 40;
                    if (BasePrompt.Len() > MaxLen)
                    {
                        BasePrompt = BasePrompt.Left(MaxLen) + TEXT("...");
                    }
                    return FText::FromString(BasePrompt);
                })
                .Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
                .ColorAndOpacity_Lambda([WeakItem = TWeakPtr<FGenerationHistoryItem>(NewItem)]()
                {
                    if (const TSharedPtr<FGenerationHistoryItem> Pinned = WeakItem.Pin())
                    {
                        return Pinned->bIsLatest ? Pinned->HighlightColor : FLinearColor::White;
                    }
                    return FLinearColor::White;
                })
                .ToolTipText(FText::FromString(Prompt)) // Provide tooltip for long text
                .AutoWrapText(false)
                .OverflowPolicy(ETextOverflowPolicy::Ellipsis) // Handle too long text
            ]

            // 2. Type/layer information (small display)
            + SHorizontalBox::Slot()
            .AutoWidth()
            .VAlign(VAlign_Center)
            .Padding(0, 0, 10, 0)
            [
                SNew(STextBlock)
                .Text_Lambda([TabType, LayerName, WeakItem = TWeakPtr<FGenerationHistoryItem>(NewItem)]()
                {
                    FString ModeText = (TabType == EOptionTabType::ImagePrompter) ? TEXT("Img2Sfx") : TEXT("Booster");

                    if (TabType == EOptionTabType::PromptBooster)
                    {
                        FString UserInput = TEXT("");
                        if (const TSharedPtr<FGenerationHistoryItem> Pinned = WeakItem.Pin())
                        {
                            UserInput = Pinned->UserInputPrompt;
                        }
                        const int32 MaxLen = 20;
                        if (UserInput.Len() > MaxLen)
                    {
                            UserInput = UserInput.Left(MaxLen) + TEXT("...");
                        }
                        return FText::FromString(FString::Printf(TEXT("[Booster: %s]"), *UserInput));
                    }

                    if (TabType == EOptionTabType::ImagePrompter && !LayerName.IsEmpty())
                    {
                        return FText::FromString(FString::Printf(TEXT("[Img2Sfx: %s]"), *LayerName));
                    }

                    return FText::FromString(FString::Printf(TEXT("[%s]"), *ModeText));
                })
                .Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
                .ColorAndOpacity(FLinearColor(0.7f, 0.7f, 0.7f))
            ]
            
            // 3. Timestamp (right end, smallest)
            + SHorizontalBox::Slot()
            .AutoWidth()
            .VAlign(VAlign_Center)
            [
                SNew(STextBlock)
                .Text(FText::FromString(NewItem->Timestamp.ToString(TEXT("%H:%M:%S")))) // Show only time to save space
                .ColorAndOpacity(FLinearColor(0.5f, 0.5f, 0.5f))
                .Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
            ]
        ]
        .BodyContent()
        [
             SNew(SBorder)
            .BorderImage(FAppStyle::GetBrush("NoBrush"))
            .BorderBackgroundColor_Lambda([WeakItem = TWeakPtr<FGenerationHistoryItem>(NewItem)]()
            {
                if (const TSharedPtr<FGenerationHistoryItem> Pinned = WeakItem.Pin())
                {
                    return Pinned->bIsLatest ? Pinned->HighlightColor.CopyWithNewOpacity(0.12f) : FLinearColor::Transparent;
                }
                return FLinearColor::Transparent;
            })
            .Padding(FMargin(0, 2, 0, 2))
        [
             SNew(SVerticalBox)
            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(0) // Remove space between body and waveform
            [
                BodyContainer.ToSharedRef()
                ]
            ]
        ];
        
        // Change result view visibility to always Visible (ExpandableArea controls)
        BodyContainer->SetVisibility(EVisibility::Visible);
        
        // Add slot at the top of the list (no gap)
        HistoryListContainer->InsertSlot(0)
        .Padding(0.0f) 
        [
            ItemWidget
        ];

        // 4. Remove oldest UI widget if maximum count is exceeded (last slot)
        if (HistoryListContainer->GetChildren()->Num() > MaxHistoryCount)
        {
            // SScrollBox requires widget reference when using RemoveSlot
            TSharedRef<SWidget> OldestWidget = HistoryListContainer->GetChildren()->GetChildAt(HistoryListContainer->GetChildren()->Num() - 1);
            HistoryListContainer->RemoveSlot(OldestWidget);
        }
    }
}