// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/SAudioPlayerControls.h"
#include "UI/SWaveformDisplay.h"
#include "UI/SVarcoSoundIconWidget.h"
#include "Utils/AudioUtils.h"
#include "Utils/AudioUIHelpers.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Text/STextBlock.h"
#include "Styling/AppStyle.h"
#include "Widgets/SNullWidget.h"
#include "SlateOptMacros.h"
#include "Sound/SoundWave.h"
#include "Sound/SoundWaveProcedural.h"
#include "Components/AudioComponent.h"
#include "Framework/Application/SlateApplication.h"
#include "InputCoreTypes.h"
#include "Delegates/DelegateCombinations.h"

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION

void SAudioPlayerControls::Construct(const FArguments& InArgs)
{
    TitleText = InArgs._Title;
    bShowTitle = InArgs._ShowTitle;
    float WaveformHeight = InArgs._WaveformHeight;
    
    CurrentSoundWave = nullptr;
    AudioPlayer = MakeUnique<FAudioPlayer>();

    ChildSlot
    [
        SNew(SVerticalBox)

        // Title (optional)
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(0, 0, 0, 5)
        [
            SNew(STextBlock)
            .Text(this, &SAudioPlayerControls::GetTitleText)
            .Font(FCoreStyle::GetDefaultFontStyle("Normal", 10))
            .Visibility(this, &SAudioPlayerControls::GetTitleVisibility)
        ]

        // 파형 표시 영역
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(0, 0, 0, 5)
        [
            SNew(SBorder)
            .BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
            .Padding(FMargin(10.0f, 6.0f))
            [
                SNew(SBox)
                .HeightOverride(WaveformHeight)
                [
                    SNew(SBorder)
                    .BorderImage(FAppStyle::GetBrush("ToolPanel.DarkGroupBorder"))
                    .BorderBackgroundColor(FLinearColor(0.05f, 0.05f, 0.05f, 1.0f))
                    [
                        SNew(SOverlay)
                        + SOverlay::Slot()
                        [
                            SAssignNew(WaveformDisplayWidget, SWaveformDisplay)
                        ]
                        + SOverlay::Slot()
                        .HAlign(HAlign_Right)
                        .VAlign(VAlign_Bottom)
                        [
                            SNew(SBox)
                            .Padding(FMargin(0.f, 0.f, 6.f, 4.f))
                            [
                                SNew(STextBlock)
                                .Text(this, &SAudioPlayerControls::GetPlaybackTimeText)
                                .Visibility(this, &SAudioPlayerControls::GetPlaybackTimeVisibility)
                                .Font(FCoreStyle::GetDefaultFontStyle("Normal", 9))
                                .ColorAndOpacity(FLinearColor(0.75f, 0.75f, 0.75f, 1.0f))
                                .ShadowColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, 0.6f))
                                .ShadowOffset(FVector2D(1.f, 1.f))
                            ]
                        ]
                    ]
                ]
            ]
        ]

        // 재생 컨트롤 버튼들
        + SVerticalBox::Slot()
        .AutoHeight()
        .HAlign(HAlign_Center)
        [
            SNew(SHorizontalBox)

            // 정지 버튼
            + SHorizontalBox::Slot()
            .AutoWidth()
            .Padding(2, 0)
            [
                SAssignNew(StopButton, SButton)
                .ButtonStyle(&FAppStyle::Get().GetWidgetStyle<FButtonStyle>("SimpleButton"))
                .ContentPadding(FMargin(6.f))
                .OnClicked(this, &SAudioPlayerControls::OnStopButtonClicked)
                .IsFocusable(false)
                [
                    SNew(SVarcoSoundIcon)
                    .IconType(EVarcoSoundIconType::Stop)
                    .IconColor(FLinearColor(0.85f, 0.85f, 0.85f, 1.0f))
                    .IconSize(FVector2D(13.f, 13.f))
                ]
            ]

            // 재생/일시정지 버튼
            + SHorizontalBox::Slot()
            .AutoWidth()
            .Padding(2, 0)
            [
                SAssignNew(PlayButton, SButton)
                .ButtonStyle(&FAppStyle::Get().GetWidgetStyle<FButtonStyle>("SimpleButton"))
                .ContentPadding(FMargin(6.f))
                .OnClicked(this, &SAudioPlayerControls::OnPlayButtonClicked)
                .IsFocusable(false)
                [
                    SAssignNew(PlayIconWidget, SVarcoSoundIcon)
                    .IconType(this, &SAudioPlayerControls::GetPlayButtonIconType)
                    .IconColor(this, &SAudioPlayerControls::GetPlayButtonTint)
                    .IconSize(FVector2D(13.f, 13.f))
                ]
            ]

            // External control button (optional)
            + SHorizontalBox::Slot()
            .AutoWidth()
            .Padding(2, 0)
            [
                SAssignNew(AuxiliaryControlSlot, SBox)
                .Visibility(EVisibility::Collapsed)
            ]
        ]
    ];

    if (WaveformDisplayWidget.IsValid())
    {
        WaveformDisplayWidget->SetOnSeekRequested(SWaveformDisplay::FOnSeekRequested::CreateSP(this, &SAudioPlayerControls::HandleWaveformSeek));
        WaveformDisplayWidget->SetOnTogglePlayPauseRequested(SWaveformDisplay::FOnTogglePlayPauseRequested::CreateSP(this, &SAudioPlayerControls::HandleWaveformTogglePlayPause));
    }
}

