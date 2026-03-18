// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/SAudioResultView.h"
#include "UI/SWaveformDisplay.h"
#include "Utils/AudioUtils.h"
#include "Utils/AudioUIHelpers.h"
#include "Utils/VarcoSoundPathUtils.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Input/SButton.h"
#include "Sound/SoundWave.h"
#include "Sound/SoundWaveProcedural.h"
#include "Components/AudioComponent.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Images/SThrobber.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Widgets/Notifications/SNotificationList.h"
#include "Styling/AppStyle.h"
#include "Widgets/Text/STextBlock.h"
#include "UI/SVarcoSoundIconWidget.h"
#include "SlateOptMacros.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Editor.h"
#include "Misc/MessageDialog.h"
#include "AssetToolsModule.h"
#include "IAssetTools.h"
#include "Factories/SoundFactory.h"
#include "UObject/SavePackage.h"
#include "Engine/Engine.h"
#include "HAL/PlatformFilemanager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "DesktopPlatformModule.h"
#include "Framework/Application/SlateApplication.h"
#include "Utils/AudioTrimUtils.h"
#include "HAL/PlatformProcess.h"

namespace
{
    FString FormatDetailedTimestamp(float Seconds)
    {
        const int32 TotalHundredths = FMath::Max(0, static_cast<int32>(FMath::RoundHalfFromZero(Seconds * 100.0f)));
        const int32 Minutes = TotalHundredths / 6000;
        const int32 RemainderHundredths = TotalHundredths % 6000;
        const int32 SecondsPart = RemainderHundredths / 100;
        const int32 Hundredths = RemainderHundredths % 100;

        if (Minutes > 0)
        {
            return FString::Printf(TEXT("%d:%02d.%02d"), Minutes, SecondsPart, Hundredths);
        }
        return FString::Printf(TEXT("%02d.%02d"), SecondsPart, Hundredths);
    }
}

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION

