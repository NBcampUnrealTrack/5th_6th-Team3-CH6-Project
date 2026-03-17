// Copyright Epic Games, Inc. All Rights Reserved.

#include "Tabs/SMonsterVoiceTab.h"
#include "Widgets/Text/STextBlock.h"
#include "Components/AudioComponent.h"
#include "SlateOptMacros.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SSpacer.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SSlider.h"
#include "Widgets/Input/STextComboBox.h"
#include "Utils/ApiClient.h"
#include "Utils/AudioUtils.h"
#include "Utils/VarcoSoundSettings.h"
#include "Utils/VarcoSoundPathUtils.h"
#include "Utils/ToastNotification.h"
#include "Utils/AssetSelectionHelpers.h"
#include "Sound/SoundWave.h"
#include "UI/SAudioResultView.h"
#include "UI/SAudioPlayerControls.h"
#include "UI/SVarcoSoundIconWidget.h"
#include "Styling/AppStyle.h"
#include "EditorFramework/AssetImportData.h"
#include "PropertyCustomizationHelpers.h"
#include "AudioCaptureCore.h"
#include "Misc/Base64.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "Misc/DateTime.h"
#include "HAL/PlatformFilemanager.h"
#include "HAL/PlatformTime.h"
#include "Async/Async.h"

namespace
{
	void AppendUInt32LE(TArray<uint8>& Buffer, uint32 Value)
	{
		Buffer.Add(static_cast<uint8>(Value & 0xFF));
		Buffer.Add(static_cast<uint8>((Value >> 8) & 0xFF));
		Buffer.Add(static_cast<uint8>((Value >> 16) & 0xFF));
		Buffer.Add(static_cast<uint8>((Value >> 24) & 0xFF));
	}

	void AppendUInt16LE(TArray<uint8>& Buffer, uint16 Value)
	{
		Buffer.Add(static_cast<uint8>(Value & 0xFF));
		Buffer.Add(static_cast<uint8>((Value >> 8) & 0xFF));
	}

	void AppendFourCC(TArray<uint8>& Buffer, const ANSICHAR* FourCC)
	{
		Buffer.Append(reinterpret_cast<const uint8*>(FourCC), 4);
	}

	constexpr float MaxRecordingDurationSeconds = 30.0f;
	constexpr float RecordingGainScalar = 4.0f;

	FString FormatTimeString(float Seconds)
	{
		Seconds = FMath::Max(0.0f, Seconds);
		int32 TotalSeconds = FMath::FloorToInt(Seconds + 0.5f);
		int32 Minutes = TotalSeconds / 60;
		int32 Secs = TotalSeconds % 60;
		return FString::Printf(TEXT("%02d:%02d"), Minutes, Secs);
	}
}

void SMonsterVoiceTab::InitInputModeOptions()
{
	InputModeOptions.Reset();
	InputModeOptions.Add(MakeShared<FString>(TEXT("Use Asset File")));
	InputModeOptions.Add(MakeShared<FString>(TEXT("Record Microphone")));
	CurrentInputMode = EMonsterVoiceInputMode::File;
}

void SMonsterVoiceTab::RefreshCaptureDevices()
{
	CaptureDeviceOptions.Reset();
	CaptureDeviceInfos.Reset();

	TArray<Audio::FCaptureDeviceInfo> DeviceInfos;
	Audio::FAudioCapture CaptureProbe;
	const int32 NumDevices = CaptureProbe.GetCaptureDevicesAvailable(DeviceInfos);
	if (NumDevices > 0)
	{
		for (const Audio::FCaptureDeviceInfo& Info : DeviceInfos)
		{
			CaptureDeviceOptions.Add(MakeShared<FString>(Info.DeviceName));
		}
		CaptureDeviceInfos = MoveTemp(DeviceInfos);
	}

	if (CaptureDeviceOptions.Num() == 0)
	{
		CaptureDeviceOptions.Add(MakeShared<FString>(TEXT("System Default")));
	}

	if (!SelectedCaptureDevice.IsValid() || !CaptureDeviceOptions.Contains(SelectedCaptureDevice))
	{
		SelectedCaptureDevice = CaptureDeviceOptions[0];
	}

	ActiveCaptureDeviceName = SelectedCaptureDevice.IsValid() ? *SelectedCaptureDevice : FString();

	if (CaptureDeviceCombo.IsValid())
	{
		CaptureDeviceCombo->RefreshOptions();
		CaptureDeviceCombo->SetSelectedItem(SelectedCaptureDevice);
	}
}

void SMonsterVoiceTab::OnInputModeChanged(TSharedPtr<FString> NewSelection, ESelectInfo::Type SelectInfo)
{
	if (!NewSelection.IsValid())
	{
		return;
	}

	const EMonsterVoiceInputMode PreviousMode = CurrentInputMode;

	const int32 OptionIndex = InputModeOptions.IndexOfByKey(NewSelection);
	if (OptionIndex == 1)
	{
		CurrentInputMode = EMonsterVoiceInputMode::Microphone;
	}
	else
	{
		CurrentInputMode = EMonsterVoiceInputMode::File;
	}

	if (CurrentInputMode == EMonsterVoiceInputMode::Microphone)
	{
		if (CaptureState == EMicCaptureState::Processing)
		{
			CaptureState = EMicCaptureState::Idle;
		}
		RefreshCaptureDevices();

		if (bHasRecordedAudio)
		{
			if (USoundWave* PlaybackWave = RecordedPlaybackWave.Get())
			{
				SourceSoundWave = PlaybackWave;
				UpdateSourceAudioPlayer();
			}
		}
	}

	if (SourceAudioPlayerWidget.IsValid() && CurrentInputMode == EMonsterVoiceInputMode::Microphone)
	{
		SourceAudioPlayerWidget->Stop();
	}

	if (PreviousMode == EMonsterVoiceInputMode::Microphone && CurrentInputMode == EMonsterVoiceInputMode::File)
	{
		StopMicrophoneCapture(false);
		CaptureState = EMicCaptureState::Idle;
	}

	ConfigureRecordingUI();
}

