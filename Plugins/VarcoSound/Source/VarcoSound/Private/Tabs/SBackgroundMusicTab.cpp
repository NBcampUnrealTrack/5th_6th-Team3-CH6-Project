// Copyright Epic Games, Inc. All Rights Reserved.

#include "Tabs/SBackgroundMusicTab.h"

#include "Widgets/Layout/SBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Input/SMultiLineEditableTextBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Images/SThrobber.h"
#include "Widgets/Views/SListView.h"
#include "Widgets/Views/STableRow.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/STextComboBox.h"
#include "Widgets/Layout/SExpandableArea.h"
#include "Styling/AppStyle.h"
#include "Styling/SlateBrush.h"
#include "PropertyCustomizationHelpers.h"
#include "Engine/Texture2D.h"
#include "Engine/Texture.h"
#include "EditorFramework/AssetImportData.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/PlatformFilemanager.h"
#include "Utils/AudioUtils.h"
#include "Utils/ApiClient.h"
#include "Utils/ImagePreviewHelpers.h"
#include "Utils/ToastNotification.h"
#include "Utils/VarcoSoundPathUtils.h"
#include "UI/SAudioResultView.h"
#include "Misc/Base64.h"
#include "Misc/Guid.h"
#include "Styling/CoreStyle.h"
#include "HAL/PlatformProcess.h"
#include "HAL/PlatformFilemanager.h"
#include "Misc/Paths.h"

#if WITH_EDITOR
#include "Utils/ImageCaptureUtils.h"
#endif

struct SBackgroundMusicTab::FBgmGenerationHistoryItem
{
	FDateTime Timestamp;
	FString Prompt;
	FString MusicTitle;
	FString TaskId;
	bool bIsActive = false;
	bool bIsLatest = false;
	bool bIsExpanded = true;
	bool bUserPinnedExpanded = false;
	bool bFinalFailed = false;
	FString FailCategory;
	FLinearColor HighlightColor = FLinearColor::Transparent;

	// Data
	TArray<TSharedPtr<FBgmMusicItemView>> Tracks;
	TArray<USoundWave*> SoundWaves;

	// Per-request async state
	FTSTicker::FDelegateHandle PollTickerHandle;
	double PollStartSeconds = 0.0;
	bool bIsPolling = false;
	bool bIsDownloading = false;
	TArray<FBgmMusicInfo> PendingMusicInfos;
	int32 DownloadQueueIndex = 0;

	// UI handles (for incremental updates)
	TSharedPtr<SExpandableArea> ExpandableArea;
	TSharedPtr<STextBlock> StatusTextWidget;
	TSharedPtr<SAudioResultView> ResultView;
};

void SBackgroundMusicTab::SetUserFacingStatusText(const TSharedPtr<FBgmGenerationHistoryItem>& Item, const FString& Text)
{
	if (!Item.IsValid() || !Item->StatusTextWidget.IsValid())
	{
		return;
	}
	Item->StatusTextWidget->SetText(FText::FromString(Text));
}

void SBackgroundMusicTab::Construct(const FArguments& InArgs)
{
	// Initial state

	ChildSlot
	[
		SNew(SScrollBox)
		+ SScrollBox::Slot()
		.Padding(FMargin(10.0f))
		[
			SNew(SVerticalBox)

			// Intro text
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0, 10)
			[
				SNew(STextBlock)
				.Text(FText::FromString(TEXT("🌌 Describe the mood, get cinematic background music. AI composes for your scene.")))
				.Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
				.ColorAndOpacity(FLinearColor(0.7f, 0.7f, 0.7f, 1.0f))
			]

			// (Mode buttons removed: free-form prompt UI)

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
				
				// Text input (left, fills most of the space) - multiline
				+ SHorizontalBox::Slot()
				.FillWidth(1.0f)
				.VAlign(VAlign_Top)
				[
					SNew(SBox)
					.MinDesiredHeight(40.0f)
					.MaxDesiredHeight(120.0f)
					[
						SAssignNew(PromptTextBox, SMultiLineEditableTextBox)
						.HintText(FText::FromString(TEXT("e.g., calm lo-fi ambient for rainy evening")))
						.AutoWrapText(true)
						.AllowMultiLine(true)
						.OnKeyDownHandler(this, &SBackgroundMusicTab::OnPromptKeyDown)
						.IsReadOnly_Lambda([this]() { return CurrentInputMode == EBgmInputMode::ImageOnly; })
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
					.OnClicked(this, &SBackgroundMusicTab::OnGenerateButtonClicked)
					.IsEnabled(this, &SBackgroundMusicTab::CanGenerate)
				]
			]

			// Input mode buttons (GeneratorTab-style)
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0, 0, 0, 5)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(0, 0, 1, 0)
				[
					SNew(SButton)
					.Text(FText::FromString(TEXT("Image with Prompt")))
					.OnClicked_Lambda([this]() { return OnInputModeClicked(EBgmInputMode::ImageWithPrompt); })
					.ButtonColorAndOpacity(this, &SBackgroundMusicTab::GetInputModeBackgroundColor, EBgmInputMode::ImageWithPrompt)
					.ForegroundColor(this, &SBackgroundMusicTab::GetInputModeTextColor, EBgmInputMode::ImageWithPrompt)
					.OnHovered_Lambda([this]() { bHasHoveredInputMode = true; HoveredInputMode = EBgmInputMode::ImageWithPrompt; })
					.OnUnhovered_Lambda([this]() { bHasHoveredInputMode = false; })
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(1, 0, 1, 0)
				[
					SNew(SButton)
					.Text(FText::FromString(TEXT("Image Only")))
					.OnClicked_Lambda([this]() { return OnInputModeClicked(EBgmInputMode::ImageOnly); })
					.ButtonColorAndOpacity(this, &SBackgroundMusicTab::GetInputModeBackgroundColor, EBgmInputMode::ImageOnly)
					.ForegroundColor(this, &SBackgroundMusicTab::GetInputModeTextColor, EBgmInputMode::ImageOnly)
					.OnHovered_Lambda([this]() { bHasHoveredInputMode = true; HoveredInputMode = EBgmInputMode::ImageOnly; })
					.OnUnhovered_Lambda([this]() { bHasHoveredInputMode = false; })
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(1, 0, 0, 0)
				[
					SNew(SButton)
					.Text(FText::FromString(TEXT("Prompt Only")))
					.OnClicked_Lambda([this]() { return OnInputModeClicked(EBgmInputMode::PromptOnly); })
					.ButtonColorAndOpacity(this, &SBackgroundMusicTab::GetInputModeBackgroundColor, EBgmInputMode::PromptOnly)
					.ForegroundColor(this, &SBackgroundMusicTab::GetInputModeTextColor, EBgmInputMode::PromptOnly)
					.OnHovered_Lambda([this]() { bHasHoveredInputMode = true; HoveredInputMode = EBgmInputMode::PromptOnly; })
					.OnUnhovered_Lambda([this]() { bHasHoveredInputMode = false; })
				]
				+ SHorizontalBox::Slot()
				.FillWidth(1.0f)
			]

			// Image input section
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0, 5)
			[
				SNew(SBox)
				.Visibility_Lambda([this]() -> EVisibility
				{
					return (CurrentInputMode == EBgmInputMode::PromptOnly) ? EVisibility::Collapsed : EVisibility::Visible;
				})
				[
					SAssignNew(ImageSectionContainer, SVerticalBox)

					// Input Image label + source combo + capture button (single row)
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
							.OnSelectionChanged(this, &SBackgroundMusicTab::OnImageSourceChanged)
						]

						+ SHorizontalBox::Slot()
						.AutoWidth()
						.Padding(8, 0)
						.VAlign(VAlign_Center)
						[
							SNew(SButton)
							.Text(this, &SBackgroundMusicTab::GetCaptureButtonText)
							.OnClicked(this, &SBackgroundMusicTab::OnCaptureButtonClicked)
							.Visibility(this, &SBackgroundMusicTab::GetCaptureButtonVisibility)
							[
								SNew(SHorizontalBox)
								+ SHorizontalBox::Slot()
								.AutoWidth()
								.VAlign(VAlign_Center)
								[
									SNew(STextBlock)
									.Text(this, &SBackgroundMusicTab::GetCaptureButtonText)
								]
							]
						]
			]

					// Image asset picker widget
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(0, 5)
					[
						SAssignNew(ImageAssetSelectorWidget, SObjectPropertyEntryBox)
						.AllowedClass(UTexture2D::StaticClass())
						.ObjectPath(TAttribute<FString>::CreateSP(this, &SBackgroundMusicTab::GetCurrentImageAssetPath))
						.OnObjectChanged(this, &SBackgroundMusicTab::OnImageAssetSelected)
						.DisplayThumbnail(true)
						.DisplayUseSelected(true)
						.DisplayBrowse(true)
						.Visibility(this, &SBackgroundMusicTab::GetImageAssetPickerVisibility)
					]

					// Image preview (fixed height, keep aspect ratio)
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(0, 5)
					[
						SNew(SBox)
						.WidthOverride(this, &SBackgroundMusicTab::GetImagePreviewWidth)
						.HeightOverride(this, &SBackgroundMusicTab::GetImagePreviewHeight)
						[
							SAssignNew(ImagePreviewWidget, SImage)
						]
					]
				]
			]

		]
		// === History / Results Section (GeneratorTab style) ===
		+ SScrollBox::Slot()
		.Padding(FMargin(0.0f, 0.0f, 0.0f, 0.0f))
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
			.Padding(10, 0, 0, 5)
			[
				SNew(STextBlock)
				.Text(FText::FromString(TEXT("Generation History")))
				.Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
				.ColorAndOpacity(FLinearColor(0.8f, 0.8f, 0.8f, 1.0f))
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SAssignNew(HistoryListContainer, SScrollBox)
				.Orientation(Orient_Vertical)
			]
		]
	];

	ImagePreviewBrush = MakeShared<FSlateBrush>();
	InitImageSourceOptions();

	// Create API client
	ApiClient = MakeShareable(new FApiClient());
}

