// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Engine/Engine.h"
#include "UObject/NoExportTypes.h"
#include "Types/SlateStructs.h"
#include "Types/SlateEnums.h"
#include "Sound/SoundWave.h"
#include "Engine/Texture2D.h"
#include "Tabs/IVarcoSoundTab.h"
#include "SGeneratorTab.generated.h"
class SObjectPropertyEntryBox;
class SImage;
class STextComboBox;
struct FAssetData;
template<typename NumericType> class SNumericEntryBox;

class SAudioResultView;

// Image source type
UENUM()
enum class EImageSourceType : uint8
{
    ContentBrowser,
    Viewport,
    SelectedActors
};

UENUM()
enum class EOptionTabType : uint8
{
    PromptBooster,
    MultiLayering,
    ImagePrompter
};

// History item structure (record of one generation)
struct FGenerationHistoryItem
{
    // Creation time
    FDateTime Timestamp;

    // Generated audio list
    TArray<TObjectPtr<USoundWave>> SoundWaves;

    // Used prompt
    FString FinalPrompt;

    // Generated tab mode
    EOptionTabType SourceTab;

    // (PromptBooster) User input original prompt (before AI suggestion)
    FString UserInputPrompt;

    // (Image2Sfx) Layer name
    FString LayerName;

    // (Image2Sfx) Layer category
    FString LayerCategory;

    // (Image2Sfx) Original image thumbnail (optional)
    TSharedPtr<FSlateBrush> ImageThumbnail;

    // Stereo setting at the time of creation (reproduce the same environment when refreshing)
    bool bWasStereo;

    // Waveform view expand state (default: true)
    bool bIsExpanded;

    // Latest creation status (for header/text highlighting)
    bool bIsLatest;

    // Highlight color (used when bIsLatest is true)
    FLinearColor HighlightColor;

    FGenerationHistoryItem()
        : Timestamp(FDateTime::Now())
        , SourceTab(EOptionTabType::PromptBooster)
        , UserInputPrompt(TEXT(""))
        , bWasStereo(false)
        , bIsExpanded(true)
        , bIsLatest(false)
        , HighlightColor(FLinearColor::Transparent)
    {}
};

// Image2Sfx layer information structure
USTRUCT()
struct FImage2SfxLayer
{
    GENERATED_BODY()

    UPROPERTY()
    FString Category;

    UPROPERTY()
    FString Name;

    UPROPERTY()
    FString Description;

    UPROPERTY()
    FString Prompt;

    FImage2SfxLayer()
        : Category(TEXT(""))
        , Name(TEXT(""))
        , Description(TEXT(""))
        , Prompt(TEXT(""))
    {}
};

// Prompt booster API response handling delegate definition
DECLARE_DELEGATE_OneParam(FOnPromptBoosterResponse, const TArray<FString>&);

class VARCOSOUND_API SGeneratorTab : public SCompoundWidget, public IVarcoSoundTab
{
public:
    SLATE_BEGIN_ARGS(SGeneratorTab) {}
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs);
    virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;
    ~SGeneratorTab();

	// IVarcoSoundTab interface
	virtual void PauseAllPlayback() override;
	virtual void SetApiKey(const FString& InApiKey) override;

private:
    FReply OnGenerateButtonClicked();
    void OnApiResponse(const TArray<FString>& AudioBase64Array);
    FReply OnLoadButtonClicked();
    void OnPromptTextCommitted(const FText& Text, ETextCommit::Type CommitType);
    FReply OnPromptKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent);
    
    // Option tab selection related functions
    FReply OnOptionTabClicked(EOptionTabType TabType);
    FSlateColor GetOptionTabTextColor(EOptionTabType TabType) const;
    FSlateColor GetOptionTabBackgroundColor(EOptionTabType TabType) const;
    
    // Prompt booster related functions
    void SendPromptBoosterRequest(const FString& Prompt);
    void OnPromptBoosterResponse(const TArray<FString>& PromptSuggestions);
    FReply OnPromptSuggestionClicked(int32 Index);
    FReply OnPromptSuggestionMouseUp(const FGeometry& Geometry, const FPointerEvent& MouseEvent, int32 Index);
    FSlateColor GetPromptSuggestionButtonColor(int32 Index) const;
    void OnPromptSuggestionHovered(int32 Index);
    void OnPromptSuggestionUnhovered();

    // Image Prompter related functions/status
    void OnImageAssetSelected(const FAssetData& AssetData);
    FString GetCurrentImageAssetPath() const;
    void UpdateImagePreview();
    bool ConvertImageAssetToBase64(FString& OutBase64) const;
    void OnImage2SfxApiResponse(const FString& ResponseContent);

    // Image2Sfx response parsing and UI related functions
    bool ParseImage2SfxResponse(const FString& ResponseContent, TArray<FImage2SfxLayer>& OutLayers);
    FString RemoveMarkdownCodeBlock(const FString& Content);
    void UpdateImage2SfxLayersUI();
    FReply OnImage2SfxLayerClicked(int32 Index);
    FSlateColor GetImage2SfxLayerButtonColor(int32 Index) const;
    void OnImage2SfxLayerHovered(int32 Index);
    void OnImage2SfxLayerUnhovered();
    FOptionalSize GetImagePreviewWidth() const;
    FOptionalSize GetImagePreviewHeight() const;

    // Image Prompter - capture source integration related
    void InitImageSourceOptions();
    void OnImageSourceChanged(TSharedPtr<FString> NewSelection, ESelectInfo::Type SelectInfo);
    FReply OnCaptureButtonClicked();
    bool UpdateImagePreviewFromPng(const TArray<uint8>& PngBytes);
    void RequestImage2SfxWithBase64(const FString& Prompt, const FString& ImageBase64);
    FText GetCaptureButtonText() const;
    EVisibility GetCaptureButtonVisibility() const;
    EVisibility GetImageAssetPickerVisibility() const;
    void SetImage2SfxLoading(bool bInLoading);

    // Get the result view corresponding to the current tab type
    TSharedPtr<SAudioResultView> GetResultView(EOptionTabType Tab) const;

    // Determine the visibility of the output section (hidden initially, only visible when loading/result is present)
    bool ShouldShowOutputSection() const;
    bool ShouldShowPromptBoosterSeparator() const;

    // History management
    void AddHistoryItem(const TArray<TObjectPtr<USoundWave>>& NewSoundWaves, const FString& Prompt, EOptionTabType TabType, bool bStereo, const FString& LayerName = TEXT(""), const FString& LayerCategory = TEXT(""), TSharedPtr<FSlateBrush> Thumbnail = nullptr, const FString& UserInputPrompt = TEXT(""));