EVisibility SMonsterVoiceTab::GetSourceFileControlsVisibility() const
{
	return CurrentInputMode == EMonsterVoiceInputMode::File ? EVisibility::Visible : EVisibility::Collapsed;
}

EVisibility SMonsterVoiceTab::GetSourceMicControlsVisibility() const
{
	return CurrentInputMode == EMonsterVoiceInputMode::Microphone ? EVisibility::Visible : EVisibility::Collapsed;
}

EVarcoSoundIconType SMonsterVoiceTab::GetRecordButtonIconType() const
{
	switch (CaptureState)
	{
	case EMicCaptureState::Recording:
	case EMicCaptureState::Processing:
		return EVarcoSoundIconType::Stop;
	default:
		return EVarcoSoundIconType::Record;
	}
}

FLinearColor SMonsterVoiceTab::GetRecordButtonTint() const
{
	const FLinearColor IdleColor(0.9f, 0.1f, 0.1f, 1.0f);
	const FLinearColor RecordingColor(1.0f, 0.05f, 0.05f, 1.0f);
	switch (CaptureState)
	{
	case EMicCaptureState::Recording:
	case EMicCaptureState::Processing:
		return RecordingColor;
	default:
		return IdleColor;
	}
}

bool SMonsterVoiceTab::IsRecordButtonEnabled() const
{
	return CurrentInputMode == EMonsterVoiceInputMode::Microphone && CaptureState != EMicCaptureState::Processing;
}

FReply SMonsterVoiceTab::OnRecordButtonClicked()
{
	if (CurrentInputMode != EMonsterVoiceInputMode::Microphone)
	{
		return FReply::Handled();
	}

	if (CaptureState == EMicCaptureState::Recording)
	{
		CaptureState = EMicCaptureState::Processing;
		StopMicrophoneCapture(false);
		FinalizeCapturedAudio();
	}
	else if (CaptureState == EMicCaptureState::Idle)
	{
		if (!BeginMicrophoneCapture())
		{
			CaptureState = EMicCaptureState::Idle;
		}
	}

	if (RecordControlButton.IsValid())
	{
		RecordControlButton->Invalidate(EInvalidateWidget::Paint);
	}
	ConfigureRecordingUI();

	return FReply::Handled();
}

FText SMonsterVoiceTab::GetRecordStatusText() const
{
	return BuildRecordingStatusText();
}

void SMonsterVoiceTab::OnCaptureDeviceChanged(TSharedPtr<FString> NewSelection, ESelectInfo::Type SelectInfo)
{
	if (!NewSelection.IsValid())
	{
		return;
	}

	SelectedCaptureDevice = NewSelection;
	ActiveCaptureDeviceName = *NewSelection;
}

float SMonsterVoiceTab::GetCurrentRecordingDurationSeconds() const
{
	FScopeLock Lock(&CaptureBufferCriticalSection);
	const int32 NumSamples = CaptureFloatBuffer.Num();
	if (NumSamples <= 0)
	{
		return 0.0f;
	}

	const int32 EffectiveSampleRate = CaptureSampleRate > 0 ? CaptureSampleRate : 48000;
	const int32 EffectiveChannels = CaptureNumChannels > 0 ? CaptureNumChannels : 1;
	if (EffectiveSampleRate <= 0 || EffectiveChannels <= 0)
	{
		return 0.0f;
	}

	return static_cast<float>(NumSamples) / static_cast<float>(EffectiveSampleRate * EffectiveChannels);
}

void SMonsterVoiceTab::UpdateRecordingMetrics(const float* InAudio, int32 NumSamples, int32 NumChannels)
{
	if (!InAudio || NumSamples <= 0 || NumChannels <= 0)
	{
		return;
	}

	float SumSquares = 0.0f;
	float Peak = LatestPeakLevel;
	for (int32 Index = 0; Index < NumSamples; ++Index)
	{
		const float Sample = InAudio[Index];
		SumSquares += Sample * Sample;
		Peak = FMath::Max(Peak, FMath::Abs(Sample));
	}

	const float Rms = FMath::Sqrt(SumSquares / static_cast<float>(NumSamples));
	LatestRmsLevel = (LatestRmsLevel * 0.85f) + (Rms * 0.15f);
	LatestPeakLevel = Peak;

	if (CaptureStartTimeSeconds <= 0.0)
	{
		CaptureStartTimeSeconds = FPlatformTime::Seconds();
	}
}

FText SMonsterVoiceTab::BuildRecordingStatusText() const
{
	const FString MaxDurationString = FormatTimeString(MaxRecordingDurationSeconds);

	switch (CaptureState)
	{
	case EMicCaptureState::Recording:
	{
		const float DurationSeconds = GetCurrentRecordingDurationSeconds();
		const FString DurationString = FormatTimeString(DurationSeconds);
		const float LevelDb = LatestRmsLevel > 0.0001f ? 20.0f * FMath::Loge(LatestRmsLevel) / FMath::Loge(10.0f) : -96.0f;
		return FText::FromString(FString::Printf(TEXT("Recording %s / %s | RMS %.1f dBFS"), *DurationString, *MaxDurationString, LevelDb));
	}
	case EMicCaptureState::Processing:
		return FText::FromString(TEXT("Processing recorded audio..."));
	default:
	{
		if (bHasRecordedAudio && RecordedPlaybackWave != nullptr)
		{
			const float DurationSeconds = RecordedPlaybackWave->Duration;
			const FString DurationString = FormatTimeString(DurationSeconds);
			const float LevelDb = LatestRmsLevel > 0.0001f ? 20.0f * FMath::Loge(LatestRmsLevel) / FMath::Loge(10.0f) : -96.0f;
			return FText::FromString(FString::Printf(TEXT("Ready (last take %s | RMS %.1f dBFS)"), *DurationString, LevelDb));
		}
		return FText::FromString(TEXT("Ready"));
	}
	}
}

EVisibility SMonsterVoiceTab::GetSaveRecordingButtonVisibility() const
{
	return CurrentInputMode == EMonsterVoiceInputMode::Microphone ? EVisibility::Visible : EVisibility::Collapsed;
}