TSharedPtr<SBackgroundMusicTab::FBgmGenerationHistoryItem> SBackgroundMusicTab::AddHistoryItem(const FString& PromptText)
{
	const int32 MaxHistoryCount = 30;

	// Reset "latest" + auto-collapse older items.
	for (const TSharedPtr<FBgmGenerationHistoryItem>& Old : HistoryData)
	{
		if (Old.IsValid())
		{
			Old->bIsLatest = false;
			Old->HighlightColor = FLinearColor::Transparent;
			// Keep user-pinned expanded items (auto-collapse exception).
			if (!Old->bUserPinnedExpanded)
			{
				Old->bIsExpanded = false;
				if (Old->ExpandableArea.IsValid())
				{
					Old->ExpandableArea->SetExpanded(false);
				}
			}
		}
	}

	TSharedPtr<FBgmGenerationHistoryItem> Item = MakeShared<FBgmGenerationHistoryItem>();
	Item->Timestamp = FDateTime::Now();
	Item->Prompt = PromptText;
	Item->bIsActive = true;
	Item->bIsLatest = true;
	Item->bIsExpanded = true;
	Item->bUserPinnedExpanded = false; // Latest is expanded by default, but not considered "user pinned".
	Item->bFinalFailed = false;
	Item->FailCategory.Empty();
	Item->HighlightColor = FLinearColor(0.3f, 1.0f, 0.6f, 1.0f); // GeneratorTab latest highlight tint

	HistoryData.Insert(Item, 0);

	if (!HistoryListContainer.IsValid())
	{
		return Item;
	}

	// Build UI widget for this item and insert at top
	TSharedRef<SExpandableArea> ItemWidget = SNew(SExpandableArea)
		.InitiallyCollapsed(!Item->bIsExpanded)
		.BorderImage(FAppStyle::GetBrush("DetailsView.CategoryTop"))
		.BodyBorderImage(FAppStyle::GetBrush("NoBrush"))
		.BodyBorderBackgroundColor(FLinearColor::Transparent)
		.HeaderPadding(FMargin(4.0f, 2.0f))
		.Padding(0.0f)
		.OnAreaExpansionChanged_Lambda([WeakItem = TWeakPtr<FBgmGenerationHistoryItem>(Item)](bool bExpanded)
		{
			if (const TSharedPtr<FBgmGenerationHistoryItem> Pinned = WeakItem.Pin())
			{
				Pinned->bIsExpanded = bExpanded;
				// If user manually expands, treat it as pinned (excluded from future auto-collapse).
				Pinned->bUserPinnedExpanded = bExpanded;
			}
		})
		.HeaderContent()
		[
			SNew(SHorizontalBox)

			+ SHorizontalBox::Slot()
			.FillWidth(1.0f)
			.VAlign(VAlign_Center)
			.Padding(0, 0, 8, 0)
			[
				SNew(STextBlock)
				.Text_Lambda([WeakItem = TWeakPtr<FBgmGenerationHistoryItem>(Item)]() -> FText
				{
					if (const TSharedPtr<FBgmGenerationHistoryItem> Pinned = WeakItem.Pin())
					{
						// Fail case: replace title area with FAIL (user request)
						if (Pinned->bFinalFailed)
						{
							const FString Cat = Pinned->FailCategory.TrimStartAndEnd();
							if (!Cat.IsEmpty())
							{
								return FText::FromString(FString::Printf(TEXT("FAIL: %s"), *Cat));
							}
							return FText::FromString(TEXT("FAIL"));
						}

						// Title word:
						// - While active: show input prompt
						// - After completion: show music title (fallback to prompt)
						FString P;
						if (Pinned->bIsActive)
						{
							P = Pinned->Prompt;
						}
						else
						{
							P = !Pinned->MusicTitle.IsEmpty() ? Pinned->MusicTitle : Pinned->Prompt;
						}
						if (P.IsEmpty())
						{
							P = TEXT("Generated Music");
						}
						const int32 MaxLen = 40;
						if (P.Len() > MaxLen) { P = P.Left(MaxLen) + TEXT("..."); }
						return FText::FromString(P);
					}
					return FText::FromString(TEXT("Generated Music"));
				})
				.Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
				.ColorAndOpacity_Lambda([WeakItem = TWeakPtr<FBgmGenerationHistoryItem>(Item)]()
				{
					if (const TSharedPtr<FBgmGenerationHistoryItem> Pinned = WeakItem.Pin())
					{
						return Pinned->bIsLatest ? Pinned->HighlightColor : FLinearColor::White;
					}
					return FLinearColor::White;
				})
				.OverflowPolicy(ETextOverflowPolicy::Ellipsis)
			]

			// Mode badge
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(0, 0, 8, 0)
			[
				SNew(STextBlock)
				.Text_Lambda([WeakItem = TWeakPtr<FBgmGenerationHistoryItem>(Item)]() -> FText
				{
					if (const TSharedPtr<FBgmGenerationHistoryItem> Pinned = WeakItem.Pin())
					{
						if (Pinned->bIsActive)
						{
							return FText::FromString(TEXT("[BGM]"));
						}

						FString Prompt = Pinned->Prompt;
						Prompt.TrimStartAndEndInline();
						const int32 MaxPromptLen = 20;
						if (Prompt.Len() > MaxPromptLen) { Prompt = Prompt.Left(MaxPromptLen) + TEXT("..."); }
						if (Prompt.IsEmpty()) { return FText::FromString(TEXT("[BGM]")); }
						return FText::FromString(FString::Printf(TEXT("[BGM: %s]"), *Prompt));
					}
					return FText::FromString(TEXT("[BGM]"));
				})
				.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
				.ColorAndOpacity_Lambda([WeakItem = TWeakPtr<FBgmGenerationHistoryItem>(Item)]()
				{
					if (const TSharedPtr<FBgmGenerationHistoryItem> Pinned = WeakItem.Pin())
					{
						return Pinned->bIsLatest ? Pinned->HighlightColor : FLinearColor(0.7f, 0.7f, 0.7f, 1.0f);
					}
					return FLinearColor(0.7f, 0.7f, 0.7f, 1.0f);
				})
			]

			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(0, 0, 8, 0)
			[
				SNew(SThrobber)
				.NumPieces(3)
				.Visibility_Lambda([WeakItem = TWeakPtr<FBgmGenerationHistoryItem>(Item)]() -> EVisibility
				{
					if (const TSharedPtr<FBgmGenerationHistoryItem> Pinned = WeakItem.Pin())
					{
						return Pinned->bIsActive ? EVisibility::Visible : EVisibility::Collapsed;
					}
					return EVisibility::Collapsed;
				})
			]

			// Status text (kept AutoWidth to stay right-aligned)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(0, 0, 8, 0)
			[
				SAssignNew(Item->StatusTextWidget, STextBlock)
				.Text(FText::FromString(TEXT("Ready.")))
				.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
				.ColorAndOpacity(FLinearColor(0.7f, 0.7f, 0.7f, 1.0f))
				.Visibility_Lambda([WeakItem = TWeakPtr<FBgmGenerationHistoryItem>(Item)]() -> EVisibility
				{
					if (const TSharedPtr<FBgmGenerationHistoryItem> Pinned = WeakItem.Pin())
					{
						// Only show while active (composition/downloading). Hide after completion.
						return Pinned->bIsActive ? EVisibility::Visible : EVisibility::Collapsed;
					}
					return EVisibility::Collapsed;
				})
			]

			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text(FText::FromString(Item->Timestamp.ToString(TEXT("%H:%M:%S"))))
				.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
				.ColorAndOpacity(FLinearColor(0.5f, 0.5f, 0.5f, 1.0f))
			]
		]
		.BodyContent()
		[
			SNew(SBorder)
			.BorderImage(FAppStyle::GetBrush("NoBrush"))
			.BorderBackgroundColor_Lambda([WeakItem = TWeakPtr<FBgmGenerationHistoryItem>(Item)]()
			{
				if (const TSharedPtr<FBgmGenerationHistoryItem> Pinned = WeakItem.Pin())
				{
					return Pinned->bIsLatest ? Pinned->HighlightColor.CopyWithNewOpacity(0.12f) : FLinearColor::Transparent;
				}
				return FLinearColor::Transparent;
			})
			.Visibility_Lambda([WeakItem = TWeakPtr<FBgmGenerationHistoryItem>(Item)]() -> EVisibility
			{
				if (const TSharedPtr<FBgmGenerationHistoryItem> Pinned = WeakItem.Pin())
				{
					const bool bHasAudio = Pinned->SoundWaves.Num() > 0;
					return (!Pinned->bIsActive && bHasAudio) ? EVisibility::Visible : EVisibility::Collapsed;
				}
				return EVisibility::Collapsed;
			})
			.Padding(FMargin(0, 4, 0, 6))
			[
				// Result view should be shown only after the request fully completes (all downloads done).
				SAssignNew(Item->ResultView, SAudioResultView)
			]
		];

	Item->ExpandableArea = ItemWidget;

	// Insert at top
	HistoryListContainer->InsertSlot(0)
	.Padding(0.0f)
	[
		ItemWidget
	];

	// Always keep the latest item visible at the top
	HistoryListContainer->ScrollToStart();

	// Trim UI + data
	if (HistoryData.Num() > MaxHistoryCount)
	{
		HistoryData.SetNum(MaxHistoryCount);
	}
	while (HistoryListContainer->GetChildren()->Num() > MaxHistoryCount)
	{
		const int32 LastIdx = HistoryListContainer->GetChildren()->Num() - 1;
		TSharedRef<SWidget> OldestWidget = HistoryListContainer->GetChildren()->GetChildAt(LastIdx);
		HistoryListContainer->RemoveSlot(OldestWidget);
	}

	// Initialize view tab type
	if (Item->ResultView.IsValid())
	{
		Item->ResultView->SetTabType(EAudioResultViewTabType::BackgroundMusic);
		Item->ResultView->SetLoading(false);
		Item->ResultView->SetSourceTag(TEXT("Unknown"));
		Item->ResultView->SetSoundWaves(TArray<USoundWave*>());
		Item->ResultView->SetPromptText(PromptText);
	}

	return Item;
}

