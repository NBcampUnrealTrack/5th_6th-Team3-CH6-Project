// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Input/SSlider.h"
#include "Widgets/Input/STextComboBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Sound/SoundWaveProcedural.h"
#include "AudioCaptureDeviceInterface.h"
#include "AudioCaptureCore.h"
#include "HAL/CriticalSection.h"
#include "Sound/SoundWave.h"
#include "UObject/ObjectMacros.h"
#include "AssetRegistry/AssetData.h"
#include "Tabs/IVarcoSoundTab.h"

class FApiClient;
class SAudioResultView;
class SAudioPlayerControls;
class SObjectPropertyEntryBox;
class SButton;
class SVarcoSoundIcon;
enum class EVarcoSoundIconType : uint8;

namespace Audio
{
	class FAudioCapture;
	struct FCaptureDeviceInfo;
}

enum class EMonsterVoiceInputMode : uint8
{
	File,
	Microphone
};

enum class EMicCaptureState : uint8
{
	Idle,
	Recording,
	Processing
};

/**
 * 몬스터 음성 탭 위젯
 */
class VARCOSOUND_API SMonsterVoiceTab : public SCompoundWidget, public IVarcoSoundTab
{
public:
	SLATE_BEGIN_ARGS(SMonsterVoiceTab) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	virtual ~SMonsterVoiceTab();

	// IVarcoSoundTab interface
	virtual void PauseAllPlayback() override;
	virtual void SetApiKey(const FString& InApiKey) override;

private:
	void InitInputModeOptions();
	void RefreshCaptureDevices();
	void OnInputModeChanged(TSharedPtr<FString> NewSelection, ESelectInfo::Type SelectInfo);
	EVisibility GetSourceFileControlsVisibility() const;
	EVisibility GetSourceMicControlsVisibility() const;
	bool IsRecordButtonEnabled() const;
	FReply OnRecordButtonClicked();
	FText GetRecordStatusText() const;
	void OnCaptureDeviceChanged(TSharedPtr<FString> NewSelection, ESelectInfo::Type SelectInfo);
	bool BeginMicrophoneCapture();
	void StopMicrophoneCapture(bool bDiscardBuffer);
	void HandleAudioCaptured(const void* InAudio, int32 NumFrames, int32 NumChannels, int32 SampleRate, bool bOverflow);
	void FinalizeCapturedAudio();
	int32 GetSelectedCaptureDeviceIndex() const;
	float GetCurrentRecordingDurationSeconds() const;
	void UpdateRecordingMetrics(const float* InAudio, int32 NumSamples, int32 NumChannels);
	FText BuildRecordingStatusText() const;
	EVisibility GetSaveRecordingButtonVisibility() const;
	bool IsSaveRecordingEnabled() const;
	FReply OnSaveRecordingButtonClicked();
	EVarcoSoundIconType GetRecordButtonIconType() const;
	FLinearColor GetRecordButtonTint() const;
	void UpdateLiveWaveformOnGameThread(TArray<float>&& Samples, int32 NumChannels);
	void ConfigureRecordingUI();
	void ClearRecordingUI();
    void ReleaseRecordedPlaybackWave();

private:
	// UI Event Handlers
	FReply OnConvertVoiceButtonClicked();
	
	// Asset selection for source audio
	void OnSourceAssetSelected(const FAssetData& AssetData);
	FString GetCurrentSourceAssetPath() const;
	
	// Asset selection for target audio
	void OnTargetAssetSelected(const FAssetData& AssetData);
	FString GetCurrentTargetAssetPath() const;
	
	// Parameter getters
	float GetConversionRatioValue() const;
	void OnConversionRatioValueChanged(float NewValue);
	
	// API Response Handler
	void OnMonsterVoiceApiResponse(const FString& AudioBase64);
	
	// Helper functions
	void UpdateSourceAudioPlayer();
	void UpdateTargetAudioPlayer();

private:
	// API Client
	TSharedPtr<FApiClient> ApiClient;
	
	// Audio data
	USoundWave* SourceSoundWave;
	USoundWave* TargetSoundWave;
	
	// UI Widgets
	TSharedPtr<SObjectPropertyEntryBox> SourceAssetSelectorWidget;
	TSharedPtr<SObjectPropertyEntryBox> TargetAssetSelectorWidget;
	TSharedPtr<SAudioPlayerControls> SourceAudioPlayerWidget;
	TSharedPtr<SAudioPlayerControls> TargetAudioPlayerWidget;
	TSharedPtr<SAudioResultView> OutputAudioResultView;
	TSharedPtr<SVerticalBox> OutputSectionContainer;
	TSharedPtr<SSlider> ConversionRatioSlider;
	TSharedPtr<STextBlock> ConversionRatioValueText;
	TSharedPtr<STextComboBox> InputModeCombo;
	TArray<TSharedPtr<FString>> InputModeOptions;
	TSharedPtr<SVerticalBox> SourceFileControls;
	TSharedPtr<SVerticalBox> SourceMicControls;
	TSharedPtr<SButton> RecordButton;
	TSharedPtr<STextComboBox> CaptureDeviceCombo;
	TArray<TSharedPtr<FString>> CaptureDeviceOptions;
	TSharedPtr<FString> SelectedCaptureDevice;
	TWeakPtr<SMonsterVoiceTab> WeakThisPtr;
	TSharedPtr<SButton> RecordControlButton;
	TSharedPtr<SVarcoSoundIcon> RecordIconWidget;
	
	// Parameters
	float ConversionRatioValue;
	EMonsterVoiceInputMode CurrentInputMode = EMonsterVoiceInputMode::File;
	EMicCaptureState CaptureState = EMicCaptureState::Idle;
	FString ActiveCaptureDeviceName;
	TUniquePtr<Audio::FAudioCapture> AudioCapture;
	TArray<Audio::FCaptureDeviceInfo> CaptureDeviceInfos;
	TArray<float> CaptureFloatBuffer;
	mutable FCriticalSection CaptureBufferCriticalSection;
	int32 CaptureSampleRate = 0;
	int32 CaptureNumChannels = 0;
	TObjectPtr<USoundWave> RecordedPlaybackWave;
	FString RecordedSourceBase64;
	bool bHasRecordedAudio = false;
	double CaptureStartTimeSeconds = 0.0;
	float LatestRmsLevel = 0.0f;
	float LatestPeakLevel = 0.0f;
	bool bPendingAutoStop = false;
	TArray<uint8> RecordedWaveData;
	mutable FCriticalSection RecordedWaveDataCriticalSection;
	TArray<float> LiveWaveformScratchBuffer;
	double LastWaveformVisualizationTime = 0.0;
}; 