bool SMonsterVoiceTab::IsSaveRecordingEnabled() const
{
	if (!bHasRecordedAudio)
	{
		return false;
	}

	FScopeLock Lock(&RecordedWaveDataCriticalSection);
	return RecordedWaveData.Num() > 0;
}

FReply SMonsterVoiceTab::OnSaveRecordingButtonClicked()
{
	if (!bHasRecordedAudio)
	{
		VarcoSoundToast::ShowError(NSLOCTEXT("VarcoSound", "NoRecordingToSave", "No recording to save."));
		return FReply::Handled();
	}

	TArray<uint8> WaveDataCopy;
	{
		FScopeLock Lock(&RecordedWaveDataCriticalSection);
		WaveDataCopy = RecordedWaveData;
	}

	if (WaveDataCopy.Num() == 0)
	{
		VarcoSoundToast::ShowError(NSLOCTEXT("VarcoSound", "RecordingDataEmpty", "Recording data is empty."));
		return FReply::Handled();
	}

	// Per unified output policy: store recordings under OutputRoot/MonsterVoice.
	const FString SaveDirectory = VarcoSoundPathUtils::GetOutputDirForTab(EAudioResultViewTabType::MonsterVoice);
	VarcoSoundPathUtils::EnsureDirectoryExists(SaveDirectory);

	const FString Filename = FString::Printf(TEXT("recorded_%s.wav"), *FDateTime::Now().ToString(TEXT("%Y%m%d_%H%M%S")));
	const FString FullPath = FPaths::Combine(SaveDirectory, Filename);

	if (FFileHelper::SaveArrayToFile(WaveDataCopy, *FullPath))
	{
		VarcoSoundToast::ShowSuccess(FText::Format(NSLOCTEXT("VarcoSound", "RecordingSaved", "Recording saved: {0}"), FText::FromString(FullPath)));
	}
	else
	{
		VarcoSoundToast::ShowError(NSLOCTEXT("VarcoSound", "RecordingSaveFailed", "Failed to save recording."));
	}

	return FReply::Handled();
}

void SMonsterVoiceTab::UpdateLiveWaveformOnGameThread(TArray<float>&& Samples, int32 NumChannels)
{
	if (!SourceAudioPlayerWidget.IsValid())
	{
		return;
	}

	const int32 EffectiveChannels = FMath::Max(1, NumChannels);
	const int32 MaxSamplesPerChannel = 4000;
	const int32 TargetTotal = MaxSamplesPerChannel * EffectiveChannels;

	if (Samples.Num() > TargetTotal && TargetTotal > 0)
	{
		LiveWaveformScratchBuffer.SetNum(TargetTotal);
		const float Step = static_cast<float>(Samples.Num()) / static_cast<float>(TargetTotal);
		for (int32 Index = 0; Index < TargetTotal; ++Index)
		{
			const int32 SampleIndex = FMath::Clamp(static_cast<int32>(Index * Step), 0, Samples.Num() - 1);
			LiveWaveformScratchBuffer[Index] = Samples[SampleIndex];
		}
		SourceAudioPlayerWidget->SetLiveWaveform(LiveWaveformScratchBuffer, EffectiveChannels);
	}
	else
	{
		SourceAudioPlayerWidget->SetLiveWaveform(Samples, EffectiveChannels);
	}
}

void SMonsterVoiceTab::ConfigureRecordingUI()
{
	if (!SourceAudioPlayerWidget.IsValid())
	{
		return;
	}

	if (CurrentInputMode == EMonsterVoiceInputMode::Microphone)
	{
		if (!RecordControlButton.IsValid())
		{
			RecordControlButton =
				SNew(SButton)
				.ButtonStyle(&FAppStyle::Get().GetWidgetStyle<FButtonStyle>("SimpleButton"))
				.ContentPadding(FMargin(6.f))
				.Cursor(EMouseCursor::Hand)
				.OnClicked(this, &SMonsterVoiceTab::OnRecordButtonClicked)
				.IsEnabled(this, &SMonsterVoiceTab::IsRecordButtonEnabled)
				[
					SAssignNew(RecordIconWidget, SVarcoSoundIcon)
					.IconType(this, &SMonsterVoiceTab::GetRecordButtonIconType)
					.IconColor(this, &SMonsterVoiceTab::GetRecordButtonTint)
					.IconSize(FVector2D(13.f, 13.f))
				];
		}

		SourceAudioPlayerWidget->SetAuxiliaryControls(RecordControlButton);
	}
	else
	{
		ClearRecordingUI();
	}

	if (RecordControlButton.IsValid())
	{
		RecordControlButton->Invalidate(EInvalidateWidget::Paint);
	}
}

void SMonsterVoiceTab::ClearRecordingUI()
{
	if (SourceAudioPlayerWidget.IsValid())
	{
		SourceAudioPlayerWidget->ClearAuxiliaryControls();
	}
}

void SMonsterVoiceTab::ReleaseRecordedPlaybackWave()
{
	if (USoundWave* Wave = RecordedPlaybackWave.Get())
	{
		if (Wave->IsRooted())
		{
			Wave->RemoveFromRoot();
		}
	}
	RecordedPlaybackWave = nullptr;
}