int32 SBackgroundMusicTab::GetActiveRequestCount() const
{
	int32 Count = 0;
	for (const TSharedPtr<FBgmGenerationHistoryItem>& Item : HistoryData)
	{
		if (Item.IsValid() && Item->bIsActive)
		{
			Count++;
		}
	}
	return Count;
}

void SBackgroundMusicTab::SetHistoryStatus(const TSharedPtr<FBgmGenerationHistoryItem>& Item, const FString& InStatus)
{
	// User-facing: keep status text minimal.
	// Developer-facing: log full details.
	if (!Item.IsValid())
	{
		return;
	}
	if (!InStatus.IsEmpty())
	{
		UE_LOG(LogTemp, Log, TEXT("[BGM History] %s"), *InStatus);
	}
}

void SBackgroundMusicTab::FinalizeHistoryItem(const TSharedPtr<FBgmGenerationHistoryItem>& Item, bool bSuccess, const FString& FinalStatus)
{
	if (!Item.IsValid())
	{
		return;
	}

	// If the API flow did not complete successfully:
	// - keep the history row
	// - replace title area with FAIL (no PARTIAL label per user request)
	if (!bSuccess)
	{
		CancelPolling(Item);
		Item->bIsActive = false;
		Item->bIsPolling = false;
		Item->bIsDownloading = false;
		Item->bFinalFailed = true;
		SetUserFacingStatusText(Item, TEXT(""));

		// Keep row compact: collapse body (no audio to show on failure)
		if (Item->ExpandableArea.IsValid())
		{
			Item->bIsExpanded = false;
			Item->ExpandableArea->SetExpanded(false);
		}

		if (!FinalStatus.IsEmpty())
		{
			UE_LOG(LogTemp, Log, TEXT("[BGM History] Finalize failed: %s"), *FinalStatus);
		}
		return;
	}

	Item->bIsActive = false;
	Item->bIsPolling = false;
	Item->bIsDownloading = false;
	Item->bFinalFailed = false;
	Item->FailCategory.Empty();
	// Do not show "Done/Failed" to users; waveform visibility is the completion signal.
	SetUserFacingStatusText(Item, TEXT(""));
	if (!FinalStatus.IsEmpty())
	{
		UE_LOG(LogTemp, Log, TEXT("[BGM History] Finalize: %s"), *FinalStatus);
	}
}

