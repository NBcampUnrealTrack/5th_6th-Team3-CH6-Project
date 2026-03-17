// Copyright Epic Games, Inc. All Rights Reserved.

#include "Utils/AudioUIHelpers.h"
#include "UI/SWaveformDisplay.h"
#include "Utils/AudioUtils.h"
#include "Utils/AudioPlayer.h"
#include "Sound/SoundWave.h"
#include "Widgets/Input/SButton.h"

void FAudioUIHelpers::UpdateWaveformDisplay(SWaveformDisplay* WaveformDisplay, USoundWave* SoundWave)
{
    if (!WaveformDisplay)
    {
        return;
    }

    if (SoundWave)
    {
        const bool bIsStereo = SoundWave->NumChannels >= 2;
        TArray<float> WaveformData;

        if (bIsStereo)
        {
            if (SoundWave->RawPCMData && SoundWave->RawPCMDataSize > 0)
            {
                WaveformData = FAudioUtils::ExtractStereoWaveformData(SoundWave);
            }
            else
            {
                WaveformData = FAudioUtils::ExtractStereoWaveformFromContentBrowserAsset(SoundWave);
            }
            WaveformDisplay->SetStereoWaveformData(WaveformData);
        }
        else
        {
            if (SoundWave->RawPCMData && SoundWave->RawPCMDataSize > 0)
            {
                WaveformData = FAudioUtils::ExtractWaveformData(SoundWave);
            }
            else
            {
                WaveformData = FAudioUtils::ExtractWaveformFromContentBrowserAsset(SoundWave);
            }
            WaveformDisplay->SetWaveformData(WaveformData);
        }
    }
    else
    {
        WaveformDisplay->SetWaveformData(TArray<float>());
    }
}

void FAudioUIHelpers::ConfigurePlayer(
    FAudioPlayer& Player,
    USoundWave* SoundWave,
    FOnAudioPlaybackPercentNative::FDelegate OnPlaybackPercent,
    bool bLooping,
    TFunction<void()>&& OnFinished)
{
    Player.SetSound(SoundWave, OnPlaybackPercent);
    Player.SetLooping(bLooping);
    Player.SetOnFinished(MoveTemp(OnFinished));
}

FString FAudioUIHelpers::FormatTimeMMMSSmmm(float Seconds)
{
    if (!FMath::IsFinite(Seconds))
    {
        return FString(TEXT("00.00"));
    }

    const int32 TotalHundredths = FMath::Max(0, static_cast<int32>(FMath::RoundHalfFromZero(Seconds * 100.0f)));
    const int32 WholeSeconds = TotalHundredths / 100;
    const int32 Hundredths = TotalHundredths % 100;

    return FString::Printf(TEXT("%02d.%02d"), WholeSeconds, Hundredths);
}

FText FAudioUIHelpers::MakeProgressText(float CurrentSeconds, float TotalSeconds)
{
    const FString Left = FormatTimeMMMSSmmm(CurrentSeconds);
    const FString Right = FormatTimeMMMSSmmm(TotalSeconds);
    return FText::FromString(FString::Printf(TEXT("%s / %s"), *Left, *Right));
}

void FAudioUIHelpers::TogglePlayPause(FAudioPlayer& Player)
{
    if (Player.IsPlaying())
    {
        Player.Pause();
    }
    else
    {
        Player.Play();
    }
}

void FAudioUIHelpers::StopAndReset(FAudioPlayer& Player, SWaveformDisplay* WaveformDisplay, float& InOutPlaybackPercent, const TSharedPtr<SButton>& PlayButtonToInvalidate)
{
    Player.Stop();
    if (WaveformDisplay)
    {
        WaveformDisplay->SetPlaybackPercentage(0.0f);
    }
    InOutPlaybackPercent = 0.0f;
    if (PlayButtonToInvalidate.IsValid())
    {
        PlayButtonToInvalidate->Invalidate(EInvalidateWidget::Paint);
    }
}

FText FAudioUIHelpers::GetPlayButtonTextFor(const FAudioPlayer& Player)
{
    return Player.IsPlaying() ? FText::FromString(TEXT("❚❚")) : FText::FromString(TEXT("▶"));
}