void SAudioResultView::Construct(const FArguments& InArgs)
{
    CurrentSoundWaveIndex = -1;
    AudioPlayer = MakeUnique<FAudioPlayer>();

    ChildSlot
    [
        SNew(SVerticalBox)

        // Waveform Display Area with Border
        +SVerticalBox::Slot()
        .AutoHeight()
        .Padding(0, 0)
        [
            SNew(SBorder)
            .BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
            .Padding(FMargin(10.0f, 6.0f)) // 내부 패딩 줄임
            [
                SNew(SBox)
                .HeightOverride(100)
                [
                    SNew(SBorder)
                    .BorderImage(FAppStyle::GetBrush("ToolPanel.DarkGroupBorder"))
                    .BorderBackgroundColor(FLinearColor(0.05f, 0.05f, 0.05f, 1.0f))
                    [
                        // 로딩 중에는 안내 문구, 아니면 파형 표시
                        SNew(SOverlay)
                        + SOverlay::Slot()
                        .HAlign(HAlign_Center)
                        .VAlign(VAlign_Center)
                        [
                            SNew(SVerticalBox)
                            .Visibility(this, &SAudioResultView::GetLoadingTextVisibility)
                            + SVerticalBox::Slot()
                            .AutoHeight()
                            .HAlign(HAlign_Center)
                            [
                                SNew(SCircularThrobber)
                                .NumPieces(12)
                                .Period(1.0f)
                                .Radius(10.0f)
                            ]
                            + SVerticalBox::Slot()
                            .AutoHeight()
                            .Padding(0, 6, 0, 0)
                            .HAlign(HAlign_Center)
                            [
                                SNew(STextBlock)
                                .Text(this, &SAudioResultView::GetLoadingStatusText)
                                .ColorAndOpacity(FLinearColor(0.8f, 0.8f, 0.8f, 1.0f))
                                .Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
                            ]
                        ]
                        + SOverlay::Slot()
                        [
                            SAssignNew(WaveformDisplayWidget, SWaveformDisplay)
                            .Visibility(this, &SAudioResultView::GetWaveformVisibility)
                            .WaveformColor(FLinearColor(0.45f, 0.55f, 1.0f, 1.0f))
                        ]
                        + SOverlay::Slot()
                        .HAlign(HAlign_Right)
                        .VAlign(VAlign_Bottom)
                        [
                            SNew(SBox)
                            .Padding(FMargin(0.f, 0.f, 6.f, 4.f))
                            [
                                SNew(STextBlock)
                                .Text(this, &SAudioResultView::GetPlaybackTimeText)
                                .Visibility(this, &SAudioResultView::GetPlaybackTimeVisibility)
                                .Font(FCoreStyle::GetDefaultFontStyle("Normal", 9))
                                .ColorAndOpacity(FLinearColor(0.75f, 0.75f, 0.75f, 1.0f))
                                .ShadowColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, 0.6f))
                                .ShadowOffset(FVector2D(1.f, 1.f))
                            ]
                        ]
                        // Sample Index (Bottom-Left)
                        + SOverlay::Slot()
                        .HAlign(HAlign_Left)
                        .VAlign(VAlign_Bottom)
                        [
                            SNew(SBox)
                            .Padding(FMargin(6.f, 0.f, 0.f, 4.f))
                            [
                                SNew(STextBlock)
                                .Text(this, &SAudioResultView::GetSampleIndexText)
                                .Visibility(this, &SAudioResultView::GetSampleIndexVisibility)
                                .Font(FCoreStyle::GetDefaultFontStyle("Bold", 9))
                                .ColorAndOpacity(FLinearColor(0.9f, 0.9f, 0.9f, 1.0f))
                                .ShadowColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, 0.6f))
                                .ShadowOffset(FVector2D(1.f, 1.f))
                            ]
                        ]
                    ]
                ]
            ]
        ]

        // Playback Controls
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(0, 2) // 파형과 재생 버튼 사이 여백 줄임
        .HAlign(HAlign_Center)
        [
            SNew(SHorizontalBox)

            // 1. Stop Button (Square)
            + SHorizontalBox::Slot()
            .AutoWidth()
            .Padding(2, 0)
            [
                SNew(SButton)
                .ButtonStyle(&FAppStyle::Get().GetWidgetStyle<FButtonStyle>("SimpleButton"))
                .ContentPadding(FMargin(6.f))
                .OnClicked(this, &SAudioResultView::OnStopButtonClicked)
                .IsFocusable(false)
                [
                    SNew(SVarcoSoundIcon)
                    .IconType(EVarcoSoundIconType::Stop)
                    .IconColor(FLinearColor(0.85f, 0.85f, 0.85f, 1.0f))
                    .IconSize(FVector2D(13.f, 13.f))
                ]
            ]

            // 2. Play/Pause Button (Triangle)
            + SHorizontalBox::Slot()
            .AutoWidth()
            .Padding(2, 0)
            [
                SAssignNew(PlayPauseButton, SButton)
                .ButtonStyle(&FAppStyle::Get().GetWidgetStyle<FButtonStyle>("SimpleButton"))
                .ContentPadding(FMargin(6.f))
                .OnClicked(this, &SAudioResultView::OnPlayButtonClicked)
                .IsFocusable(false)
                [
                    SNew(SVarcoSoundIcon)
                    .IconType(this, &SAudioResultView::GetPlayButtonIconType)
                    .IconColor(this, &SAudioResultView::GetPlayButtonIconTint)
                    .IconSize(FVector2D(13.f, 13.f))
                ]
            ]

            // 3. Next Sample Button (Arrow Right)
            + SHorizontalBox::Slot()
            .AutoWidth()
            .Padding(2, 0)
            [
                SAssignNew(SampleCycleButton, SButton)
                .ButtonStyle(&FAppStyle::Get().GetWidgetStyle<FButtonStyle>("SimpleButton"))
                .ContentPadding(FMargin(6.f))
                .OnClicked(this, &SAudioResultView::OnSampleCycleButtonClicked)
                .Visibility(this, &SAudioResultView::GetSampleButtonVisibility) // Only visible if >1 sample
                .IsEnabled(this, &SAudioResultView::IsSampleCycleEnabled)
                .IsFocusable(false)
                [
                    SNew(SImage)
                    .Image(FAppStyle::Get().GetBrush("Icons.ArrowRight"))
                    .ColorAndOpacity(FLinearColor(0.85f, 0.85f, 0.85f, 1.0f))
                    .DesiredSizeOverride(FVector2D(16.f, 16.f))
                ]
            ]

            // 4. Refresh/Regenerate Button (Refresh)
            + SHorizontalBox::Slot()
            .AutoWidth()
            .Padding(2, 0)
            [
                SNew(SButton)
                .ButtonStyle(&FAppStyle::Get().GetWidgetStyle<FButtonStyle>("SimpleButton"))
                .ContentPadding(FMargin(6.f))
                .OnClicked(this, &SAudioResultView::OnRefreshButtonClicked)
                .Visibility(this, &SAudioResultView::GetRefreshButtonVisibility)
                .IsFocusable(false)
                [
                    SNew(SImage)
                    .Image(FAppStyle::Get().GetBrush("Icons.Refresh"))
                    .ColorAndOpacity(FLinearColor(0.85f, 0.85f, 0.85f, 1.0f))
                    .DesiredSizeOverride(FVector2D(16.f, 16.f))
                ]
            ]

            // 5. Save Button (Save)
            + SHorizontalBox::Slot()
            .AutoWidth()
            .Padding(2, 0)
            [
                SNew(SButton)
                .ButtonStyle(&FAppStyle::Get().GetWidgetStyle<FButtonStyle>("SimpleButton"))
                .ContentPadding(FMargin(6.f))
                .OnClicked(this, &SAudioResultView::OnSaveButtonClicked)
                .IsEnabled(this, &SAudioResultView::IsSaveButtonEnabled)
                .IsFocusable(false)
				.ToolTipText_Lambda([this]() -> FText
				{
					if (TabType == EAudioResultViewTabType::BackgroundMusic)
					{
						return NSLOCTEXT("VarcoSound", "AlreadyAutoSaved", "Already auto-saved. Open save location");
					}
					return FText::FromString(TEXT("Save as WAV"));
				})
                [
                    SNew(SImage)
                    .Image_Lambda([this]() -> const FSlateBrush*
                    {
						if (TabType == EAudioResultViewTabType::BackgroundMusic)
						{
							return FAppStyle::Get().GetBrush("Icons.FolderOpen");
						}
                        return FAppStyle::Get().GetBrush("Icons.Save");
                    })
                    .ColorAndOpacity(FLinearColor(0.85f, 0.85f, 0.85f, 1.0f))
                    .DesiredSizeOverride(FVector2D(16.f, 16.f))
                ]
            ]

            // Fill remaining space
            + SHorizontalBox::Slot()
            .FillWidth(1.0f)
        ]
    ];

        // 파형 상호작용 델리게이트 바인딩
    if (WaveformDisplayWidget.IsValid())
    {
        WaveformDisplayWidget->SetOnSeekRequested(SWaveformDisplay::FOnSeekRequested::CreateSP(this, &SAudioResultView::HandleWaveformSeek));
        WaveformDisplayWidget->SetOnTogglePlayPauseRequested(SWaveformDisplay::FOnTogglePlayPauseRequested::CreateSP(this, &SAudioResultView::HandleWaveformTogglePlayPause));
    }
}