SBackgroundMusicTab::~SBackgroundMusicTab()
{
	// Cancel all active tickers (per-history-item) to avoid dangling callbacks.
	for (const TSharedPtr<FBgmGenerationHistoryItem>& Item : HistoryData)
	{
		CancelPolling(Item);
	}
}

void SBackgroundMusicTab::PauseAllPlayback()
{
}

void SBackgroundMusicTab::SetApiKey(const FString& InApiKey)
{
	if (ApiClient.IsValid())
	{
		ApiClient->SetApiKey(InApiKey);
	}
}

void SBackgroundMusicTab::OnPromptCommitted(const FText& Text, ETextCommit::Type CommitType)
{
	// Multiline textbox: Enter commit is disabled (Generate button only).
}

FReply SBackgroundMusicTab::OnPromptKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent)
{
	// When pressing Enter
	if (InKeyEvent.GetKey() == EKeys::Enter)
	{
		// Enter (no Shift): trigger Generate
		if (!InKeyEvent.IsShiftDown())
		{
			if (CurrentInputMode == EBgmInputMode::ImageOnly)
			{
				// Prompt is disabled in Image Only mode.
				return FReply::Handled();
			}
			OnGenerateButtonClicked();
			return FReply::Handled();
		}
		// Shift+Enter: insert newline (default behavior)
	}
	
	return FReply::Unhandled();
}

FReply SBackgroundMusicTab::OnInputModeClicked(EBgmInputMode NewMode)
{
	CurrentInputMode = NewMode;

	// Image Only: lock prompt editing.
	if (PromptTextBox.IsValid() && NewMode == EBgmInputMode::ImageOnly)
	{
		PromptTextBox->SetText(FText::GetEmpty());
	}

	// Prompt Only: hide the image section, but keep the last selected/captured image state.
	// This way, when the user switches back, the preview is restored.
	if (NewMode == EBgmInputMode::PromptOnly)
	{
		// No-op: keep InputTexture / PendingCapturedPng as-is.
		// Ensure preview stays consistent (including the default transparent placeholder when empty).
		UpdateImagePreview();
	}

	if (ImageSectionContainer.IsValid())
	{
		ImageSectionContainer->Invalidate(EInvalidateWidget::Layout);
	}
	if (PromptTextBox.IsValid())
	{
		PromptTextBox->Invalidate(EInvalidateWidget::Layout);
	}

	return FReply::Handled();
}

void SBackgroundMusicTab::InitImageSourceOptions()
{
	ImageSourceOptions.Empty();
	ImageSourceOptions.Add(MakeShared<FString>(TEXT("Content Browser")));
	ImageSourceOptions.Add(MakeShared<FString>(TEXT("Viewport")));
	ImageSourceOptions.Add(MakeShared<FString>(TEXT("Selected Actors")));
	CurrentImageSourceType = EBgmImageSourceType::ContentBrowser;
	if (ImageSourceCombo.IsValid())
	{
		ImageSourceCombo->RefreshOptions();
		ImageSourceCombo->SetSelectedItem(ImageSourceOptions[0]);
	}
}

void SBackgroundMusicTab::OnImageAssetSelected(const FAssetData& AssetData)
{
	UTexture2D* SelectedTexture = Cast<UTexture2D>(AssetData.GetAsset());
	InputTexture = (SelectedTexture && IsValid(SelectedTexture)) ? SelectedTexture : nullptr;
	PendingCapturedPng.Reset();
	bHasPendingCapturedImage = false;
	UpdateImagePreview();
}

FString SBackgroundMusicTab::GetCurrentImageAssetPath() const
{
	UTexture2D* LocalTexture = InputTexture;
	if (!LocalTexture || !IsValid(LocalTexture)) { return FString(); }
	return LocalTexture->GetPathName();
}