void SAudioPlayerControls::SetSoundWave(USoundWave* InSoundWave)
{
    // Stop existing audio (this widget instance only)
    if (AudioPlayer)
    {
        AudioPlayer->Stop();
    }
    
    CurrentSoundWave = InSoundWave;
	CurrentPlaybackPercent = 0.0f;
	LastPlayStartPercent = 0.0f;
	bPendingSeekWhilePaused = false;
	if (CurrentSourceTag == TEXT("Unknown") && CurrentSoundWave)
	{
		CurrentSourceTag = CurrentSoundWave->IsAsset() ? TEXT("ContentBrowser") : TEXT("Runtime");
	}
	if (AudioPlayer)
	{
		AudioPlayer->SetSourceTag(CurrentSourceTag);
	}
    bPlaybackCompletionHandled = false;
    UpdateWaveformAndSound();
}

void SAudioPlayerControls::SetTitle(const FText& InTitle)
{
    TitleText = InTitle;
}

void SAudioPlayerControls::Play()
{
    if (CurrentSoundWave && AudioPlayer)
    {
        bPlaybackCompletionHandled = false;
        AudioPlayer->Play();
    }
}

void SAudioPlayerControls::Pause()
{
    if (AudioPlayer)
    {
        AudioPlayer->Pause();
    }
}

void SAudioPlayerControls::Stop()
{
    if (AudioPlayer)
    {
        FAudioUIHelpers::StopAndReset(*AudioPlayer, WaveformDisplayWidget.Get(), CurrentPlaybackPercent, PlayButton);
    }
    bPlaybackCompletionHandled = false;
	bPendingSeekWhilePaused = false;
	LastPlayStartPercent = 0.0f;
}

bool SAudioPlayerControls::IsPlaying() const
{
    return AudioPlayer ? AudioPlayer->IsPlaying() : false;
}

FReply SAudioPlayerControls::OnPlayButtonClicked()
{
    if (!CurrentSoundWave)
    {
        return FReply::Handled();
    }

    if (AudioPlayer)
    {
        bPlaybackCompletionHandled = false;
        const float total = FMath::Max(0.f, CurrentSoundWave->Duration);
		const bool bIsPlaying = AudioPlayer->IsPlaying();
		
		if (bIsPlaying)
		{
			UE_LOG(LogTemp, Log, TEXT("[APC::OnPlay] Pause pct=%.3f"), CurrentPlaybackPercent);
			AudioPlayer->Pause();
		}
		else
		{
			const float StartPercent = bPendingSeekWhilePaused ? LastPlayStartPercent : CurrentPlaybackPercent;
			const float startSec = FMath::Clamp(StartPercent, 0.f, 1.f) * total;
			UE_LOG(LogTemp, Log, TEXT("[APC::OnPlay] Play pct=%.3f start=%.3f pending=%d"), StartPercent, startSec, bPendingSeekWhilePaused ? 1 : 0);
			if (bPendingSeekWhilePaused)
			{
				bPendingSeekWhilePaused = false;
				CurrentPlaybackPercent = StartPercent;
				AudioPlayer->Stop();
			}
			AudioPlayer->Play(startSec);
		}
    }

    if (PlayButton.IsValid())
    {
        PlayButton->Invalidate(EInvalidateWidget::Paint);
    }

    return FReply::Handled();
}