bool SMonsterVoiceTab::BeginMicrophoneCapture()
{
	if (CaptureState == EMicCaptureState::Processing)
	{
		return false;
	}

	RefreshCaptureDevices();

	if (!AudioCapture.IsValid())
	{
		AudioCapture = MakeUnique<Audio::FAudioCapture>();
	}

	if (!AudioCapture.IsValid())
	{
		VarcoSoundToast::ShowError(NSLOCTEXT("VarcoSound", "AudioCaptureInitFailed", "Failed to initialize audio capture module."));
		return false;
	}

	StopMicrophoneCapture(true);

	CaptureFloatBuffer.Reset();
	CaptureSampleRate = 0;
	CaptureNumChannels = 0;
	RecordedSourceBase64.Reset();
	ReleaseRecordedPlaybackWave();
	bHasRecordedAudio = false;
	LatestRmsLevel = 0.0f;
	LatestPeakLevel = 0.0f;
	bPendingAutoStop = false;
	CaptureStartTimeSeconds = FPlatformTime::Seconds();

	{
		FScopeLock DataLock(&RecordedWaveDataCriticalSection);
		RecordedWaveData.Reset();
	}

	LastWaveformVisualizationTime = 0.0;

	if (SourceAudioPlayerWidget.IsValid())
	{
		SourceAudioPlayerWidget->SetSoundWave(nullptr);
		TArray<float> EmptyBuffer;
		SourceAudioPlayerWidget->SetLiveWaveform(EmptyBuffer, 1);
	}

	Audio::FAudioCaptureDeviceParams Params;
	Params.DeviceIndex = GetSelectedCaptureDeviceIndex();
	Params.NumInputChannels = Audio::InvalidDeviceChannelCount;
	Params.SampleRate = Audio::InvalidDeviceSampleRate;

	if (CaptureDeviceInfos.IsValidIndex(Params.DeviceIndex))
	{
		const Audio::FCaptureDeviceInfo& SelectedInfo = CaptureDeviceInfos[Params.DeviceIndex];
		if (SelectedInfo.InputChannels > 0)
		{
			Params.NumInputChannels = SelectedInfo.InputChannels;
		}
		if (SelectedInfo.PreferredSampleRate > 0)
		{
			Params.SampleRate = SelectedInfo.PreferredSampleRate;
		}
	}

	Audio::FOnAudioCaptureFunction OnCapture = [this](const void* InAudio, int32 NumFrames, int32 NumChannels, int32 SampleRate, double /*StreamTime*/, bool bOverflow)
	{
		HandleAudioCaptured(InAudio, NumFrames, NumChannels, SampleRate, bOverflow);
	};

	constexpr uint32 FramesPerBuffer = 1024;

	if (!AudioCapture->OpenAudioCaptureStream(Params, MoveTemp(OnCapture), FramesPerBuffer))
	{
		VarcoSoundToast::ShowError(NSLOCTEXT("VarcoSound", "MicStreamOpenFailed", "Failed to open microphone stream."));
		AudioCapture->AbortStream();
		return false;
	}

	if (!AudioCapture->StartStream())
	{
		VarcoSoundToast::ShowError(NSLOCTEXT("VarcoSound", "MicRecordingStartFailed", "Failed to start microphone recording."));
		AudioCapture->AbortStream();
		return false;
	}

	CaptureState = EMicCaptureState::Recording;
	VarcoSoundToast::ShowSuccess(NSLOCTEXT("VarcoSound", "RecordingStarted", "Recording started."));
	return true;
}

void SMonsterVoiceTab::StopMicrophoneCapture(bool bDiscardBuffer)
{
	if (AudioCapture.IsValid())
	{
		if (AudioCapture->IsCapturing())
		{
			AudioCapture->StopStream();
		}
		if (AudioCapture->IsStreamOpen())
		{
			AudioCapture->CloseStream();
		}
	}

	if (bDiscardBuffer)
	{
		FScopeLock Lock(&CaptureBufferCriticalSection);
		CaptureFloatBuffer.Reset();
		bHasRecordedAudio = false;
		RecordedSourceBase64.Reset();
		ReleaseRecordedPlaybackWave();
		LatestRmsLevel = 0.0f;
		LatestPeakLevel = 0.0f;
	}

	if (bDiscardBuffer)
	{
		FScopeLock DataLock(&RecordedWaveDataCriticalSection);
		RecordedWaveData.Reset();
	}

	CaptureStartTimeSeconds = 0.0;
	bPendingAutoStop = false;
	LastWaveformVisualizationTime = 0.0;

	if (bDiscardBuffer && SourceAudioPlayerWidget.IsValid())
	{
		SourceAudioPlayerWidget->SetSoundWave(nullptr);
		TArray<float> EmptyBuffer;
		SourceAudioPlayerWidget->SetLiveWaveform(EmptyBuffer, 1);
	}
}

void SMonsterVoiceTab::HandleAudioCaptured(const void* InAudio, int32 NumFrames, int32 NumChannels, int32 SampleRate, bool bOverflow)
{
	if (CaptureState != EMicCaptureState::Recording)
	{
		return;
	}

	if (!InAudio || NumFrames <= 0 || NumChannels <= 0)
	{
		return;
	}

	const int32 NumSamples = NumFrames * NumChannels;
	const float* AudioBuffer = static_cast<const float*>(InAudio);
	const int32 EffectiveChannels = NumChannels > 0 ? NumChannels : 1;

	TArray<float> GainAdjustedSamples;
	GainAdjustedSamples.SetNumUninitialized(NumSamples);
	for (int32 SampleIndex = 0; SampleIndex < NumSamples; ++SampleIndex)
	{
		const float Scaled = AudioBuffer[SampleIndex] * RecordingGainScalar;
		GainAdjustedSamples[SampleIndex] = FMath::Clamp(Scaled, -1.0f, 1.0f);
	}

	TArray<float> SamplesCopy;
	bool bDispatchWaveform = false;

	{
		FScopeLock Lock(&CaptureBufferCriticalSection);
		CaptureFloatBuffer.Append(GainAdjustedSamples.GetData(), NumSamples);
		CaptureSampleRate = SampleRate;
		CaptureNumChannels = NumChannels;

		const double Now = FPlatformTime::Seconds();
		if (Now - LastWaveformVisualizationTime >= 0.05)
		{
			LastWaveformVisualizationTime = Now;
			SamplesCopy = CaptureFloatBuffer;
			bDispatchWaveform = true;
		}
	}

	UpdateRecordingMetrics(GainAdjustedSamples.GetData(), NumSamples, NumChannels);

	const float DurationSeconds = GetCurrentRecordingDurationSeconds();
	if (!bPendingAutoStop && DurationSeconds >= MaxRecordingDurationSeconds)
	{
		bPendingAutoStop = true;
		TWeakPtr<SMonsterVoiceTab> LocalWeak = WeakThisPtr;
		AsyncTask(ENamedThreads::GameThread, [LocalWeak]()
		{
			if (TSharedPtr<SMonsterVoiceTab> Pinned = LocalWeak.Pin())
			{
				if (Pinned->CaptureState == EMicCaptureState::Recording)
				{
					Pinned->CaptureState = EMicCaptureState::Processing;
					Pinned->FinalizeCapturedAudio();
					VarcoSoundToast::ShowInfo(NSLOCTEXT("VarcoSound", "RecordingMaxLengthReached", "Maximum recording length reached. Recording stopped automatically."));
				}
			}
		});
	}

	if (bDispatchWaveform)
	{
		TWeakPtr<SMonsterVoiceTab> LocalWeak = WeakThisPtr;
		AsyncTask(ENamedThreads::GameThread, [LocalWeak, Samples = MoveTemp(SamplesCopy), EffectiveChannels]() mutable
		{
			if (TSharedPtr<SMonsterVoiceTab> Pinned = LocalWeak.Pin())
			{
				Pinned->UpdateLiveWaveformOnGameThread(MoveTemp(Samples), EffectiveChannels);
			}
		});
	}

	if (bOverflow)
	{
		UE_LOG(LogTemp, Warning, TEXT("Audio capture overflow occurred."));
	}
}