void SAudioResultView::SetSoundWaves(const TArray<USoundWave*>& InSoundWaves)
{
    ResetPlaybackState();
    SoundWaves.Empty();

    // Unroot previous
    UnrootSoundWaves();

    const bool bShouldTrim = (TabType == EAudioResultViewTabType::Generator || TabType == EAudioResultViewTabType::AutoVariation);

    TArray<USoundWave*> ToRoot;

    for (USoundWave* SoundWave : InSoundWaves)
    {
        if (!SoundWave)
        {
            continue;
        }

        if (bShouldTrim)
        {
            if (USoundWave* Trimmed = FAudioTrimUtils::TrimSilenceFromEnds(SoundWave, TrimVadThreshold, TrimPreRollSeconds, TrimPostRollSeconds, TrimFrameDurationSeconds))
            {
                SoundWaves.Add(Trimmed);
                ToRoot.Add(Trimmed);
                continue;
            }
        }

        SoundWaves.Add(SoundWave);
        ToRoot.Add(SoundWave);
    }

    // Root new waves to prevent GC while UI uses them
    RootSoundWaves(ToRoot);
    
    if (SoundWaves.Num() > 0)
    {
        CurrentSoundWaveIndex = 0;
    }
    else
    {
        CurrentSoundWaveIndex = -1;
    }

    if (SoundWaves.Num() > 0)
    {
        bIsLoading = false;
        Invalidate(EInvalidateWidget::Layout);
    }

    UpdateWaveformDisplayAndSetSound();

    if (SampleCycleButton.IsValid())
    {
        SampleCycleButton->Invalidate(EInvalidateWidget::Paint);
    }
    if (PlayPauseButton.IsValid())
    {
        PlayPauseButton->Invalidate(EInvalidateWidget::Paint);
    }
}

void SAudioResultView::RootSoundWaves(const TArray<USoundWave*>& InSoundWaves)
{
    RootedWaves.Empty();
    for (USoundWave* W : InSoundWaves)
    {
        if (W && !W->IsRooted())
        {
            W->AddToRoot();
        }
        RootedWaves.Add(W);
    }
}

void SAudioResultView::UnrootSoundWaves()
{
    for (TWeakObjectPtr<USoundWave>& W : RootedWaves)
    {
        if (W.IsValid() && W->IsRooted())
        {
            W->RemoveFromRoot();
        }
    }
    RootedWaves.Empty();
}

SAudioResultView::~SAudioResultView()
{
    // 라이브 코딩 안전성을 위한 예외 처리
    try
    {
        // Ensure audio is stopped when the widget is destroyed
        if (AudioPlayer)
        {
            AudioPlayer->Stop();
        }
        ResetPlaybackState();
        UnrootSoundWaves();
    }
    catch (...)
    {
        UE_LOG(LogTemp, Warning, TEXT("Exception caught in SAudioResultView destructor"));
    }
}