FReply SAudioPlayerControls::OnStopButtonClicked()
{
    UE_LOG(LogTemp, Log, TEXT("[APC::OnStop] pct=%.3f"), CurrentPlaybackPercent);
    Stop();
    return FReply::Handled();
}

EVarcoSoundIconType SAudioPlayerControls::GetPlayButtonIconType() const
{
    if (AudioPlayer && AudioPlayer->IsPlaying())
    {
        return EVarcoSoundIconType::Pause;
    }
    return EVarcoSoundIconType::Play;
}

FLinearColor SAudioPlayerControls::GetPlayButtonTint() const
{
    const bool bHasPlayableAudio = (CurrentSoundWave != nullptr);
    return bHasPlayableAudio ? FLinearColor(0.85f, 0.85f, 0.85f, 1.0f) : FLinearColor(0.45f, 0.45f, 0.45f, 0.6f);
}

FText SAudioPlayerControls::GetTitleText() const
{
    return TitleText;
}

EVisibility SAudioPlayerControls::GetTitleVisibility() const
{
    return bShowTitle ? EVisibility::Visible : EVisibility::Collapsed;
}

void SAudioPlayerControls::OnPlaybackPercentChanged(const UAudioComponent* InAudioComponent, const USoundWave* PlayingSoundWave, float PlaybackPercent)
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
    UE_LOG(LogTemp, Verbose, TEXT("[APC::Pct] pct=%.3f src=%s"), PlaybackPercent, AudioPlayer ? *AudioPlayer->GetSourceTag() : TEXT("Unknown"));
    if (WaveformDisplayWidget.IsValid())
    {
        WaveformDisplayWidget->SetPlaybackPercentage(PlaybackPercent);
    }
    CurrentPlaybackPercent = FMath::Clamp(PlaybackPercent, 0.0f, 1.0f);

    // Procedural이 아닌 일반 SoundWave의 재생 종료 감지 (Procedural은 FAudioPlayer에서 처리)
    if (AudioPlayer && CurrentSoundWave && CurrentPlaybackPercent >= 0.999f && !bPlaybackCompletionHandled)
    {
        // Procedural이 아닌 경우에만 여기서 처리
        if (!Cast<USoundWaveProcedural>(CurrentSoundWave))
        {
            if (!AudioPlayer->IsPlaying())
            {
                UE_LOG(LogTemp, Log, TEXT("[APC::Pct] Non-procedural playback completed"));
                bPlaybackCompletionHandled = true;
                CurrentPlaybackPercent = 0.0f;
                if (WaveformDisplayWidget.IsValid())
                {
                    WaveformDisplayWidget->SetPlaybackPercentage(0.0f);
                }
                if (PlayButton.IsValid())
                {
                    PlayButton->Invalidate(EInvalidateWidget::Paint);
                }
            }
        }
    }
}

void SAudioPlayerControls::UpdateWaveformAndSound()
{
    if (WaveformDisplayWidget.IsValid())
    {
        if (CurrentSoundWave)
        {
            // 파형 업데이트 (스테레오/모노 자동)
            FAudioUIHelpers::UpdateWaveformDisplay(WaveformDisplayWidget.Get(), CurrentSoundWave);

            // Set audio for playback (instance player)
            if (AudioPlayer)
            {
                FOnAudioPlaybackPercentNative::FDelegate PlaybackPercentDelegate;
                PlaybackPercentDelegate.BindSP(this, &SAudioPlayerControls::OnPlaybackPercentChanged);

                TWeakPtr<SAudioPlayerControls> SelfWeak = SharedThis(this);
				AudioPlayer->SetSourceTag(CurrentSourceTag);
                FAudioUIHelpers::ConfigurePlayer(
                    *AudioPlayer,
                    CurrentSoundWave,
                    PlaybackPercentDelegate,
                    /*bLooping*/ false,
                    [SelfWeak]()
                    {
                        if (TSharedPtr<SAudioPlayerControls> Pinned = SelfWeak.Pin())
                        {
                            if (Pinned->WaveformDisplayWidget.IsValid())
                            {
                                Pinned->WaveformDisplayWidget->SetPlaybackPercentage(0.0f);
                            }
                            Pinned->CurrentPlaybackPercent = 0.0f;
                            Pinned->bPlaybackCompletionHandled = true;
                            if (Pinned->PlayButton.IsValid())
                            {
                                Pinned->PlayButton->Invalidate(EInvalidateWidget::Paint);
                            }
                        }
                    }
                );
            }
        }
        else
        {
            WaveformDisplayWidget->SetWaveformData(TArray<float>());
            if (AudioPlayer)
            {
                AudioPlayer->SetSound(nullptr);
            }
        }
    }   
}