void SMonsterVoiceTab::FinalizeCapturedAudio()
{
	TArray<float> LocalBuffer;
	{
		FScopeLock Lock(&CaptureBufferCriticalSection);
		LocalBuffer = CaptureFloatBuffer;
		CaptureFloatBuffer.Reset();
	}

	StopMicrophoneCapture(false);

	if (LocalBuffer.Num() == 0)
	{
		CaptureState = EMicCaptureState::Idle;
		VarcoSoundToast::ShowError(NSLOCTEXT("VarcoSound", "NoRecordedAudio", "No recorded audio."));
		return;
	}

	const int32 SampleRate = CaptureSampleRate > 0 ? CaptureSampleRate : 48000;
	const int32 NumChannels = CaptureNumChannels > 0 ? CaptureNumChannels : 1;
	const int32 SampleCount = LocalBuffer.Num();

	float SumSquares = 0.0f;
	float Peak = 0.0f;
	for (float Sample : LocalBuffer)
	{
		SumSquares += Sample * Sample;
		Peak = FMath::Max(Peak, FMath::Abs(Sample));
	}

	TArray<int16> Int16Buffer;
	Int16Buffer.Reserve(LocalBuffer.Num());
	for (float Sample : LocalBuffer)
	{
		const float Clamped = FMath::Clamp(Sample, -1.0f, 1.0f);
		Int16Buffer.Add(static_cast<int16>(FMath::RoundToInt(Clamped * 32767.0f)));
	}

	const int32 PcmByteCount = Int16Buffer.Num() * sizeof(int16);
	TArray<uint8> PcmBytes;
	PcmBytes.SetNumUninitialized(PcmByteCount);
	FMemory::Memcpy(PcmBytes.GetData(), Int16Buffer.GetData(), PcmByteCount);

	TArray<uint8> WaveBytes;
	const uint32 DataChunkSize = static_cast<uint32>(PcmBytes.Num());
	const uint32 FileSizeMinus8 = 36u + DataChunkSize;
	const uint32 ByteRate = static_cast<uint32>(SampleRate * NumChannels * sizeof(int16));
	const uint16 BlockAlign = static_cast<uint16>(NumChannels * sizeof(int16));
	const uint16 BitsPerSample = 16;

	WaveBytes.Reserve(44 + DataChunkSize);
	AppendFourCC(WaveBytes, "RIFF");
	AppendUInt32LE(WaveBytes, FileSizeMinus8);
	AppendFourCC(WaveBytes, "WAVE");
	AppendFourCC(WaveBytes, "fmt ");
	AppendUInt32LE(WaveBytes, 16u);
	AppendUInt16LE(WaveBytes, 1u);
	AppendUInt16LE(WaveBytes, static_cast<uint16>(NumChannels));
	AppendUInt32LE(WaveBytes, static_cast<uint32>(SampleRate));
	AppendUInt32LE(WaveBytes, ByteRate);
	AppendUInt16LE(WaveBytes, BlockAlign);
	AppendUInt16LE(WaveBytes, BitsPerSample);
	AppendFourCC(WaveBytes, "data");
	AppendUInt32LE(WaveBytes, DataChunkSize);
	WaveBytes.Append(PcmBytes.GetData(), PcmBytes.Num());

	RecordedSourceBase64 = FBase64::Encode(WaveBytes);
	bHasRecordedAudio = true;
	{
		FScopeLock DataLock(&RecordedWaveDataCriticalSection);
		RecordedWaveData = WaveBytes;
	}

	ReleaseRecordedPlaybackWave();
	RecordedPlaybackWave = FAudioUtils::CreateSoundWaveFromBase64(RecordedSourceBase64);
	if (USoundWave* PlaybackWave = RecordedPlaybackWave.Get())
	{
		PlaybackWave->AddToRoot();
		PlaybackWave->SoundGroup = SOUNDGROUP_Voice;
		SourceSoundWave = PlaybackWave;
		UpdateSourceAudioPlayer();
	}

	TArray<float> WaveformCopy = LocalBuffer;
	UpdateLiveWaveformOnGameThread(MoveTemp(WaveformCopy), NumChannels);

	LatestRmsLevel = SampleCount > 0 ? FMath::Sqrt(SumSquares / static_cast<float>(SampleCount)) : 0.0f;
	LatestPeakLevel = Peak;

	CaptureState = EMicCaptureState::Idle;
	bPendingAutoStop = false;
	ConfigureRecordingUI();
	VarcoSoundToast::ShowSuccess(NSLOCTEXT("VarcoSound", "RecordingCompleted", "Recording completed."));
}