void SAudioResultView::OnPlaybackPercentChanged(const UAudioComponent* InAudioComponent, const USoundWave* PlayingSoundWave, float PlaybackPercent)
{
    if (AudioPlayer && InAudioComponent != AudioPlayer->GetAudioComponent())
    {
        return;
    }
	// 재생 중이 아니면 콜백 무시 (일시정지/정지 상태의 UI 유지)
	if (!AudioPlayer || !AudioPlayer->IsPlaying())
	{
		return;
	}
    const double Now = FPlatformTime::Seconds();
	float AdjustedPercent = PlaybackPercent;
	const USoundWave* ActiveWave = PlayingSoundWave;
	if (!ActiveWave && SoundWaves.IsValidIndex(CurrentSoundWaveIndex))
	{
		ActiveWave = SoundWaves[CurrentSoundWaveIndex];
	}
	
	const bool bShouldCorrect = bHasActiveStartOffset && ActiveWave && Cast<USoundWaveProcedural>(ActiveWave) && LastPlayStartPercent > 0.0f;
	if (bShouldCorrect)
	{
		const float Remaining = 1.0f - LastPlayStartPercent;
		if (Remaining > KINDA_SMALL_NUMBER)
		{
			// Procedural seek queues remaining audio but duration stays full,
			// so PlaybackPercent is absolute (0..Remaining).
			const float Clamped = FMath::Clamp(PlaybackPercent, 0.0f, Remaining);
			AdjustedPercent = LastPlayStartPercent + Clamped;
		}
		else
		{
			AdjustedPercent = 1.0f;
		}
		UE_LOG(LogTemp, Verbose, TEXT("[ARV::Pct] corrected raw=%.3f adj=%.3f start=%.3f"), PlaybackPercent, AdjustedPercent, LastPlayStartPercent);
	}
    CurrentPlaybackPercent = FMath::Clamp(AdjustedPercent, 0.0f, 1.0f);
    if (WaveformDisplayWidget.IsValid())
    {
        WaveformDisplayWidget->SetPlaybackPercentage(CurrentPlaybackPercent);
    }

    // 한 프레임이라도 0과 1 사이 진행을 본 이후에만 완료 폴백 허용
    if (CurrentPlaybackPercent > 0.0f && CurrentPlaybackPercent < 0.999f)
    {
        bSeenProgressThisPlay = true;
    }

    // Procedural 종료 콜백 미발생 대비: 퍼센트가 끝에 도달하면 종료/루프 처리
	// 첫 유효 진행 로그
	if (!bSeenProgressThisPlay && CurrentPlaybackPercent > 0.0f && CurrentPlaybackPercent < 0.999f)
	{
		UE_LOG(LogTemp, Log, TEXT("[ARV::Pct] first-progress pct=%.3f dt(ms)=%.1f src=%s"),
			CurrentPlaybackPercent, (Now - LastPlayRequestSeconds) * 1000.0, *CurrentSourceTag);
	}

    // 단, 재생 클릭 직후 즉시 도착하는 스퍼리어스 1.0 이벤트는 무시 (레이스 방지)
    const double dtSincePlayMs = (Now - LastPlayRequestSeconds) * 1000.0;
    if (CurrentPlaybackPercent >= 0.999f && dtSincePlayMs < 200.0)
    {
        UE_LOG(LogTemp, Log, TEXT("[ARV::Pct] ignore spurious end pct=%.3f dt(ms)=%.1f src=%s"), CurrentPlaybackPercent, dtSincePlayMs, *CurrentSourceTag);
        return;
    }
    if (bSeenProgressThisPlay && CurrentPlaybackPercent >= 0.999f)
    {
        UE_LOG(LogTemp, Log, TEXT("[ARV::Pct] near end pct=%.3f dt(ms)=%.1f src=%s"), CurrentPlaybackPercent, (Now-LastPlayRequestSeconds)*1000.0, *CurrentSourceTag);
        if (TabType == EAudioResultViewTabType::AutoLoop)
        {
            if (AudioPlayer)
            {
                bSeenProgressThisPlay = false;
				LastPlayStartPercent = 0.0f;
				bHasActiveStartOffset = false;
                AudioPlayer->Play(0.0f);
            }
            CurrentPlaybackPercent = 0.0f;
            if (WaveformDisplayWidget.IsValid())
            {
                WaveformDisplayWidget->SetPlaybackPercentage(0.0f);
            }
        }
        else
        {
            if (AudioPlayer)
            {
                AudioPlayer->Stop();
            }
            CurrentPlaybackPercent = 0.0f;
			bHasActiveStartOffset = false;
            if (WaveformDisplayWidget.IsValid())
            {
                WaveformDisplayWidget->SetPlaybackPercentage(0.0f);
            }
            if (PlayPauseButton.IsValid())
            {
                PlayPauseButton->Invalidate(EInvalidateWidget::Paint);
            }
        }
    }
}

void SAudioResultView::UpdateWaveformDisplayAndSetSound()
{
    if (WaveformDisplayWidget.IsValid())
    {
        if (SoundWaves.IsValidIndex(CurrentSoundWaveIndex))
        {
            USoundWave* CurrentSoundWave = SoundWaves[CurrentSoundWaveIndex];
            
            // 파형 업데이트 (스테레오/모노 자동)
            FAudioUIHelpers::UpdateWaveformDisplay(WaveformDisplayWidget.Get(), CurrentSoundWave);

            FOnAudioPlaybackPercentNative::FDelegate PlaybackPercentDelegate;
            PlaybackPercentDelegate.BindSP(this, &SAudioResultView::OnPlaybackPercentChanged);
            if (AudioPlayer)
            {
                const bool bShouldLoop = (TabType == EAudioResultViewTabType::AutoLoop);
                TWeakPtr<SAudioResultView> SelfWeak = SharedThis(this);
				AudioPlayer->SetSourceTag(CurrentSourceTag);
                FAudioUIHelpers::ConfigurePlayer(
                    *AudioPlayer,
                    CurrentSoundWave,
                    PlaybackPercentDelegate,
                    bShouldLoop,
                    [SelfWeak]()
                    {
                        if (TSharedPtr<SAudioResultView> Pinned = SelfWeak.Pin())
                        {
                            Pinned->CurrentPlaybackPercent = 0.0f;
                            if (Pinned->WaveformDisplayWidget.IsValid())
                            {
                                Pinned->WaveformDisplayWidget->SetPlaybackPercentage(0.0f);
                            }
                            if (Pinned->PlayPauseButton.IsValid())
                            {
                                Pinned->PlayPauseButton->Invalidate(EInvalidateWidget::Paint);
                            }
                        }
                    }
                );
            }

            // Store total length (seconds)
            CurrentTotalDurationSeconds = CurrentSoundWave->Duration;
			UE_LOG(LogTemp, Log, TEXT("[ARV::SetSound] idx=%d name=%s dur=%.3f ch=%d sr=%d raw=%d src=%s"),
				CurrentSoundWaveIndex,
				*CurrentSoundWave->GetName(),
				CurrentTotalDurationSeconds,
				static_cast<int32>(CurrentSoundWave->NumChannels),
				static_cast<int32>(CurrentSoundWave->GetSampleRateForCurrentPlatform()),
				static_cast<int32>(CurrentSoundWave->RawPCMDataSize),
				*CurrentSourceTag);
        }
        else
        {
            WaveformDisplayWidget->SetWaveformData(TArray<float>());
            if (AudioPlayer)
            {
                // Clear before unsetting sound
                bSeenProgressThisPlay = false;
                AudioPlayer->SetSound(nullptr);
            }
            CurrentTotalDurationSeconds = 0.0f;
        }
    }
}