void SBackgroundMusicTab::UpdateImagePreview()
{
	// Keep a stable "transparent placeholder" when no image is selected.
	// (FImagePreviewHelpers clears the SImage brush with nullptr, which makes the preview appear to vanish.)
	if (InputTexture && IsValid(InputTexture))
	{
		FImagePreviewHelpers::UpdateImagePreview(InputTexture, ImagePreviewBrush, ImagePreviewWidget, ImagePreviewFixedHeight);
		return;
	}

	if (!ImagePreviewBrush.IsValid())
	{
		ImagePreviewBrush = MakeShared<FSlateBrush>();
	}

	// Clear resource but keep a valid brush size.
	ImagePreviewBrush->SetResourceObject(nullptr);
	ImagePreviewBrush->ImageSize = FVector2D(ImagePreviewFixedHeight, ImagePreviewFixedHeight);

	if (ImagePreviewWidget.IsValid())
	{
		// Use a checkerboard placeholder (transparent-layer look).
		ImagePreviewWidget->SetImage(FCoreStyle::Get().GetBrush("Checkerboard"));
		ImagePreviewWidget->Invalidate(EInvalidateWidget::Layout);
	}
}

bool SBackgroundMusicTab::UpdateImagePreviewFromPng(const TArray<uint8>& PngBytes)
{
	return FImagePreviewHelpers::UpdateImagePreviewFromPng(
		PngBytes,
		CapturedPreviewTexture,
		InputTexture,
		ImagePreviewBrush,
		ImagePreviewWidget,
		ImagePreviewFixedHeight);
}

void SBackgroundMusicTab::OnImageSourceChanged(TSharedPtr<FString> NewSelection, ESelectInfo::Type /*SelectInfo*/)
{
	if (!NewSelection.IsValid()) { return; }
	const FString& Sel = *NewSelection.Get();
	if (Sel == TEXT("Viewport"))
	{
		CurrentImageSourceType = EBgmImageSourceType::Viewport;
	}
	else if (Sel == TEXT("Selected Actors"))
	{
		CurrentImageSourceType = EBgmImageSourceType::SelectedActors;
	}
	else
	{
		CurrentImageSourceType = EBgmImageSourceType::ContentBrowser;
	}
	if (ImageSectionContainer.IsValid())
	{
		ImageSectionContainer->Invalidate(EInvalidateWidget::Layout);
	}
}

FReply SBackgroundMusicTab::OnCaptureButtonClicked()
{
#if WITH_EDITOR
	TArray<uint8> Png;
	bool bOk = false;
	if (CurrentImageSourceType == EBgmImageSourceType::Viewport)
	{
		FViewportCaptureOptions Opts;
		bOk = CaptureEditorViewportToPng(Opts, Png);
	}
	else if (CurrentImageSourceType == EBgmImageSourceType::SelectedActors)
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
	VarcoSoundToast::ShowError(FText::FromString(TEXT("Capture failed or no actors selected.")));
	return FReply::Handled();
}

FText SBackgroundMusicTab::GetCaptureButtonText() const
{
	switch (CurrentImageSourceType)
	{
	case EBgmImageSourceType::Viewport: return FText::FromString(TEXT("Capture Viewport"));
	case EBgmImageSourceType::SelectedActors: return FText::FromString(TEXT("Capture Selected"));
	default: return FText::FromString(TEXT("Capture"));
	}
}

EVisibility SBackgroundMusicTab::GetCaptureButtonVisibility() const
{
	return (CurrentImageSourceType == EBgmImageSourceType::Viewport || CurrentImageSourceType == EBgmImageSourceType::SelectedActors)
		? EVisibility::Visible : EVisibility::Collapsed;
}

EVisibility SBackgroundMusicTab::GetImageAssetPickerVisibility() const
{
	return (CurrentImageSourceType == EBgmImageSourceType::ContentBrowser) ? EVisibility::Visible : EVisibility::Collapsed;
}

FOptionalSize SBackgroundMusicTab::GetImagePreviewWidth() const
{
	return FImagePreviewHelpers::GetImagePreviewWidth(ImagePreviewBrush, ImagePreviewFixedHeight);
}

FOptionalSize SBackgroundMusicTab::GetImagePreviewHeight() const
{
	return FImagePreviewHelpers::GetImagePreviewHeight(ImagePreviewFixedHeight);
}

bool SBackgroundMusicTab::CanGenerate() const
{
	const bool bHasText = PromptTextBox.IsValid() && !PromptTextBox->GetText().ToString().TrimStartAndEnd().IsEmpty();
	const bool bHasImage = (InputTexture && IsValid(InputTexture)) || bHasPendingCapturedImage;
	switch (CurrentInputMode)
	{
	case EBgmInputMode::ImageWithPrompt:
		return bHasText && bHasImage;
	case EBgmInputMode::ImageOnly:
		return bHasImage;
	case EBgmInputMode::PromptOnly:
		return bHasText;
	default:
		return bHasText || bHasImage;
	}
}

FSlateColor SBackgroundMusicTab::GetInputModeTextColor(EBgmInputMode Mode) const
{
	if (CurrentInputMode == Mode)
	{
		return FLinearColor::White;
	}
	return FLinearColor(0.7f, 0.7f, 0.7f, 1.0f);
}

FSlateColor SBackgroundMusicTab::GetInputModeBackgroundColor(EBgmInputMode Mode) const
{
	if (CurrentInputMode == Mode)
	{
		return FLinearColor(0.2f, 0.4f, 0.8f, 1.0f);
	}
	// Hover highlight (match GeneratorTab feel)
	if (bHasHoveredInputMode && HoveredInputMode == Mode)
	{
		return FLinearColor(0.25f, 0.5f, 0.9f, 1.0f);
	}
	return FLinearColor(0.3f, 0.3f, 0.3f, 1.0f);
}

