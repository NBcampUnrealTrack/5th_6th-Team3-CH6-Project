// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBox.h"
#include "UI/SWaveformDisplay.h"
#include "Utils/AudioPlayer.h"

class USoundWave;
class SVarcoSoundIcon;
enum class EVarcoSoundIconType : uint8;
 
 /**
  * Reusable audio player component
  * Includes waveform display + play/stop buttons
  */
 class VARCOSOUND_API SAudioPlayerControls : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SAudioPlayerControls)
        : _WaveformHeight(100.0f)
        , _ShowTitle(true)
        , _Title(FText::FromString(TEXT("Audio Player")))
    {}
        SLATE_ARGUMENT(float, WaveformHeight)
        SLATE_ARGUMENT(bool, ShowTitle)
        SLATE_ARGUMENT(FText, Title)
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs);

    // Audio setting
    void SetSoundWave(USoundWave* InSoundWave);
	void SetSourceTag(const FString& InTag) { CurrentSourceTag = InTag; }
    void SetTitle(const FText& InTitle);

    // Real-time waveform/additional button control
    void SetLiveWaveform(const TArray<float>& InSamples, int32 NumChannels);
    void SetAuxiliaryControls(const TSharedPtr<SWidget>& InWidget);
    void ClearAuxiliaryControls();

    // Playback state control
    void Play();
    void Pause();
    void Stop();
    bool IsPlaying() const;

private:
    // UI widgets
    TSharedPtr<SWaveformDisplay> WaveformDisplayWidget;
    TSharedPtr<SButton> PlayButton;
    TSharedPtr<SButton> StopButton;
    TSharedPtr<SVarcoSoundIcon> PlayIconWidget;
    TSharedPtr<SBox> AuxiliaryControlSlot;
    
    // Current sound
    TObjectPtr<USoundWave> CurrentSoundWave;

    // Instance audio player
    TUniquePtr<FAudioPlayer> AudioPlayer;
     
     // Title
     FText TitleText;
    bool bShowTitle;
    
    // Callback functions
    FReply OnPlayButtonClicked();
    FReply OnStopButtonClicked();
    EVarcoSoundIconType GetPlayButtonIconType() const;
    FLinearColor GetPlayButtonTint() const;
    FText GetTitleText() const;
    EVisibility GetTitleVisibility() const;
    FText GetPlaybackTimeText() const;
    EVisibility GetPlaybackTimeVisibility() const;
    
    // Audio playback callback
    void OnPlaybackPercentChanged(const UAudioComponent* InAudioComponent, const USoundWave* PlayingSoundWave, float PlaybackPercent);
    
    // Waveform and sound update
    void UpdateWaveformAndSound();

    // Playback state
    float CurrentPlaybackPercent = 0.0f;
    bool bPlaybackCompletionHandled = false;
	float LastPlayStartPercent = 0.0f;
	bool bPendingSeekWhilePaused = false;
	FString CurrentSourceTag = TEXT("Unknown");

    // Waveform interaction handler (waveform click/drag seeking, spacebar toggle)
    void HandleWaveformSeek(float Percent);
    void HandleWaveformTogglePlayPause();
}; 