void SAudioResultView::ResetPlaybackState()
{
    if (AudioPlayer)
    {
        AudioPlayer->Stop();
    }
    CurrentPlaybackPercent = 0.0f;
    bSeenProgressThisPlay = false;
	LastPlayStartPercent = 0.0f;
	bPendingSeekWhilePaused = false;
	bHasActiveStartOffset = false;
    if (WaveformDisplayWidget.IsValid())
    {
        WaveformDisplayWidget->SetPlaybackPercentage(0.0f);
    }
    if (PlayPauseButton.IsValid())
    {
        PlayPauseButton->Invalidate(EInvalidateWidget::Paint);
    }
}

FReply SAudioResultView::OnPlayButtonClicked()
{
    if (!SoundWaves.IsValidIndex(CurrentSoundWaveIndex))
    {
        return FReply::Handled();
    }
    
    if (AudioPlayer && AudioPlayer->IsPlaying())
    {
        UE_LOG(LogTemp, Log, TEXT("[ARV::OnPlay] Pause idx=%d, pct=%.3f"), CurrentSoundWaveIndex, CurrentPlaybackPercent);
        AudioPlayer->Pause();
    }
    else if (AudioPlayer)
    {
        // 재생 시작 (일시정지면 그대로 재개, 정지/자연종료로 컴포넌트가 없으면 초기화)
        bSeenProgressThisPlay = false;
        LastPlayRequestSeconds = FPlatformTime::Seconds();

        USoundWave* CurrentSoundWave = SoundWaves[CurrentSoundWaveIndex];
        const bool bShouldLoop = (TabType == EAudioResultViewTabType::AutoLoop);

        // 사용자가 시킹한 경우에는 해당 지점부터 재시작하도록 컴포넌트 정지
        if (bPendingSeekWhilePaused)
        {
            AudioPlayer->Stop();
        }

        bool bHasValidComp = false;
        if (UAudioComponent* AC = AudioPlayer->GetAudioComponent())
        {
            bHasValidComp = AC->IsValidLowLevelFast() && !AC->HasAnyFlags(RF_BeginDestroyed);
        }

        if (!bHasValidComp)
        {
            FOnAudioPlaybackPercentNative::FDelegate PlaybackPercentDelegate;
            PlaybackPercentDelegate.BindSP(this, &SAudioResultView::OnPlaybackPercentChanged);
            TWeakPtr<SAudioResultView> SelfWeak = SharedThis(this);
            FAudioUIHelpers::ConfigurePlayer(
                *AudioPlayer,
                CurrentSoundWave,
                PlaybackPercentDelegate,
                bShouldLoop,
                [SelfWeak]()
                {
                    if (TSharedPtr<SAudioResultView> Pinned = SelfWeak.Pin())
                    {
                        Pinned->CurrentPlaybackPercent = 0.0f;
                        if (Pinned->WaveformDisplayWidget.IsValid())
                        {
                            Pinned->WaveformDisplayWidget->SetPlaybackPercentage(0.0f);
                        }
                        if (Pinned->PlayPauseButton.IsValid())
                        {
                            Pinned->PlayPauseButton->Invalidate(EInvalidateWidget::Paint);
                        }
                    }
                }
            );
        }

		const float StartPercent = bPendingSeekWhilePaused ? LastPlayStartPercent : CurrentPlaybackPercent;
        const float StartSec = StartPercent * CurrentTotalDurationSeconds;
		if (bPendingSeekWhilePaused)
		{
			bPendingSeekWhilePaused = false;
			CurrentPlaybackPercent = StartPercent;
		}
		LastPlayStartPercent = StartPercent;
		bHasActiveStartOffset = (StartPercent > 0.0f);
        UE_LOG(LogTemp, Log, TEXT("[ARV::OnPlay] Play idx=%d, sound=%p, loop=%d startPct=%.3f offset=%d"), CurrentSoundWaveIndex, CurrentSoundWave, bShouldLoop, StartPercent, bHasActiveStartOffset ? 1 : 0);
        AudioPlayer->Play(StartSec);
    }
    
    return FReply::Handled();
}

FReply SAudioResultView::OnStopButtonClicked()
{
    if (AudioPlayer)
    {
        UE_LOG(LogTemp, Log, TEXT("[ARV::OnStop] idx=%d, pct=%.3f"), CurrentSoundWaveIndex, CurrentPlaybackPercent);
        FAudioUIHelpers::StopAndReset(*AudioPlayer, WaveformDisplayWidget.Get(), CurrentPlaybackPercent, PlayPauseButton);
        bSeenProgressThisPlay = false;
		bPendingSeekWhilePaused = false;
		LastPlayStartPercent = 0.0f;
    }
    return FReply::Handled();
}

FReply SAudioResultView::OnSampleCycleButtonClicked()
{
    if (SoundWaves.Num() > 0)
    {
        ResetPlaybackState();
        CurrentSoundWaveIndex = (CurrentSoundWaveIndex + 1) % SoundWaves.Num();
        UpdateWaveformDisplayAndSetSound();
        if (PlayPauseButton.IsValid())
        {
            PlayPauseButton->Invalidate(EInvalidateWidget::Paint);
        }
        if (SampleCycleButton.IsValid())
        {
            SampleCycleButton->Invalidate(EInvalidateWidget::Paint);
        }
    }
    return FReply::Handled();
}