int32 SMonsterVoiceTab::GetSelectedCaptureDeviceIndex() const
{
	if (SelectedCaptureDevice.IsValid())
	{
		for (int32 Index = 0; Index < CaptureDeviceInfos.Num(); ++Index)
		{
			if (CaptureDeviceInfos[Index].DeviceName == *SelectedCaptureDevice)
			{
				return Index;
			}
		}
	}

	return Audio::DefaultDeviceIndex;
}

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION

void SMonsterVoiceTab::Construct(const FArguments& InArgs)
{
    ApiClient = MakeShareable(new FApiClient());
    SourceSoundWave = nullptr;
    TargetSoundWave = nullptr;
    ConversionRatioValue = 1.0f;  // 기본값: 1.0 (0.0~2.0 범위)
    CaptureState = EMicCaptureState::Idle;
    WeakThisPtr = SharedThis(this);

    InitInputModeOptions();
    RefreshCaptureDevices();

    ChildSlot
    [
        SNew(SScrollBox)
        + SScrollBox::Slot()
        .Padding(FMargin(10.0f))
        [
            SNew(SVerticalBox)

            /* =========================
               페이지 제목 섹션 
               ========================= */
            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(0, 10)
            [
                SNew(STextBlock)
                .Text(FText::FromString(TEXT("👹Transform your voice into a deep, powerful monster sound in one click.")))
                .Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
                .ColorAndOpacity(FLinearColor(0.7f, 0.7f, 0.7f, 1.0f))
            ]

            /* =========================
               Source Audio file selection section
               ========================= */
            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(0, 10)
            [
                SNew(SVerticalBox)
                
                // Source Audio Label과 Convert Voice 버튼을 같은 라인에 배치
                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(0, 5)
                [
                    SNew(SHorizontalBox)
                    
                    // Source Audio Label
                    + SHorizontalBox::Slot()
                    .AutoWidth()
                    .VAlign(VAlign_Center)
                    [
                        SNew(STextBlock)
                        .Text(FText::FromString(TEXT("Source Audio")))
                    ]
                    
                    // 빈 공간
                    + SHorizontalBox::Slot()
                    .FillWidth(1.0f)
                    [
                        SNew(SSpacer)
                    ]
                    
                    // Convert Voice 버튼
                    + SHorizontalBox::Slot()
                    .AutoWidth()
                    .VAlign(VAlign_Center)
                    [
                        SNew(SButton)
                        .Text(FText::FromString(TEXT("Convert Voice")))
                        .OnClicked(this, &SMonsterVoiceTab::OnConvertVoiceButtonClicked)
                    ]
                ]

                // Select input mode
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
                        .Text(FText::FromString(TEXT("Input Mode")))
                    ]
                    + SHorizontalBox::Slot()
                    .AutoWidth()
                    .Padding(8, 0)
                    .VAlign(VAlign_Center)
                    [
                        SAssignNew(InputModeCombo, STextComboBox)
                        .OptionsSource(&InputModeOptions)
                        .OnSelectionChanged(this, &SMonsterVoiceTab::OnInputModeChanged)
                        .InitiallySelectedItem(InputModeOptions.Num() > 0 ? InputModeOptions[0] : nullptr)
                    ]
                    + SHorizontalBox::Slot()
                    .FillWidth(1.0f)
                    [
                        SNew(SSpacer)
                    ]
                ]

                // File selection control
                + SVerticalBox::Slot()
                .AutoHeight()
                [
                    SAssignNew(SourceFileControls, SVerticalBox)
                    .Visibility(this, &SMonsterVoiceTab::GetSourceFileControlsVisibility)
                    + SVerticalBox::Slot()
                    .AutoHeight()
                    .Padding(0, 5)
                    [
                        SAssignNew(SourceAssetSelectorWidget, SObjectPropertyEntryBox)
                        .AllowedClass(USoundWave::StaticClass())
                        .ObjectPath(this, &SMonsterVoiceTab::GetCurrentSourceAssetPath)
                        .OnObjectChanged(this, &SMonsterVoiceTab::OnSourceAssetSelected)
                        .DisplayThumbnail(true)
                        .DisplayUseSelected(true)
                        .DisplayBrowse(true)
                    ]
                ]

                // Microphone recording control
                + SVerticalBox::Slot()
                .AutoHeight()
                [
                    SAssignNew(SourceMicControls, SVerticalBox)
                    .Visibility(this, &SMonsterVoiceTab::GetSourceMicControlsVisibility)

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
                            .Text(FText::FromString(TEXT("Capture Device")))
                        ]
                        + SHorizontalBox::Slot()
                        .FillWidth(1.0f)
                        .Padding(8, 0)
                        .VAlign(VAlign_Center)
                        [
                            SAssignNew(CaptureDeviceCombo, STextComboBox)
                            .OptionsSource(&CaptureDeviceOptions)
                            .OnSelectionChanged(this, &SMonsterVoiceTab::OnCaptureDeviceChanged)
                            .InitiallySelectedItem(SelectedCaptureDevice)
                        ]
                    ]

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
							.Text(this, &SMonsterVoiceTab::GetRecordStatusText)
							.ColorAndOpacity(FLinearColor(0.7f, 0.7f, 0.7f, 1.0f))
						]
						+ SHorizontalBox::Slot()
						.FillWidth(1.0f)
						.VAlign(VAlign_Center)
						[
							SNew(SSpacer)
						]
						+ SHorizontalBox::Slot()
						.AutoWidth()
						.VAlign(VAlign_Center)
						[
							SNew(SButton)
							.Visibility(this, &SMonsterVoiceTab::GetSaveRecordingButtonVisibility)
							.Text(FText::FromString(TEXT("Save Recording")))
							.OnClicked(this, &SMonsterVoiceTab::OnSaveRecordingButtonClicked)
							.IsEnabled(this, &SMonsterVoiceTab::IsSaveRecordingEnabled)
						]
					]
                ]
            ]
            
            /* =========================
               Source audio waveform player section
               ========================= */
            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(0, 0)
            [
                SAssignNew(SourceAudioPlayerWidget, SAudioPlayerControls)
                .WaveformHeight(100.0f)
                .ShowTitle(false)
            ]

            /* =========================
               Target Audio 파일 선택 섹션
               ========================= */
            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(0, 10)
            [
                SNew(SVerticalBox)
                
                // Target Audio Label
                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(0, 5)
                [
                    SNew(STextBlock)
                    .Text(FText::FromString(TEXT("Target Audio")))
                ]
                
                // Target Audio Selector
                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(0, 5)
                [
                    SAssignNew(TargetAssetSelectorWidget, SObjectPropertyEntryBox)
                    .AllowedClass(USoundWave::StaticClass())
                    .ObjectPath(this, &SMonsterVoiceTab::GetCurrentTargetAssetPath)
                    .OnObjectChanged(this, &SMonsterVoiceTab::OnTargetAssetSelected)
                    .DisplayThumbnail(true)
                    .DisplayUseSelected(true)
                    .DisplayBrowse(true)
                ]
            ]
            
            /* =========================
               Target 오디오 파형 플레이어 섹션
               ========================= */
            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(0, 0)
            [
                SAssignNew(TargetAudioPlayerWidget, SAudioPlayerControls)
                .WaveformHeight(100.0f)
                .ShowTitle(false)
            ]
            
            /* =========================
               Conversion Ratio 설정 섹션
               ========================= */
            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(0, 10)
            [
                SNew(SBorder)
                .BorderImage(FAppStyle::GetBrush("DetailsView.CategoryMiddle"))
                .BorderBackgroundColor(FLinearColor(0.0f, 0.0f, 0.0f, 0.1f))
                .Padding(FMargin(6, 4))
                [
                    SNew(SHorizontalBox)
                    
                    + SHorizontalBox::Slot()
                    .AutoWidth()
                    .VAlign(VAlign_Center)
                    .Padding(0, 0, 10, 0)
                    [
                        SNew(SBox)
                        .WidthOverride(100)
                        [
                            SNew(STextBlock)
                            .Text(FText::FromString(TEXT("Conversion Ratio")))
                            .Font(FAppStyle::GetFontStyle("PropertyWindow.NormalFont"))
                        ]
                    ]
                    
                    + SHorizontalBox::Slot()
                    .FillWidth(0.7f)
                    .VAlign(VAlign_Center)
                    [
                        SAssignNew(ConversionRatioSlider, SSlider)
                        .Value(this, &SMonsterVoiceTab::GetConversionRatioValue)
                        .OnValueChanged(this, &SMonsterVoiceTab::OnConversionRatioValueChanged)
                        .MinValue(0.0f)
                        .MaxValue(2.0f)
                        .StepSize(0.1f)
                    ]
                    
                    + SHorizontalBox::Slot()
                    .AutoWidth()
                    .VAlign(VAlign_Center)
                    .Padding(8, 0, 0, 0)
                    [
                        SNew(SBox)
                        .WidthOverride(40)
                        [
                            SAssignNew(ConversionRatioValueText, STextBlock)
                            .Text_Lambda([this]() { return FText::FromString(FString::Printf(TEXT("%.1f"), ConversionRatioValue)); })
                            .Justification(ETextJustify::Center)
                            .Font(FAppStyle::GetFontStyle("PropertyWindow.NormalFont"))
                        ]
                    ]
                ]
            ]
            
            /* =========================
               결과 오디오 출력 섹션 (조건부 표시)
               ========================= */
            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(0, 10)
            [
                SAssignNew(OutputSectionContainer, SVerticalBox)
                .Visibility_Lambda([this]() -> EVisibility
                {
                    if (OutputAudioResultView.IsValid())
                    {
                        return (OutputAudioResultView->IsLoading() || OutputAudioResultView->HasSoundWaves())
                            ? EVisibility::Visible
                            : EVisibility::Collapsed;
                    }
                    return EVisibility::Collapsed;
                })
                
                // 구분선
                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(0, 5)
                [
                    SNew(SBox)
                    .HeightOverride(1.0f)
                    [
                        SNew(SBorder)
                        .BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
                        .BorderBackgroundColor(FLinearColor(0.0f, 0.0f, 0.0f, 0.3f))
                    ]
                ]
                
                // 실제 결과 뷰
                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(0, 5)
                [
                    SAssignNew(OutputAudioResultView, SAudioResultView)
                ]
            ]
		]
	];

    RefreshCaptureDevices();
    if (InputModeCombo.IsValid() && InputModeOptions.Num() > 0)
    {
        InputModeCombo->SetSelectedItem(InputModeOptions[0]);
    }

    // SAudioResultView에 탭 타입 설정 (몬스터 보이스용)
    if (OutputAudioResultView.IsValid())
    {
        OutputAudioResultView->SetTabType(EAudioResultViewTabType::MonsterVoice);
    }

	ConfigureRecordingUI();
}