FText SAudioPlayerControls::GetPlaybackTimeText() const
{
    if (!CurrentSoundWave)
    {
        return FText::GetEmpty();
    }
    const float total = FMath::Max(0.0f, CurrentSoundWave->Duration);
    const float current = FMath::Clamp(CurrentPlaybackPercent * total, 0.0f, total);
    return FAudioUIHelpers::MakeProgressText(current, total);
}

EVisibility SAudioPlayerControls::GetPlaybackTimeVisibility() const
{
    return (CurrentSoundWave && CurrentSoundWave->Duration > 0.0f) ? EVisibility::Visible : EVisibility::Collapsed;
}

// 파형 시킹 핸들러 - SWaveformDisplay 델리게이트에서 호출됨

void SAudioPlayerControls::HandleWaveformSeek(float Percent)
{
  const float p = FMath::Clamp(Percent, 0.f, 1.f);
  CurrentPlaybackPercent = p;
  if (WaveformDisplayWidget.IsValid())
  {
    WaveformDisplayWidget->SetPlaybackPercentage(p);
  }
  if (AudioPlayer && CurrentSoundWave)
  {
    const float total = FMath::Max(0.f, CurrentSoundWave->Duration);
	UE_LOG(LogTemp, Log, TEXT("[APC::Seek] pct=%.3f start=%.3f playing=%d src=%s"),
		p, p * total, AudioPlayer->IsPlaying() ? 1 : 0, *AudioPlayer->GetSourceTag());
	LastPlayStartPercent = p;
	if (AudioPlayer->IsPlaying())
	{
		bPendingSeekWhilePaused = false;
		AudioPlayer->Play(p * total);
	}
	else
	{
		bPendingSeekWhilePaused = true;
	}
  }
}

void SAudioPlayerControls::HandleWaveformTogglePlayPause()
{
  if (!AudioPlayer || !CurrentSoundWave) return;
  if (AudioPlayer->IsPlaying())
  {
    AudioPlayer->Pause();
  }
  else
  {
    const float total = FMath::Max(0.f, CurrentSoundWave->Duration);
	const bool bShouldRestart = bPendingSeekWhilePaused;
	const float StartPercent = bShouldRestart ? LastPlayStartPercent : CurrentPlaybackPercent;
	const float StartSec = StartPercent * total;
	if (bShouldRestart)
	{
		bPendingSeekWhilePaused = false;
		CurrentPlaybackPercent = StartPercent;
		AudioPlayer->Stop();
	}
	AudioPlayer->Play(StartSec);
  }
}

void SAudioPlayerControls::SetLiveWaveform(const TArray<float>& InSamples, int32 NumChannels)
{
    if (!WaveformDisplayWidget.IsValid())
    {
        return;
    }

    if (NumChannels >= 2)
    {
        WaveformDisplayWidget->SetStereoWaveformData(InSamples);
    }
    else
    {
        WaveformDisplayWidget->SetWaveformData(InSamples);
    }
}

void SAudioPlayerControls::SetAuxiliaryControls(const TSharedPtr<SWidget>& InWidget)
{
    if (!AuxiliaryControlSlot.IsValid())
    {
        return;
    }

    if (InWidget.IsValid())
    {
        AuxiliaryControlSlot->SetContent(InWidget.ToSharedRef());
        AuxiliaryControlSlot->SetVisibility(EVisibility::Visible);
    }
    else
    {
        ClearAuxiliaryControls();
    }
}

void SAudioPlayerControls::ClearAuxiliaryControls()
{
    if (!AuxiliaryControlSlot.IsValid())
    {
        return;
    }

    AuxiliaryControlSlot->SetContent(SNullWidget::NullWidget);
    AuxiliaryControlSlot->SetVisibility(EVisibility::Collapsed);
}


END_SLATE_FUNCTION_BUILD_OPTIMIZATION 