FText SAudioResultView::GetSampleIndexText() const
{
    if (SoundWaves.Num() > 0)
    {
        return FText::FromString(FString::Printf(TEXT("%d/%d"), CurrentSoundWaveIndex + 1, SoundWaves.Num()));
    }
    return FText::GetEmpty();
}

EVisibility SAudioResultView::GetSampleIndexVisibility() const
{
    return (SoundWaves.Num() > 1) ? EVisibility::Visible : EVisibility::Collapsed;
}

EVisibility SAudioResultView::GetRefreshButtonVisibility() const
{
    return OnRegenerateRequested.IsBound() ? EVisibility::Visible : EVisibility::Collapsed;
}

FReply SAudioResultView::OnRefreshButtonClicked()
{
    OnRegenerateRequested.ExecuteIfBound();
    return FReply::Handled();
}

EVisibility SAudioResultView::GetSampleButtonVisibility() const
{
    return (SoundWaves.Num() > 1) ? EVisibility::Visible : EVisibility::Collapsed;
}

EVarcoSoundIconType SAudioResultView::GetPlayButtonIconType() const
{
    if (AudioPlayer && AudioPlayer->IsPlaying())
    {
        return EVarcoSoundIconType::Pause;
    }
    return EVarcoSoundIconType::Play;
}

FLinearColor SAudioResultView::GetPlayButtonIconTint() const
{
    const bool bHasSound = SoundWaves.IsValidIndex(CurrentSoundWaveIndex) && SoundWaves[CurrentSoundWaveIndex] != nullptr;
    const bool bIsPlayable = bHasSound && !bIsLoading && CurrentTotalDurationSeconds > 0.0f;
    return bIsPlayable ? FLinearColor(0.85f, 0.85f, 0.85f, 1.0f) : FLinearColor(0.45f, 0.45f, 0.45f, 0.6f);
}

bool SAudioResultView::IsSampleCycleEnabled() const
{
    return SoundWaves.Num() > 1;
}

EVisibility SAudioResultView::GetLoadingTextVisibility() const
{
    return bIsLoading ? EVisibility::Visible : EVisibility::Collapsed;
}

EVisibility SAudioResultView::GetWaveformVisibility() const
{
    // 로딩 중에는 파형 숨김
    return bIsLoading ? EVisibility::Collapsed : EVisibility::Visible;
}

FText SAudioResultView::GetPlaybackTimeText() const
{
    if (!SoundWaves.IsValidIndex(CurrentSoundWaveIndex))
    {
        return FText::GetEmpty();
    }
    if (CurrentTotalDurationSeconds <= 0.0f)
    {
        return FText::GetEmpty();
    }

    const float CurrentSeconds = CurrentPlaybackPercent * CurrentTotalDurationSeconds;
    const FString CurrentText = FormatDetailedTimestamp(CurrentSeconds);
    const FString TotalText = FormatDetailedTimestamp(CurrentTotalDurationSeconds);
    return FText::FromString(FString::Printf(TEXT("%s / %s"), *CurrentText, *TotalText));
}

EVisibility SAudioResultView::GetPlaybackTimeVisibility() const
{
    const bool bHasSound = SoundWaves.IsValidIndex(CurrentSoundWaveIndex) && SoundWaves[CurrentSoundWaveIndex] != nullptr;
    return (bHasSound && !bIsLoading && CurrentTotalDurationSeconds > 0.0f) ? EVisibility::Visible : EVisibility::Collapsed;
}

FText SAudioResultView::GetLoadingStatusText() const
{
    if (!bIsLoading)
    {
        return FText::GetEmpty();
    }
    const double now = FPlatformTime::Seconds();
    const double elapsed = FMath::Max(0.0, now - LoadingStartSeconds);
    // 소수 첫째 자리만
    const double tenths = FMath::FloorToDouble(elapsed * 10.0 + 0.5) / 10.0;
    return FText::FromString(FString::Printf(TEXT("Generating %.1fs"), tenths));
}

void SAudioResultView::SetTitle(const FText& InTitle)
{
    TitleText = InTitle;
}

FText SAudioResultView::GetTitleText() const
{
    return TitleText;
}

bool SAudioResultView::IsSaveButtonEnabled() const
{
	if (TabType == EAudioResultViewTabType::BackgroundMusic)
	{
		return true;
	}
    return SoundWaves.IsValidIndex(CurrentSoundWaveIndex) && SoundWaves[CurrentSoundWaveIndex] != nullptr;
}