SMonsterVoiceTab::~SMonsterVoiceTab()
{
    StopMicrophoneCapture(true);
    ReleaseRecordedPlaybackWave();
    AudioCapture.Reset();
}

void SMonsterVoiceTab::PauseAllPlayback()
{
    if (SourceAudioPlayerWidget.IsValid())
    {
        SourceAudioPlayerWidget->Pause();
    }
    if (TargetAudioPlayerWidget.IsValid())
    {
        TargetAudioPlayerWidget->Pause();
    }
    if (OutputAudioResultView.IsValid())
    {
        OutputAudioResultView->Pause();
    }
}

void SMonsterVoiceTab::SetApiKey(const FString& InApiKey)
{
	if (ApiClient.IsValid())
	{
		ApiClient->SetApiKey(InApiKey);
	}
}

void SMonsterVoiceTab::UpdateSourceAudioPlayer()
{
    if (SourceAudioPlayerWidget.IsValid())
    {
        SourceAudioPlayerWidget->SetSoundWave(SourceSoundWave);
    }
}

void SMonsterVoiceTab::UpdateTargetAudioPlayer()
{
    if (TargetAudioPlayerWidget.IsValid())
    {
        TargetAudioPlayerWidget->SetSoundWave(TargetSoundWave);
    }
}

