// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Delegates/DelegateCombinations.h"
#include "Components/AudioComponent.h"

class USoundWave;
class SWaveformDisplay;
class FAudioPlayer;
class SButton;

/**
 * UI level audio helper: waveform display/player binding/time format etc. common logic
 */
class FAudioUIHelpers
{
public:
    /** Extract waveform data from given sound and display it on the display (stereo/mono auto-processed) */
    static void UpdateWaveformDisplay(SWaveformDisplay* WaveformDisplay, USoundWave* SoundWave);

    /** Configure player with sound, percent/completion callback and loop options */
    static void ConfigurePlayer(
        FAudioPlayer& Player,
        USoundWave* SoundWave,
        FOnAudioPlaybackPercentNative::FDelegate OnPlaybackPercent,
        bool bLooping,
        TFunction<void()>&& OnFinished);

    /** Format seconds to "SS.HH" (always two decimal places) string */
    static FString FormatTimeMMMSSmmm(float Seconds);

    /** Create progress text "left / right" */
    static FText MakeProgressText(float CurrentSeconds, float TotalSeconds);

    /** Toggle play/pause (call Pause or Play based on current state) */
    static void TogglePlayPause(FAudioPlayer& Player);

    /** Stop and reset (update waveform/percent/button text) */
    static void StopAndReset(FAudioPlayer& Player, SWaveformDisplay* WaveformDisplay, float& InOutPlaybackPercent, const TSharedPtr<SButton>& PlayButtonToInvalidate);

    /** Return play button text (▶ / ❚❚) */
    static FText GetPlayButtonTextFor(const FAudioPlayer& Player);
};