FReply SAudioResultView::OnSaveButtonClicked()
{
	if (TabType == EAudioResultViewTabType::BackgroundMusic)
	{
		const FString Dir = VarcoSoundPathUtils::GetBgmOutputDir();
		VarcoSoundPathUtils::EnsureDirectoryExists(Dir);
		FPlatformProcess::ExploreFolder(*Dir);

		const FString StatusMessage = FString::Printf(TEXT("Open folder: %s"), *Dir);
		FNotificationInfo Info(FText::FromString(StatusMessage));
		Info.bFireAndForget = true;
		Info.FadeOutDuration = 0.5f;
		Info.ExpireDuration = 3.0f;
		Info.bUseThrobber = false;
		TSharedPtr<SNotificationItem> Notification = FSlateNotificationManager::Get().AddNotification(Info);
		if (Notification.IsValid())
		{
			Notification->SetCompletionState(SNotificationItem::CS_None);
		}
		return FReply::Handled();
	}

    if (!SoundWaves.IsValidIndex(CurrentSoundWaveIndex) || !SoundWaves[CurrentSoundWaveIndex])
    {
        return FReply::Handled();
    }

    USoundWave* CurrentSoundWave = SoundWaves[CurrentSoundWaveIndex];
    
    // 탭별 파일명 생성
    FString DefaultName = GenerateFilename(CurrentSoundWaveIndex);
    
    // Set save path (using settings)
    FString WavDir = VarcoSoundPathUtils::GetOutputDirForTab(TabType);
    FString WavFilePath = FPaths::Combine(WavDir, DefaultName + TEXT(".wav"));
    
    // 디렉토리 생성
    VarcoSoundPathUtils::EnsureDirectoryExists(WavDir);
    
    // Save WAV file
    bool bWavSaved = SaveAsWavFile(CurrentSoundWave, WavFilePath);
    
    if (bWavSaved)
    {
        // 성공 토스트 (녹색 체크)
        const FString StatusMessage = FString::Printf(TEXT("WAV saved: %s"), *WavFilePath);
        FNotificationInfo Info(FText::FromString(StatusMessage));
        Info.bFireAndForget = true;
        Info.FadeOutDuration = 0.5f;
        Info.ExpireDuration = 3.0f;
        Info.bUseThrobber = false;
        TSharedPtr<SNotificationItem> Notification = FSlateNotificationManager::Get().AddNotification(Info);
        if (Notification.IsValid())
        {
            Notification->SetCompletionState(SNotificationItem::CS_Success);
        }
    }
    else
    {
        // 실패 토스트 (빨간 실패 아이콘)
        FNotificationInfo Info(FText::FromString(TEXT("Failed to save WAV file.")));
        Info.bFireAndForget = true;
        Info.FadeOutDuration = 0.5f;
        Info.ExpireDuration = 3.0f;
        Info.bUseThrobber = false;
        TSharedPtr<SNotificationItem> Notification = FSlateNotificationManager::Get().AddNotification(Info);
        if (Notification.IsValid())
        {
            Notification->SetCompletionState(SNotificationItem::CS_Fail);
        }
    }
    
    return FReply::Handled();
}

bool SAudioResultView::SaveAsWavFile(USoundWave* SoundWave, const FString& FilePath)
{
    if (!SoundWave)
    {
        return false;
    }
    
    // Extract audio data directly using RawPCMData
    if (!SoundWave->RawPCMData || SoundWave->RawPCMDataSize == 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("SoundWave has no RawPCMData"));
        return false;
    }
    
    // WAV 파일 헤더 구조체
    struct WAVHeader
    {
        char RIFF[4] = {'R', 'I', 'F', 'F'};
        uint32 ChunkSize;
        char WAVE[4] = {'W', 'A', 'V', 'E'};
        char fmt[4] = {'f', 'm', 't', ' '};
        uint32 Subchunk1Size = 16;
        uint16 AudioFormat = 1; // PCM
        uint16 NumChannels;
        uint32 SampleRate;
        uint32 ByteRate;
        uint16 BlockAlign;
        uint16 BitsPerSample = 16;
        char data[4] = {'d', 'a', 't', 'a'};
        uint32 Subchunk2Size;
    };
    
    WAVHeader Header;
    Header.NumChannels = SoundWave->NumChannels;
    Header.SampleRate = SoundWave->GetSampleRateForCurrentPlatform();
    Header.BitsPerSample = 16;
    Header.ByteRate = Header.SampleRate * Header.NumChannels * Header.BitsPerSample / 8;
    Header.BlockAlign = Header.NumChannels * Header.BitsPerSample / 8;
    Header.Subchunk2Size = SoundWave->RawPCMDataSize;
    Header.ChunkSize = 36 + Header.Subchunk2Size;
    
    // 최종 파일 데이터 구성
    TArray<uint8> FileData;
    FileData.SetNum(sizeof(WAVHeader) + Header.Subchunk2Size);
    
    // 헤더 복사
    FMemory::Memcpy(FileData.GetData(), &Header, sizeof(WAVHeader));
    
    // Copy audio data
    FMemory::Memcpy(FileData.GetData() + sizeof(WAVHeader), SoundWave->RawPCMData, Header.Subchunk2Size);
    
    return FFileHelper::SaveArrayToFile(FileData, *FilePath);
}

bool SAudioResultView::HasSoundWaves() const
{
    return SoundWaves.Num() > 0;
}

void SAudioResultView::SetTabType(EAudioResultViewTabType InTabType)
{
    TabType = InTabType;
}

void SAudioResultView::SetPromptText(const FString& InPromptText)
{
    // 띄어쓰기를 언더바로 변경
    PromptText = InPromptText;
    PromptText = PromptText.Replace(TEXT(" "), TEXT("_"));
}

void SAudioResultView::SetSourceFilename(const FString& InSourceFilename)
{
    // Remove extension and store filename only
    SourceFilename = FPaths::GetBaseFilename(InSourceFilename);
}

void SAudioResultView::SetTargetFilename(const FString& InTargetFilename)
{
    // Remove extension and store filename only
    TargetFilename = FPaths::GetBaseFilename(InTargetFilename);
}

void SAudioResultView::SetTrimmingParameters(float InVadThreshold, float InPreRollSeconds, float InPostRollSeconds, float InFrameDurationSeconds)
{
    TrimVadThreshold = InVadThreshold;
    TrimPreRollSeconds = InPreRollSeconds;
    TrimPostRollSeconds = InPostRollSeconds;
    TrimFrameDurationSeconds = InFrameDurationSeconds;
}