FReply SMonsterVoiceTab::OnConvertVoiceButtonClicked()
{
    if (!ApiClient.IsValid())
    {
        return FReply::Handled();
    }

    if (!TargetSoundWave)
    {
        VarcoSoundToast::ShowError(NSLOCTEXT("VarcoSound", "SelectTargetAudioAsset", "Please select target audio asset."));
        return FReply::Handled();
    }

    if (OutputAudioResultView.IsValid())
    {
        OutputAudioResultView->SetLoading(true);
		OutputAudioResultView->SetSourceTag(TEXT("Unknown"));
        OutputAudioResultView->SetSoundWaves(TArray<USoundWave*>());
    }

    if (CurrentInputMode == EMonsterVoiceInputMode::Microphone)
    {
        if (!bHasRecordedAudio || RecordedSourceBase64.IsEmpty())
        {
            VarcoSoundToast::ShowError(NSLOCTEXT("VarcoSound", "CompleteRecordingFirst", "Please complete recording first."));
            if (OutputAudioResultView.IsValid())
            {
                OutputAudioResultView->SetLoading(false);
            }
            return FReply::Handled();
        }

        ApiClient->SendMonsterVoiceRequestFromBase64(RecordedSourceBase64, TargetSoundWave, ConversionRatioValue, FOnMonsterVoiceApiResponse::CreateSP(this, &SMonsterVoiceTab::OnMonsterVoiceApiResponse));
    }
    else
    {
        if (!SourceSoundWave)
        {
            VarcoSoundToast::ShowError(NSLOCTEXT("VarcoSound", "SelectSourceAudioAsset", "Please select source audio asset."));
            if (OutputAudioResultView.IsValid())
            {
                OutputAudioResultView->SetLoading(false);
            }
            return FReply::Handled();
        }

        ApiClient->SendMonsterVoiceRequest(SourceSoundWave, TargetSoundWave, ConversionRatioValue, FOnMonsterVoiceApiResponse::CreateSP(this, &SMonsterVoiceTab::OnMonsterVoiceApiResponse));
    }

    return FReply::Handled();
}

void SMonsterVoiceTab::OnMonsterVoiceApiResponse(const FString& AudioBase64)
{
    	if (OutputAudioResultView.IsValid())
	{
		// 응답 도착 시 로딩 해제
		OutputAudioResultView->SetLoading(false);
		// 새 결과 재생 전 정지
		OutputAudioResultView->ResetPlaybackState();
		if (!AudioBase64.IsEmpty())
		{
			USoundWave* OutputSoundWave = FAudioUtils::CreateSoundWaveFromBase64(AudioBase64);
			if (OutputSoundWave)
			{
				TArray<USoundWave*> SoundWaves;
				SoundWaves.Add(OutputSoundWave);
				OutputAudioResultView->SetSourceTag(TEXT("Base64"));
				OutputAudioResultView->SetSoundWaves(SoundWaves);
				
				// 녹음 파일인 경우 source filename을 recorded_날짜 형식으로 설정
				if (CurrentInputMode == EMonsterVoiceInputMode::Microphone)
				{
					FString RecordedFilename = FString::Printf(TEXT("recorded_%s"), *FDateTime::Now().ToString(TEXT("%Y%m%d_%H%M%S")));
					OutputAudioResultView->SetSourceFilename(RecordedFilename);
				}
				// 일반 파일인 경우는 이미 OnSourceAssetSelected에서 설정됨
				
				// Target filename은 이미 OnTargetAssetSelected에서 설정됨
			}
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("Received empty audio data from monster voice API."));
			OutputAudioResultView->SetSoundWaves(TArray<USoundWave*>());
			VarcoSoundToast::ShowError(NSLOCTEXT("VarcoSound", "VoiceConversionFailed", "Voice conversion failed: Empty response or server error"));
		}
		
		// UI 가시성 업데이트를 위해 무효화
		if (OutputSectionContainer.IsValid())
		{
			OutputSectionContainer->Invalidate(EInvalidateWidget::Layout);
		}
	}
}

FString SMonsterVoiceTab::GetCurrentSourceAssetPath() const
{
    return FAssetSelectionHelpers::GetSoundWaveAssetPath(SourceSoundWave);
}

void SMonsterVoiceTab::OnSourceAssetSelected(const FAssetData& AssetData)
{
    USoundWave* SelectedSound = FAssetSelectionHelpers::GetSoundWaveFromAssetData(AssetData);
    if (SelectedSound)
    {
        SourceSoundWave = SelectedSound;
        UpdateSourceAudioPlayer();
        
        // Set source filename to OutputAudioResultView
        if (OutputAudioResultView.IsValid())
        {
            FString SourceFilePath = FAssetSelectionHelpers::GetSoundWaveSourceFilePath(SourceSoundWave);
            OutputAudioResultView->SetSourceFilename(SourceFilePath);
        }
    }
}

FString SMonsterVoiceTab::GetCurrentTargetAssetPath() const
{
    return FAssetSelectionHelpers::GetSoundWaveAssetPath(TargetSoundWave);
}

void SMonsterVoiceTab::OnTargetAssetSelected(const FAssetData& AssetData)
{
    USoundWave* SelectedSound = FAssetSelectionHelpers::GetSoundWaveFromAssetData(AssetData);
    if (SelectedSound)
    {
        TargetSoundWave = SelectedSound;
        UpdateTargetAudioPlayer();
        
        // Set target filename to OutputAudioResultView
        if (OutputAudioResultView.IsValid())
        {
            FString TargetFilePath = FAssetSelectionHelpers::GetSoundWaveSourceFilePath(TargetSoundWave);
            OutputAudioResultView->SetTargetFilename(TargetFilePath);
        }
    }
}

float SMonsterVoiceTab::GetConversionRatioValue() const
{
    return ConversionRatioValue;
}

void SMonsterVoiceTab::OnConversionRatioValueChanged(float NewValue)
{
    ConversionRatioValue = NewValue;
}

END_SLATE_FUNCTION_BUILD_OPTIMIZATION 
END_SLATE_FUNCTION_BUILD_OPTIMIZATION 