FReply SBackgroundMusicTab::OnGenerateButtonClicked()
{
	// Limit concurrent requests.
	if (GetActiveRequestCount() >= 3)
	{
		// Hard limit: do not cancel existing requests.
		VarcoSoundToast::ShowWarning(FText::FromString(TEXT("Request limit reached (max 3). Please wait for a request to finish.")));
		return FReply::Handled();
	}

	if (!ApiClient.IsValid())
	{
		VarcoSoundToast::ShowError(FText::FromString(TEXT("API client is not initialized.")));
		return FReply::Handled();
	}

	const FString PromptForUi = PromptTextBox.IsValid() ? PromptTextBox->GetText().ToString().TrimStartAndEnd() : FString();
	FString PromptForApi = PromptForUi;

	FString ImageBase64;
	bool bHaveImage = false;

	const bool bNeedPrompt = (CurrentInputMode == EBgmInputMode::ImageWithPrompt || CurrentInputMode == EBgmInputMode::PromptOnly);
	const bool bNeedImage = (CurrentInputMode == EBgmInputMode::ImageWithPrompt || CurrentInputMode == EBgmInputMode::ImageOnly);

	if (bNeedPrompt && PromptForUi.IsEmpty())
	{
		VarcoSoundToast::ShowError(FText::FromString(TEXT("Please enter a prompt.")));
		return FReply::Handled();
	}

	// Image Only: do not send a prompt field. Keep a UI label only.
	if (CurrentInputMode == EBgmInputMode::ImageOnly)
	{
		PromptForApi.Empty();
	}

	if (bNeedImage)
	{
		if (bHasPendingCapturedImage && PendingCapturedPng.Num() > 0)
		{
			const int64 ImageSize = PendingCapturedPng.Num();
			if (ImageSize > 50 * 1024 * 1024)
			{
				VarcoSoundToast::ShowError(FText::FromString(TEXT("Captured image is too large (max 50 MB).")));
				return FReply::Handled();
			}
			ImageBase64 = FBase64::Encode(PendingCapturedPng);
			bHaveImage = true;
		}
		else if (CurrentImageSourceType == EBgmImageSourceType::ContentBrowser)
		{
			bHaveImage = ConvertImageAssetToBase64(ImageBase64);
		}

		if (!bHaveImage)
		{
			VarcoSoundToast::ShowError(FText::FromString(TEXT("Please select or capture an image.")));
			return FReply::Handled();
		}
	}
	else
	{
		// Prompt-only mode: do not send an image.
		ImageBase64.Empty();
	}

	// History label uses the user's prompt (or a default label for Image Only).
	const FString PromptForHistory = (CurrentInputMode == EBgmInputMode::ImageOnly && PromptForUi.IsEmpty()) ? TEXT("Image only") : PromptForUi;
	TSharedPtr<FBgmGenerationHistoryItem> Item = AddHistoryItem(PromptForHistory);
	// Do not show the waveform while generating. The header throbber + status text is enough.
	SetUserFacingStatusText(Item, TEXT("Sending request..."));
	SetHistoryStatus(Item, TEXT("Sending request (BGM generate)."));
	StartMusicGeneration(Item, PromptForApi, ImageBase64);
	return FReply::Handled();
}

FString SBackgroundMusicTab::SaveCapturedPngToTemp(const TArray<uint8>& PngBytes) const
{
    const FString Dir = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("VarcoSound"));
    IPlatformFile& PF = FPlatformFileManager::Get().GetPlatformFile();
    if (!PF.DirectoryExists(*Dir)) { PF.CreateDirectoryTree(*Dir); }
    const FString Path = FPaths::Combine(Dir, FString::Printf(TEXT("bgm_cap_%s.png"), *FGuid::NewGuid().ToString(EGuidFormats::Digits)));
    if (FFileHelper::SaveArrayToFile(PngBytes, *Path))
    {
        return Path;
    }
    return FString();
}

bool SBackgroundMusicTab::ConvertImageAssetToBase64(FString& OutBase64) const
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
		// Only allow PNG/JPG(JPEG) for BGM image input.
		{
			const FString Ext = FPaths::GetExtension(SourceFilePath).ToLower();
			if (Ext != TEXT("png") && Ext != TEXT("jpg") && Ext != TEXT("jpeg"))
			{
				VarcoSoundToast::ShowError(FText::FromString(TEXT("Only PNG or JPG images are supported.")));
				return false;
			}
		}

		IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
		const int64 FileSize = PlatformFile.FileSize(*SourceFilePath);
		if (FileSize > 50 * 1024 * 1024)
		{
			VarcoSoundToast::ShowError(FText::Format(
				NSLOCTEXT("VarcoSound", "ImageTooLargeBgm", "Image is too large ({0} MB). Maximum size is 50 MB."),
				FText::AsNumber(FileSize / 1024.0 / 1024.0)));
			return false;
		}

		TArray<uint8> FileData;
		if (FFileHelper::LoadFileToArray(FileData, *SourceFilePath))
		{
			OutBase64 = FBase64::Encode(FileData);
			return true;
		}
	}

	VarcoSoundToast::ShowError(NSLOCTEXT("VarcoSound", "ImagePathNotFoundBgm", "Could not find image source file path or loading failed."));
	return false;
}

void SBackgroundMusicTab::OnMusicGenApiResponse(const TArray<FString>& AudioBase64Array)
{
	// Legacy/shortcut path: if direct audio is provided, attach it to the most recent item.
	TArray<USoundWave*> Waves;
	for (const FString& B64 : AudioBase64Array)
	{
		if (USoundWave* W = FAudioUtils::CreateSoundWaveFromBase64(B64))
		{
			Waves.Add(W);
		}
	}
	if (HistoryData.Num() > 0 && HistoryData[0].IsValid())
	{
		const TSharedPtr<FBgmGenerationHistoryItem> Item = HistoryData[0];
		Item->SoundWaves = Waves;
		if (Item->ResultView.IsValid())
		{
			Item->ResultView->SetLoading(false);
			Item->ResultView->SetSourceTag(TEXT("Base64"));
			Item->ResultView->SetSoundWaves(Waves);
		}
	}
}

void SBackgroundMusicTab::StartMusicGeneration(const TSharedPtr<FBgmGenerationHistoryItem>& Item, const FString& Prompt, const FString& ImageBase64)
{
	if (!ApiClient.IsValid() || !Item.IsValid())
	{
		return;
	}

	TWeakPtr<FBgmGenerationHistoryItem> WeakItem(Item);
	ApiClient->SendMusicGenerationRequest(Prompt, ImageBase64,
		FOnMusicTaskCreated::CreateLambda([this, WeakItem](bool bSuccess, const FString& TaskIdOrError)
		{
			const TSharedPtr<FBgmGenerationHistoryItem> Pinned = WeakItem.Pin();
			if (!Pinned.IsValid())
			{
				return;
			}

			if (!bSuccess)
			{
				Pinned->FailCategory = TEXT("Request Failed");
				SetHistoryStatus(Pinned, FString::Printf(TEXT("Request failed: %s"), *TaskIdOrError));
				VarcoSoundToast::ShowError(FText::Format(
					NSLOCTEXT("VarcoSound", "BgmRequestFailed", "BGM generation request failed: {0}"),
					FText::FromString(TaskIdOrError)));
				if (Pinned->ResultView.IsValid())
				{
					Pinned->ResultView->SetLoading(false);
				}
				FinalizeHistoryItem(Pinned, false, TEXT("Failed"));
				return;
			}

			Pinned->TaskId = TaskIdOrError;
			SetHistoryStatus(Pinned, FString::Printf(TEXT("Task received: %s. Starting polling..."), *TaskIdOrError));
			SetUserFacingStatusText(Pinned, TEXT("In composition..."));
			StartPolling(Pinned);
		})
	);
}

void SBackgroundMusicTab::StartPolling(const TSharedPtr<FBgmGenerationHistoryItem>& Item)
{
	if (!Item.IsValid())
	{
		return;
	}
	CancelPolling(Item);
	Item->bIsPolling = true;
	Item->PollStartSeconds = FPlatformTime::Seconds();
	RequestMusicStatus(Item);
}

