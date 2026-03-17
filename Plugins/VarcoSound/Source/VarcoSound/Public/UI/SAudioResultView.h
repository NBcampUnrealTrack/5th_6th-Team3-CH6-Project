// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "ContentBrowserModule.h"
#include "Utils/AudioPlayer.h"
 
class USoundWave;
class SWaveformDisplay;
class SVarcoSoundIcon;
enum class EVarcoSoundIconType : uint8;
 
 /**
  * 탭 타입을 구분하는 열거형
  */
 UENUM()
 enum class EAudioResultViewTabType : uint8
 {
     Generator,
     AutoLoop,
     AutoVariation,
     MonsterVoice,
	 BackgroundMusic
 };
 
 /**
  * A widget that displays a sound waveform and provides playback controls.
  * It can handle single or multiple sound waves, showing a refresh button if multiple are provided.
  */
 class VARCOSOUND_API SAudioResultView : public SCompoundWidget
 {
 public:
     SLATE_BEGIN_ARGS(SAudioResultView) {}
     SLATE_END_ARGS()
 
     /** Constructs this widget */
     void Construct(const FArguments& InArgs);
 
     /** Destructor */
     ~SAudioResultView();
 
     /**
      * Sets the sound waves to be displayed and managed by this widget.
      * @param InSoundWaves The array of sound waves to display.
      */
     void SetSoundWaves(const TArray<USoundWave*>& InSoundWaves);
	void SetSourceTag(const FString& InTag) { CurrentSourceTag = InTag; }
 
     /**
      * Sets the title to be displayed above the waveform.
      * @param InTitle The title text to display.
      */
     void SetTitle(const FText& InTitle);
 
     /** Resets the playback state (stops audio, resets progress). */
     void ResetPlaybackState();
 
     /**
      * Checks if there are any sound waves loaded.
      * @return True if sound waves are available, false otherwise.
      */
     bool HasSoundWaves() const;
 
     /**
      * Sets the tab type for filename generation purposes.
      * @param InTabType The tab type this widget is being used in.
      */
     void SetTabType(EAudioResultViewTabType InTabType);
 
     /**
      * Sets the prompt text for Generator tab filename generation.
      * @param InPromptText The prompt text used to generate the audio.
      */
     void SetPromptText(const FString& InPromptText);
 
    /**
     * Sets the source filename for Loop/Variation/MonsterVoice tabs.
     * @param InSourceFilename The original filename of the source audio.
     */
    void SetSourceFilename(const FString& InSourceFilename);

    /**
     * Sets the target filename for MonsterVoice tab.
     * @param InTargetFilename The target filename for voice conversion.
     */
    void SetTargetFilename(const FString& InTargetFilename);
 
      /** Set trimming parameters (optional, call from tab if needed) */
     void SetTrimmingParameters(float InVadThreshold, float InPreRollSeconds, float InPostRollSeconds, float InFrameDurationSeconds);
 
     /** Toggle loading state (true when API is waiting) */
     void SetLoading(bool bInLoading);
     
     /** Check current loading state */
     bool IsLoading() const { return bIsLoading; }

     /** Start/pause playback from outside (auto-play/tab switch, etc.) */
     void Play();
     void Pause();
 
    /** Handles the Refresh/Regenerate button click. */
    FReply OnRefreshButtonClicked();

    /**
     * Delegate to handle regeneration requests.
     * The parent widget should bind this to logic that re-sends the API request.
     */
    DECLARE_DELEGATE(FOnRegenerateRequested);
    FOnRegenerateRequested OnRegenerateRequested;

    /** Gets the sample index text (e.g., "1/2"). */
    FText GetSampleIndexText() const;

    /** Gets the visibility of the sample index (Collapsed if total <= 1). */
    EVisibility GetSampleIndexVisibility() const;

    /** Gets the visibility of the refresh button (Visible if delegate is bound). */
    EVisibility GetRefreshButtonVisibility() const;

private:
    /** Holds the sound waves provided to this widget. */
     TArray<TObjectPtr<USoundWave>> SoundWaves;
 
     /** The index of the currently active sound wave in the SoundWaves array. */
     int32 CurrentSoundWaveIndex;
 
     /** The title text to display above the waveform. */
     FText TitleText;
 
     /**
      * Called by the audio system to report the current playback percentage.
      * @param InAudioComponent The audio component playing the sound.
      * @param PlayingSoundWave The sound wave being played.
      * @param PlaybackPercent The playback progress, from 0.0 to 1.0.
      */
     void OnPlaybackPercentChanged(const class UAudioComponent* InAudioComponent, const USoundWave* PlayingSoundWave, float PlaybackPercent);
 
     /** Updates the waveform display to show the current sound wave and sets it in AudioUtils. */
     void UpdateWaveformDisplayAndSetSound();
 
     /** Handles the Play/Pause button click. */
     FReply OnPlayButtonClicked();
 
     /** Handles the Stop button click. */
     FReply OnStopButtonClicked();
 
     /** Handles the Refresh button click to cycle through sounds. */
    FReply OnSampleCycleButtonClicked();
 
     /** Handles the Save button click. */
     FReply OnSaveButtonClicked();
 
     /** Determines if the Save button should be enabled. */
     bool IsSaveButtonEnabled() const;
 
     /** Save sound wave as WAV file to disk. */
     bool SaveAsWavFile(USoundWave* SoundWave, const FString& FilePath);
 
    /** Gets the text for the play/pause button. */
   EVarcoSoundIconType GetPlayButtonIconType() const;
    FLinearColor GetPlayButtonIconTint() const;

    /** Gets the title text to display. */
     FText GetTitleText() const;
 
    /** Determines the visibility of the sample button based on the number of sounds. */
    EVisibility GetSampleButtonVisibility() const;
    bool IsSampleCycleEnabled() const;
 
     /** Visibility for loading/waveform display control */
     EVisibility GetLoadingTextVisibility() const;
     EVisibility GetWaveformVisibility() const;
 
     /** Right-aligned playback time text */
     FText GetPlaybackTimeText() const;
     EVisibility GetPlaybackTimeVisibility() const;
 
     /** Loading status text (elapsed time) */
     FText GetLoadingStatusText() const;
 
     /** Pointer to the waveform display widget. */
     TSharedPtr<SWaveformDisplay> WaveformDisplayWidget;

      /** Play/Pause button handle (update text on completion) */
      TSharedPtr<class SButton> PlayPauseButton;
     TSharedPtr<class SButton> SampleCycleButton;
 
    /** Member variables for filename generation */
    EAudioResultViewTabType TabType = EAudioResultViewTabType::Generator;
    FString PromptText;
    FString SourceFilename;
    FString TargetFilename;
 
     /** Trimming parameters */
     float TrimVadThreshold = 0.001f;
     float TrimPreRollSeconds = 0.1f;
     float TrimPostRollSeconds = 0.1f;
     float TrimFrameDurationSeconds = 0.02f;
 
     /** Loading state */
     bool bIsLoading = false;
     double LoadingStartSeconds = 0.0;
     
     /** Recent play button click time (seconds) - race diagnosis */
     double LastPlayRequestSeconds = 0.0;
 
     /** Playback state */
     float CurrentPlaybackPercent = 0.0f;
     float CurrentTotalDurationSeconds = 0.0f;
     bool bSeenProgressThisPlay = false;
	float LastPlayStartPercent = 0.0f;
	bool bHasActiveStartOffset = false;
 
     /** Instance audio player (UI-specific) */
     TUniquePtr<FAudioPlayer> AudioPlayer;

     /** Keep created sounds rooted to prevent GC from collecting */
     TArray<TWeakObjectPtr<USoundWave>> RootedWaves;
     void RootSoundWaves(const TArray<USoundWave*>& InSoundWaves);
     void UnrootSoundWaves();
 
    /** Filename generation helper function */
    FString GenerateFilename(int32 AudioIndex) const;

    /** Waveform interaction handler (waveform click/drag seeking, spacebar toggle) */
    void HandleWaveformSeek(float Percent);
    void HandleWaveformTogglePlayPause();

    /** Whether user seeking occurred while paused */
    bool bPendingSeekWhilePaused = false;

	FString CurrentSourceTag = TEXT("Unknown");
}; 