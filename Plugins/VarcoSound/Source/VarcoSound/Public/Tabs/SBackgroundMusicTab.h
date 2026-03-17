// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "AssetRegistry/AssetData.h"
#include "Engine/Texture2D.h"
#include "Types/SlateStructs.h"
#include "Tabs/IVarcoSoundTab.h"
#include "Utils/VarcoApiTypes.h"
#include "Widgets/Text/STextBlock.h"
#include "Sound/SoundWave.h"
#include "Containers/Ticker.h"

class FApiClient;
class SAudioResultView;
class SObjectPropertyEntryBox;
class SImage;
class STextComboBox;
class SScrollBox;
template <typename ItemType> class SListView;

struct FBgmMusicDownloadResult;

struct FBgmMusicItemView
{
	FString Id;
	FString Title;
	FString CreatedAt;
	FString FilePath;
	bool bDownloaded = false;
};

/**
 * Background music generation tab widget (prompt + image input + request + history UI)
 */
class VARCOSOUND_API SBackgroundMusicTab : public SCompoundWidget, public IVarcoSoundTab
{
public:
	SLATE_BEGIN_ARGS(SBackgroundMusicTab) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	virtual ~SBackgroundMusicTab();

	// IVarcoSoundTab interface
	virtual void PauseAllPlayback() override;
	virtual void SetApiKey(const FString& InApiKey) override;

private:
	struct FBgmGenerationHistoryItem;
	enum class EBgmInputMode : uint8
	{
		ImageWithPrompt,
		ImageOnly,
		PromptOnly
	};

	// UI Events
	FReply OnGenerateButtonClicked();
	bool CanGenerate() const;
	FReply OnInputModeClicked(EBgmInputMode NewMode);
	FSlateColor GetInputModeTextColor(EBgmInputMode Mode) const;
	FSlateColor GetInputModeBackgroundColor(EBgmInputMode Mode) const;

	// Text input (multiline)
	FReply OnPromptKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent);
	void OnPromptCommitted(const FText& Text, ETextCommit::Type CommitType);

	// Image input (Content Browser / Viewport / Selected Actors)
	void InitImageSourceOptions();
	void OnImageAssetSelected(const FAssetData& AssetData);
	FString GetCurrentImageAssetPath() const;
	void UpdateImagePreview();
	bool UpdateImagePreviewFromPng(const TArray<uint8>& PngBytes);
	void OnImageSourceChanged(TSharedPtr<FString> NewSelection, ESelectInfo::Type SelectInfo);
	FReply OnCaptureButtonClicked();
	FText GetCaptureButtonText() const;
	EVisibility GetCaptureButtonVisibility() const;
	EVisibility GetImageAssetPickerVisibility() const;
	FOptionalSize GetImagePreviewWidth() const;
	FOptionalSize GetImagePreviewHeight() const;
	bool ConvertImageAssetToBase64(FString& OutBase64) const;

	// MusicGen wiring
	void OnMusicGenApiResponse(const TArray<FString>& AudioBase64Array);
	FString SaveCapturedPngToTemp(const TArray<uint8>& PngBytes) const;

	// Concurrency guard
	int32 GetActiveRequestCount() const;

	// Per-item async flow (polling + download)
	void StartMusicGeneration(const TSharedPtr<FBgmGenerationHistoryItem>& Item, const FString& Prompt, const FString& ImageBase64);
	void StartPolling(const TSharedPtr<FBgmGenerationHistoryItem>& Item);
	void RequestMusicStatus(const TSharedPtr<FBgmGenerationHistoryItem>& Item);
	void HandleCompletedStatus(const TSharedPtr<FBgmGenerationHistoryItem>& Item, const FBgmMusicStatusResponse& StatusResponse);
	void ScheduleNextPoll(const TSharedPtr<FBgmGenerationHistoryItem>& Item, float DelaySeconds);
	void CancelPolling(const TSharedPtr<FBgmGenerationHistoryItem>& Item);
	bool HasPollingTimedOut(const TSharedPtr<FBgmGenerationHistoryItem>& Item) const;
	void BeginDownloadQueue(const TSharedPtr<FBgmGenerationHistoryItem>& Item, const TArray<FBgmMusicInfo>& Musics);
	void DownloadNextMusic(const TSharedPtr<FBgmGenerationHistoryItem>& Item);
	void OnMusicDownloaded(const TSharedPtr<FBgmGenerationHistoryItem>& Item, bool bSuccess, const FBgmMusicDownloadResult& Result);

	// History UI / status
	TSharedPtr<FBgmGenerationHistoryItem> AddHistoryItem(const FString& PromptText);
	void SetUserFacingStatusText(const TSharedPtr<FBgmGenerationHistoryItem>& Item, const FString& Text);
	void SetHistoryStatus(const TSharedPtr<FBgmGenerationHistoryItem>& Item, const FString& InStatus);
	void FinalizeHistoryItem(const TSharedPtr<FBgmGenerationHistoryItem>& Item, bool bSuccess, const FString& FinalStatus);

	void LaunchMusicExternally(const FString& FilePath) const;

private:
	// State
	// Text
	TSharedPtr<class SMultiLineEditableTextBox> PromptTextBox;
	EBgmInputMode CurrentInputMode = EBgmInputMode::ImageWithPrompt;
	bool bHasHoveredInputMode = false;
	EBgmInputMode HoveredInputMode = EBgmInputMode::ImageWithPrompt;

	// Containers for visibility toggling
	TSharedPtr<class SVerticalBox> TextSectionContainer;
	TSharedPtr<class SVerticalBox> ImageSectionContainer;

	// Image state & widgets
	UTexture2D* InputTexture = nullptr;
	TSharedPtr<SObjectPropertyEntryBox> ImageAssetSelectorWidget;
	TSharedPtr<SImage> ImagePreviewWidget;
	TSharedPtr<struct FSlateBrush> ImagePreviewBrush;
	float ImagePreviewFixedHeight = 256.0f;
	TArray<TSharedPtr<FString>> ImageSourceOptions;
	TSharedPtr<STextComboBox> ImageSourceCombo;
	TStrongObjectPtr<UTexture2D> CapturedPreviewTexture;
	TArray<uint8> PendingCapturedPng;
	bool bHasPendingCapturedImage = false;

	enum class EBgmImageSourceType : uint8
	{
		ContentBrowser,
		Viewport,
		SelectedActors
	};
	EBgmImageSourceType CurrentImageSourceType = EBgmImageSourceType::ContentBrowser;

	// API client and result view
	TSharedPtr<FApiClient> ApiClient;
	TSharedPtr<SScrollBox> HistoryListContainer;
	TArray<TSharedPtr<FBgmGenerationHistoryItem>> HistoryData;

	static constexpr float PollIntervalSeconds = 10.0f;
	static constexpr float PollTimeoutSeconds = 300.0f;
};