void SBackgroundMusicTab::RequestMusicStatus(const TSharedPtr<FBgmGenerationHistoryItem>& Item)
{
	if (!ApiClient.IsValid() || !Item.IsValid() || Item->TaskId.IsEmpty())
	{
		return;
	}

	TWeakPtr<FBgmGenerationHistoryItem> WeakItem(Item);
	ApiClient->GetMusicGenerationStatus(Item->TaskId,
		FOnMusicStatusReceived::CreateLambda([this, WeakItem](bool bSuccess, const FBgmMusicStatusResponse& Response)
		{
			const TSharedPtr<FBgmGenerationHistoryItem> Pinned = WeakItem.Pin();
			if (!Pinned.IsValid())
			{
				return;
			}

			if (!bSuccess)
			{
				Pinned->FailCategory = TEXT("Polling Failed");
				SetHistoryStatus(Pinned, TEXT("Status polling failed."));
				VarcoSoundToast::ShowError(NSLOCTEXT("VarcoSound", "BgmStatusPollFailed", "Failed to poll BGM generation status. Please try again."));
				CancelPolling(Pinned);
				if (Pinned->ResultView.IsValid())
				{
					Pinned->ResultView->SetLoading(false);
				}
				FinalizeHistoryItem(Pinned, false, TEXT("Failed"));
				return;
			}

			const FString NormalizedStatus = Response.status.ToLower();

			if (NormalizedStatus == TEXT("processing"))
			{
				if (HasPollingTimedOut(Pinned))
				{
					Pinned->FailCategory = TEXT("Timeout");
					SetHistoryStatus(Pinned, TEXT("Polling timed out (5 minutes)."));
					VarcoSoundToast::ShowWarning(NSLOCTEXT("VarcoSound", "BgmPollTimeout", "BGM generation timed out after 5 minutes. Please try again."));
					CancelPolling(Pinned);
					if (Pinned->ResultView.IsValid())
					{
						Pinned->ResultView->SetLoading(false);
					}
					FinalizeHistoryItem(Pinned, false, TEXT("Timed out"));
					return;
				}
				// Keep user-facing status minimal.
				SetUserFacingStatusText(Pinned, TEXT("In composition..."));
				SetHistoryStatus(Pinned, TEXT("Processing..."));
				ScheduleNextPoll(Pinned, PollIntervalSeconds);
				return;
			}

			if (NormalizedStatus == TEXT("completed"))
			{
				SetHistoryStatus(Pinned, TEXT("Generation completed. Starting download..."));
				SetUserFacingStatusText(Pinned, TEXT("Downloading..."));
				HandleCompletedStatus(Pinned, Response);
				return;
			}

			Pinned->FailCategory = TEXT("Unknown Status");
			SetHistoryStatus(Pinned, FString::Printf(TEXT("Unknown status: %s"), *Response.status));
			VarcoSoundToast::ShowWarning(FText::Format(
				NSLOCTEXT("VarcoSound", "BgmUnknownStatus", "Received unknown status: {0}. Generation stopped."),
				FText::FromString(Response.status)));
			CancelPolling(Pinned);
			if (Pinned->ResultView.IsValid())
			{
				Pinned->ResultView->SetLoading(false);
			}
			FinalizeHistoryItem(Pinned, false, TEXT("Failed"));
		})
	);
}

void SBackgroundMusicTab::HandleCompletedStatus(const TSharedPtr<FBgmGenerationHistoryItem>& Item, const FBgmMusicStatusResponse& StatusResponse)
{
	if (!Item.IsValid())
	{
		return;
	}
	CancelPolling(Item);
	if (Item->ResultView.IsValid())
	{
		Item->ResultView->SetLoading(false);
	}

	if (StatusResponse.musics.Num() == 0)
	{
		Item->FailCategory = TEXT("No Tracks Returned");
		SetHistoryStatus(Item, TEXT("Completed, but no tracks were returned."));
		VarcoSoundToast::ShowWarning(NSLOCTEXT("VarcoSound", "BgmNoMusicReturned", "BGM generation completed but no tracks were returned."));
		FinalizeHistoryItem(Item, false, TEXT("No tracks returned"));
		return;
	}

	VarcoSoundToast::ShowSuccess(FText::Format(
		NSLOCTEXT("VarcoSound", "BgmGenerationComplete", "BGM generation completed! {0} track(s) ready."),
		FText::AsNumber(StatusResponse.musics.Num())));
	BeginDownloadQueue(Item, StatusResponse.musics);
}

void SBackgroundMusicTab::ScheduleNextPoll(const TSharedPtr<FBgmGenerationHistoryItem>& Item, float DelaySeconds)
{
	if (!Item.IsValid())
	{
		return;
	}
	FTSTicker::GetCoreTicker().RemoveTicker(Item->PollTickerHandle);
	TWeakPtr<FBgmGenerationHistoryItem> WeakItem(Item);
	Item->PollTickerHandle = FTSTicker::GetCoreTicker().AddTicker(
		FTickerDelegate::CreateLambda([this, WeakItem](float)
		{
			const TSharedPtr<FBgmGenerationHistoryItem> Pinned = WeakItem.Pin();
			if (!Pinned.IsValid())
			{
				return false;
			}
			RequestMusicStatus(Pinned);
			return false;
		}),
		DelaySeconds);
}

void SBackgroundMusicTab::CancelPolling(const TSharedPtr<FBgmGenerationHistoryItem>& Item)
{
	if (!Item.IsValid())
	{
		return;
	}
	if (Item->PollTickerHandle.IsValid())
	{
		FTSTicker::GetCoreTicker().RemoveTicker(Item->PollTickerHandle);
		Item->PollTickerHandle = FTSTicker::FDelegateHandle();
	}
	Item->bIsPolling = false;
}

bool SBackgroundMusicTab::HasPollingTimedOut(const TSharedPtr<FBgmGenerationHistoryItem>& Item) const
{
	if (!Item.IsValid())
	{
		return false;
	}
	const double Elapsed = FPlatformTime::Seconds() - Item->PollStartSeconds;
	return Elapsed > PollTimeoutSeconds;
}

void SBackgroundMusicTab::BeginDownloadQueue(const TSharedPtr<FBgmGenerationHistoryItem>& Item, const TArray<FBgmMusicInfo>& Musics)
{
	if (!Item.IsValid())
	{
		return;
	}
	Item->PendingMusicInfos = Musics;
	Item->DownloadQueueIndex = 0;
	Item->bIsDownloading = true;
	Item->Tracks.Reset();
	Item->SoundWaves.Reset();

	for (const FBgmMusicInfo& Info : Item->PendingMusicInfos)
	{
		TSharedPtr<FBgmMusicItemView> Track = MakeShared<FBgmMusicItemView>();
		Track->Id = Info.id;
		Track->Title = Info.title.IsEmpty() ? TEXT("Untitled") : Info.title;
		Track->CreatedAt = Info.created_at;
		Track->bDownloaded = false;
		Item->Tracks.Add(Track);
	}
	// Title word: use first track name when available.
	if (Item->Tracks.Num() > 0 && Item->Tracks[0].IsValid())
	{
		Item->MusicTitle = Item->Tracks[0]->Title;
	}

	DownloadNextMusic(Item);
}