private:
    TSharedPtr<class FApiClient> ApiClient;
    TSharedPtr<class SMultiLineEditableTextBox> PromptTextBox;
    
    // Tab-specific independent result view
    TSharedPtr<SAudioResultView> AudioResultView_PromptBooster;
    TSharedPtr<SAudioResultView> AudioResultView_MultiLayering;
    TSharedPtr<SAudioResultView> AudioResultView_ImagePrompter;

    TSharedPtr<class SVerticalBox> OutputSectionContainer;
    
    // Option tab related UI elements
    TSharedPtr<class SVerticalBox> OptionContentContainer;
    TSharedPtr<class SVerticalBox> PromptBoosterLoadingContainer;
    TSharedPtr<class SVerticalBox> PromptSuggestionsContainer;
    TSharedPtr<class SVerticalBox> ImagePrompterContainer;
    TSharedPtr<class SVerticalBox> MultiLayeringContainer;
    TSharedPtr<class SScrollBox> Image2SfxLayersScrollBox;
    TSharedPtr<class SVerticalBox> ImagePrompterLoadingContainer;
    TSharedPtr<SObjectPropertyEntryBox> ImageAssetSelectorWidget;
    TSharedPtr<class SImage> ImagePreviewWidget;
    TSharedPtr<struct FSlateBrush> ImagePreviewBrush;
    
    TArray<TObjectPtr<USoundWave>> GeneratedSoundWaves;
    
    // Currently selected option tab
    EOptionTabType CurrentSelectedTab;

    // Last tab that sent the gen request (response routing)
    EOptionTabType LastGenRequestTab = EOptionTabType::PromptBooster;
    
    // Prompt booster response/status
    TArray<FString> CurrentPromptSuggestions;
    bool bIsPromptBoosterLoading = false;
    FString LastUserPromptText;
    
    // Selected prompt index (-1 if not selected)
    int32 SelectedPromptIndex;

    // Currently loading prompt index list (manage concurrent requests)
    TSet<int32> LoadingPromptIndices;

    // Already tried/completed prompt index (prevent duplicate requests)
    TSet<int32> CompletedPromptIndices;
    
    // Hovered prompt index (-1 if not hovered)
    int32 HoveredPromptIndex;

    // Image Prompter status
    UTexture2D* InputTexture = nullptr;
    float ImagePreviewFixedHeight = 256.0f;
    TStrongObjectPtr<UTexture2D> CapturedPreviewTexture; // Capture/PNG preview texture (root maintain)
    EImageSourceType CurrentImageSourceType = EImageSourceType::ContentBrowser;
    TArray<TSharedPtr<FString>> ImageSourceOptions;
    TSharedPtr<STextComboBox> ImageSourceCombo;
    bool bIsImage2SfxLoading = false;
    TSharedPtr<class SVerticalBox> Image2SfxLoadingContainer;
    TArray<uint8> PendingCapturedPng;
    bool bHasPendingCapturedImage = false;

    // Image Prompter generation loading flag
    bool bIsImagePrompterGenerating = false;
    // Image Prompter layer-wise loading status (consider concurrent requests)
    TSet<int32> LoadingImageLayerIndices;
    // Image Prompter layer-wise latest request ID
    TMap<int32, int32> ImageLayerRequestId;
    int32 ImageLayerRequestIdCounter = 0;

    // Already tried/completed Image2Sfx layer index (prevent duplicate requests)
    TSet<int32> CompletedImageLayerIndices;

    // Image2Sfx response storage
    TArray<FImage2SfxLayer> CurrentImage2SfxLayers;
    
    // Selected Image2Sfx layer index (-1 if not selected)
    int32 SelectedImage2SfxLayerIndex;
    
    // Hovered Image2Sfx layer index (-1 if not hovered)
    int32 HoveredImage2SfxLayerIndex;

    // Generator parameters
    int32 NumSamplesValue;  // Sample count (1~3, default 2)
    bool bStereoEnabled;     // Stereo activation status (on/off)

    // Parameter related functions
    TOptional<int32> GetNumSamplesValue() const;
    void OnNumSamplesValueChanged(int32 NewValue);
    ECheckBoxState GetStereoCheckState() const;
    void OnStereoCheckStateChanged(ECheckBoxState NewState);

    // UI Widgets
    TSharedPtr<class SExpandableArea> GenParametersExpandableArea;
    TSharedPtr<SNumericEntryBox<int32>> NumSamplesSpinBox;
    TSharedPtr<class SCheckBox> StereoCheckBox;

    // History UI container and data
    TSharedPtr<class SScrollBox> HistoryListContainer;
    TArray<TSharedPtr<FGenerationHistoryItem>> HistoryData;
}; 