void SAudioResultView::SetLoading(bool bInLoading)
{
    bIsLoading = bInLoading;
    if (bIsLoading)
    {
        LoadingStartSeconds = FPlatformTime::Seconds();
    }
    Invalidate(EInvalidateWidget::Layout);
}

FString SAudioResultView::GenerateFilename(int32 AudioIndex) const
{
    FString Filename;
    
    switch (TabType)
    {
        case EAudioResultViewTabType::Generator:
            // gen_[prompt]_##.wav
            {
                FString CleanPrompt = VarcoSoundPathUtils::SanitizeFilename(PromptText, 40);
                Filename = FString::Printf(TEXT("gen_%s_%02d"), *CleanPrompt, AudioIndex + 1);
            }
            break;
        
        case EAudioResultViewTabType::AutoLoop:
            // loop_[inputfilename].wav
            {
                FString CleanSource = VarcoSoundPathUtils::SanitizeFilename(SourceFilename, 50);
                Filename = FString::Printf(TEXT("loop_%s"), *CleanSource);
            }
            break;
        
        case EAudioResultViewTabType::AutoVariation:
            // var_[inputfilename]_##.wav
            {
                FString CleanSource = VarcoSoundPathUtils::SanitizeFilename(SourceFilename, 50);
                Filename = FString::Printf(TEXT("var_%s_%02d"), *CleanSource, AudioIndex + 1);
            }
            break;
        
        case EAudioResultViewTabType::MonsterVoice:
            // conv_[sourcefilename]_[targetfilename].wav
            // 전체 파일명이 너무 길지 않도록 각 파일명을 20자로 제한
            {
                FString CleanSource = VarcoSoundPathUtils::SanitizeFilename(SourceFilename, 20);
                FString CleanTarget = VarcoSoundPathUtils::SanitizeFilename(TargetFilename, 20);
                Filename = FString::Printf(TEXT("conv_%s2%s"), *CleanSource, *CleanTarget);
            }
            break;

		case EAudioResultViewTabType::BackgroundMusic:
			// bgm_##.wav (rare: BGM is auto-saved as mp3; Save button is repurposed to open folder)
			Filename = FString::Printf(TEXT("bgm_%02d"), AudioIndex + 1);
			break;
        
        default:
            Filename = FString::Printf(TEXT("audio_%d"), FDateTime::Now().ToUnixTimestamp());
            break;
    }
    
    return Filename;
}

void SAudioResultView::Play()
{
    if (SoundWaves.IsValidIndex(CurrentSoundWaveIndex) && AudioPlayer)
    {
        AudioPlayer->Play();
    }
}

void SAudioResultView::Pause()
{
    if (AudioPlayer)
    {
        AudioPlayer->Pause();
    }
}

void SAudioResultView::HandleWaveformSeek(float Percent)
{
    const float p = FMath::Clamp(Percent, 0.f, 1.f);
    CurrentPlaybackPercent = p;
    if (WaveformDisplayWidget.IsValid())
    {
        WaveformDisplayWidget->SetPlaybackPercentage(p);
    }

	const float startSec = p * CurrentTotalDurationSeconds;
	UE_LOG(LogTemp, Log, TEXT("[ARV::Seek] pct=%.3f start=%.3f playing=%d paused=%d src=%s"),
		p, startSec,
		(AudioPlayer && AudioPlayer->IsPlaying()) ? 1 : 0,
		(AudioPlayer && AudioPlayer->IsPaused()) ? 1 : 0,
		*CurrentSourceTag);

	LastPlayStartPercent = p;
    if (AudioPlayer && AudioPlayer->IsPlaying())
    {
        bSeenProgressThisPlay = false;
        LastPlayRequestSeconds = FPlatformTime::Seconds();
		bPendingSeekWhilePaused = false;
		bHasActiveStartOffset = (LastPlayStartPercent > 0.0f);
        AudioPlayer->Play(startSec);
    }
    else if (AudioPlayer)
    {
        // 정지/일시정지 중에는 커서만 이동 표시하고, 다음 Play에서 해당 지점부터 시작하도록 표식
        bPendingSeekWhilePaused = true;
		bHasActiveStartOffset = (LastPlayStartPercent > 0.0f);
    }
}

void SAudioResultView::HandleWaveformTogglePlayPause()
{
    if (!AudioPlayer) return;
    if (AudioPlayer->IsPlaying())
    {
        AudioPlayer->Pause();
        return;
    }
    
    // 재생 시작
    bSeenProgressThisPlay = false;
    LastPlayRequestSeconds = FPlatformTime::Seconds();
	const bool bShouldRestart = bPendingSeekWhilePaused;
    const float StartPercent = bShouldRestart ? LastPlayStartPercent : CurrentPlaybackPercent;
    const float startSec = StartPercent * CurrentTotalDurationSeconds;
	if (bShouldRestart)
	{
		bPendingSeekWhilePaused = false;
		CurrentPlaybackPercent = StartPercent;
		AudioPlayer->Stop();
	}
	LastPlayStartPercent = StartPercent;
	bHasActiveStartOffset = (StartPercent > 0.0f);
	UE_LOG(LogTemp, Log, TEXT("[ARV::Toggle] Play startPct=%.3f offset=%d"), StartPercent, bHasActiveStartOffset ? 1 : 0);
	AudioPlayer->Play(startSec);
}

END_SLATE_FUNCTION_BUILD_OPTIMIZATION 