void SBackgroundMusicTab::DownloadNextMusic(const TSharedPtr<FBgmGenerationHistoryItem>& Item)
{
	if (!ApiClient.IsValid() || !Item.IsValid() || Item->DownloadQueueIndex >= Item->PendingMusicInfos.Num())
	{
		if (Item.IsValid())
		{
			Item->bIsDownloading = false;
		}
		SetHistoryStatus(Item, TEXT("All tracks downloaded."));
		// Show waveform only after all downloads complete.
		if (Item.IsValid() && Item->ResultView.IsValid() && Item->SoundWaves.Num() > 0)
		{
			Item->ResultView->SetSourceTag(TEXT("Unknown"));
			Item->ResultView->SetSoundWaves(Item->SoundWaves);
		}
		SetUserFacingStatusText(Item, TEXT(""));
		
		// Check download success
		int32 SuccessCount = 0;
		if (Item.IsValid())
		{
			for (const auto& Track : Item->Tracks)
			{
				if (Track.IsValid() && Track->bDownloaded)
				{
					SuccessCount++;
				}
			}
		}
		
		const int32 TotalCount = Item.IsValid() ? Item->Tracks.Num() : 0;
		const bool bAllOk = (TotalCount > 0 && SuccessCount == TotalCount);
		if (bAllOk)
		{
			VarcoSoundToast::ShowSuccess(NSLOCTEXT("VarcoSound", "BgmDownloadComplete", "All BGM tracks downloaded successfully!"));
			FinalizeHistoryItem(Item, true, TEXT("Done"));
		}
		else if (TotalCount > 0 && SuccessCount == 0)
		{
			// All downloads failed -> mark as FAIL and keep row with reason
			if (Item.IsValid())
			{
				Item->FailCategory = TEXT("Download Failed");
			}
			VarcoSoundToast::ShowError(NSLOCTEXT("VarcoSound", "BgmDownloadAllFailed", "Failed to download BGM tracks."));
			FinalizeHistoryItem(Item, false, TEXT("All downloads failed"));
		}
		else
		{
			VarcoSoundToast::ShowWarning(FText::Format(
				NSLOCTEXT("VarcoSound", "BgmDownloadPartial", "{0} of {1} tracks downloaded successfully."),
				FText::AsNumber(SuccessCount),
				FText::AsNumber(TotalCount)));
			// Partial: keep history row and show only successfully downloaded tracks (no PARTIAL label per user request).
			FinalizeHistoryItem(Item, true, TEXT("Completed with errors"));
		}
		return;
	}

	const FBgmMusicInfo& Info = Item->PendingMusicInfos[Item->DownloadQueueIndex];
	// User-facing: "Downloading..." only. Developer-facing: show progress.
	SetUserFacingStatusText(Item, TEXT("Downloading..."));
	SetHistoryStatus(Item, FString::Printf(TEXT("Downloading... (%d/%d)"), Item->DownloadQueueIndex + 1, Item->PendingMusicInfos.Num()));

	const FString CleanTitle = VarcoSoundPathUtils::SanitizeFilename(Info.title.IsEmpty() ? TEXT("Untitled") : Info.title, 60);
	const FString Timestamp = VarcoSoundPathUtils::MakeTimestamp_yyyyMMdd_HHmmss();
	const FString DesiredFileName = FString::Printf(TEXT("%s_%02d_%s.mp3"), *CleanTitle, Item->DownloadQueueIndex + 1, *Timestamp);

	ApiClient->DownloadMusicFile(Info.id, DesiredFileName,
		[this, WeakItem = TWeakPtr<FBgmGenerationHistoryItem>(Item)](bool bSuccess, const FBgmMusicDownloadResult& Result)
		{
			const TSharedPtr<FBgmGenerationHistoryItem> Pinned = WeakItem.Pin();
			if (!Pinned.IsValid())
			{
				return;
			}
			OnMusicDownloaded(Pinned, bSuccess, Result);
		});
}

void SBackgroundMusicTab::OnMusicDownloaded(const TSharedPtr<FBgmGenerationHistoryItem>& Item, bool bSuccess, const FBgmMusicDownloadResult& Result)
{
	if (!Item.IsValid())
	{
		return;
	}

	if (Item->DownloadQueueIndex < Item->Tracks.Num() && Item->Tracks[Item->DownloadQueueIndex].IsValid())
	{
		if (bSuccess)
		{
			Item->Tracks[Item->DownloadQueueIndex]->FilePath = Result.FilePath;
			Item->Tracks[Item->DownloadQueueIndex]->bDownloaded = true;
		}
		else
		{
			Item->Tracks[Item->DownloadQueueIndex]->bDownloaded = false;
		}
	}

	if (bSuccess && !Result.FilePath.IsEmpty())
	{
		const FString Extension = FPaths::GetExtension(Result.FilePath).ToLower();
		if (Extension == TEXT("wav") || Extension == TEXT("mp3"))
		{
			if (USoundWave* Wave = FAudioUtils::CreateSoundWaveFromFile(Result.FilePath))
			{
				Item->SoundWaves.Add(Wave);
				// Do not update/show waveform incrementally. It will appear after all downloads complete.
			}
		}
	}
	else
	{
		// Track download failed (used as failure reason if everything fails, or just a warning when partial)
		Item->FailCategory = TEXT("Download Failed");
		SetHistoryStatus(Item, FString::Printf(TEXT("Download failed: %s"), *Result.ErrorMessage));
		VarcoSoundToast::ShowError(FText::Format(
			NSLOCTEXT("VarcoSound", "BgmTrackDownloadFailed", "Failed to download track {0}: {1}"),
			FText::AsNumber(Item->DownloadQueueIndex + 1),
			FText::FromString(Result.ErrorMessage)));
	}

	Item->DownloadQueueIndex++;
	// Always continue via DownloadNextMusic(). It owns the completion path
	// (including setting ResultView sound waves after all downloads complete).
	DownloadNextMusic(Item);
}

void SBackgroundMusicTab::LaunchMusicExternally(const FString& FilePath) const
{
	if (FilePath.IsEmpty()) { return; }
	FPlatformProcess::LaunchFileInDefaultExternalApplication(*FilePath);
}

