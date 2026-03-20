// Copyright Epic Games, Inc. All Rights Reserved.

#include "Utils/AudioUtils.h"
#include "Utils/VarcoSoundSettings.h"
#include "Utils/AssetSelectionHelpers.h"
#include "Components/AudioComponent.h"
#include "Misc/Base64.h"
#include "Editor.h"
#include "Kismet/GameplayStatics.h"
#include "HAL/FileManager.h"
#include "DesktopPlatformModule.h"
#include "IDesktopPlatform.h"
#include "GenericPlatform/GenericPlatformFile.h"
#include "Framework/Application/SlateApplication.h"
#include "AudioDevice.h"
#include "Sound/SoundWaveProcedural.h"
#include "Sound/SoundWave.h"
#include "HAL/PlatformFileManager.h"
#include "Misc/FileHelper.h"
#include "Widgets/SWindow.h"
#include "EditorFramework/AssetImportData.h"

// minimp3 for MP3 decoding
#define MINIMP3_IMPLEMENTATION
#include "ThirdParty/minimp3.h"

// Debug logging macro controlled by settings
#define VS_DEV_LOG(Verbosity, Format, ...) \
	if (VarcoSoundSettings::IsDebugLoggingEnabled()) \
	{ \
		UE_LOG(LogTemp, Verbosity, Format, ##__VA_ARGS__); \
	}

// Static member initialization
TObjectPtr<UAudioComponent> FAudioUtils::EditorAudioComponent = nullptr;
TObjectPtr<USoundBase> FAudioUtils::CurrentSound = nullptr;
FOnAudioPlaybackPercentNative::FDelegate FAudioUtils::PlaybackPercentDelegate;
bool FAudioUtils::bIsPaused = false;

void FAudioUtils::SetSound(USoundBase* NewSound, FOnAudioPlaybackPercentNative::FDelegate OnPlaybackPercent)
{
    Stop();
    CurrentSound = NewSound;
    PlaybackPercentDelegate = OnPlaybackPercent;
    bIsPaused = false;
    
    if (CurrentSound)
    {
        VS_DEV_LOG(Log, TEXT("Sound set: %s"), *CurrentSound->GetName());
    }
    else
    {
        VS_DEV_LOG(Log, TEXT("Sound cleared"));
    }
}

void FAudioUtils::Play(float StartTime)
{
    if (!CurrentSound)
    {
        UE_LOG(LogTemp, Warning, TEXT("Play failed: No sound is set."));
        return;
    }

    // When playback request comes in, if not paused, reset the internal state of the Procedural Sound Wave and re-queue the data.
    if (USoundWaveProcedural* ProceduralSoundWave = Cast<USoundWaveProcedural>(CurrentSound))
    {
        if (!bIsPaused)
        {
            ProceduralSoundWave->ResetAudio();
            if (ProceduralSoundWave->RawPCMDataSize > 0)
            {
                ProceduralSoundWave->QueueAudio(ProceduralSoundWave->RawPCMData, ProceduralSoundWave->RawPCMDataSize);
                VS_DEV_LOG(Log, TEXT("Procedural sound wave has been reset and re-queued for: %s"), *ProceduralSoundWave->GetName());
            }
        }
    }

    // Resume from paused state
    if (bIsPaused && EditorAudioComponent != nullptr && IsValid(EditorAudioComponent) && 
        EditorAudioComponent->IsValidLowLevel() && !EditorAudioComponent->HasAnyFlags(RF_BeginDestroyed))
    {
        EditorAudioComponent->SetPaused(false);
        bIsPaused = false;
        VS_DEV_LOG(Log, TEXT("Resuming audio: %s"), CurrentSound ? *CurrentSound->GetName() : TEXT("Unknown"));
        return;
    }

    // Stop and initialize the existing component safely when stopping or playing for the first time.
    Stop();
    InitializeAudioComponent();

    if (EditorAudioComponent != nullptr && IsValid(EditorAudioComponent) && EditorAudioComponent->IsValidLowLevelFast() && !EditorAudioComponent->HasAnyFlags(RF_BeginDestroyed))
    {
        EditorAudioComponent->Play(StartTime);
        bIsPaused = false;
        VS_DEV_LOG(Log, TEXT("Playing audio from start: %s"), CurrentSound ? *CurrentSound->GetName() : TEXT("Unknown"));
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to initialize audio component"));
    }
}

void FAudioUtils::Pause()
{
    if (!EditorAudioComponent || !IsValid(EditorAudioComponent))
    {
        return;
    }
    
    if (!EditorAudioComponent->IsValidLowLevel() || EditorAudioComponent->HasAnyFlags(RF_BeginDestroyed))
    {
        EditorAudioComponent = nullptr;
        return;
    }
    
    if (IsPlaying())
    {
        EditorAudioComponent->SetPaused(true);
        bIsPaused = true;
        UE_LOG(LogTemp, Log, TEXT("Pausing audio: %s"), CurrentSound ? *CurrentSound->GetName() : TEXT("Unknown"));
    }
}

void FAudioUtils::Stop()
{
    if (!EditorAudioComponent)
    {
        bIsPaused = false;
        return;
    }
    
    if (IsValid(EditorAudioComponent) && 
        EditorAudioComponent->IsValidLowLevelFast() && 
        !EditorAudioComponent->HasAnyFlags(RF_BeginDestroyed))
    {
        EditorAudioComponent->Stop();
        UE_LOG(LogTemp, Log, TEXT("Stopping audio: %s"), CurrentSound ? *CurrentSound->GetName() : TEXT("Unknown"));
    }
    
    EditorAudioComponent = nullptr;
    bIsPaused = false;
}

bool FAudioUtils::IsPlaying()
{
    // Enhanced safety checks
    if (!EditorAudioComponent || !IsValid(EditorAudioComponent))
    {
        return false;
    }
    
    // Check if the component is scheduled for destruction
    if (!EditorAudioComponent->IsValidLowLevelFast() || EditorAudioComponent->HasAnyFlags(RF_BeginDestroyed))
    {
        return false;
    }
    
    // If paused, return false
    if (bIsPaused)
    {
        return false;
    }
    
    return EditorAudioComponent->IsPlaying();
}

bool FAudioUtils::IsPaused()
{
    return bIsPaused;
}

UAudioComponent* FAudioUtils::GetEditorAudioComponent()
{
    // Enhanced safety checks
    if (!EditorAudioComponent || !IsValid(EditorAudioComponent))
    {
        return nullptr;
    }
    
    // Check if the component is scheduled for destruction
    if (!EditorAudioComponent->IsValidLowLevelFast() || EditorAudioComponent->HasAnyFlags(RF_BeginDestroyed))
    {
        EditorAudioComponent = nullptr;
        return nullptr;
    }
    
    return EditorAudioComponent;
}

// --- Private Helper Functions ---

void FAudioUtils::InitializeAudioComponent()
{
    if (!GEditor || !CurrentSound)
    {
        return;
    }
    
    // If there was a component previously, safely release it
    Stop();

    UWorld* EditorWorld = GEditor->GetEditorWorldContext().World();
    if (!EditorWorld)
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to get editor world."));
        return;
    }

    EditorAudioComponent = UGameplayStatics::CreateSound2D(EditorWorld, CurrentSound, 1.0f, 1.0f, 0.0f, nullptr, true);
    if (EditorAudioComponent != nullptr && IsValid(EditorAudioComponent))
    {
        EditorAudioComponent->OnAudioPlaybackPercentNative.Add(PlaybackPercentDelegate);
        UE_LOG(LogTemp, Log, TEXT("Audio component initialized."));
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to create audio component."));
        EditorAudioComponent = nullptr; // Set to nullptr definitely on failure
    }
}

void FAudioUtils::TeardownAudioComponent()
{
    if (EditorAudioComponent)
    {
        // For live coding safety, cache the pointer before IsValid check and set it immediately to nullptr
        UAudioComponent* TempComponent = EditorAudioComponent;
        EditorAudioComponent = nullptr;
        
        // Strict validity check for TempComponent
        const bool bTempValid = (TempComponent != nullptr) 
            && IsValid(TempComponent) 
            && TempComponent->IsValidLowLevelFast() 
            && !TempComponent->HasAnyFlags(RF_BeginDestroyed);
            
        if (bTempValid)
        {
            // Call safely after validation
            if (TempComponent->IsPlaying())
            {
                TempComponent->Stop();
            }
            TempComponent->OnAudioPlaybackPercentNative.Clear();
            
            UE_LOG(LogTemp, Log, TEXT("Audio component torn down successfully."));
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("Audio component was invalid during teardown."));
        }
	}
}


// --- Utility Functions ---

USoundWave* FAudioUtils::CreateSoundWaveFromBase64(const FString& Base64String)
{
    // Decode Base64 string to binary data
    TArray<uint8> BinaryData;
    if (!FBase64::Decode(Base64String, BinaryData))
    {
        UE_LOG(LogTemp, Error, TEXT("Base64 decoding failed"));
        return nullptr;
    }
    
    UE_LOG(LogTemp, Log, TEXT("Base64 decoding success - data size: %d bytes"), BinaryData.Num());
	VS_DEV_LOG(Log, TEXT("[AudioUtils::CreateSoundWaveFromBase64] source=Base64 bytes=%d"), BinaryData.Num());
    
    // Call common WAV parser
    return ParseWavAndCreateSoundWave(BinaryData, TEXT("FromBase64"));
}

TArray<float> FAudioUtils::ExtractWaveformData(const USoundWave* SoundWave)
{
    TArray<float> WaveformData;
    if (!SoundWave || !SoundWave->RawPCMData || SoundWave->RawPCMDataSize == 0)
    {
        return WaveformData;
    }

    const int32 NumChannels = SoundWave->NumChannels;
    const int32 DataSize = SoundWave->RawPCMDataSize;
    const uint8* AudioData = SoundWave->RawPCMData;
    const int32 BitsPerSample = 16; // Known limitation: Currently supports 16-bit audio only. 24-bit and 32-bit formats are not yet supported.
    const int32 MaxSamples = 1000; // Limit the number of samples for performance

    const int32 BytesPerSample = BitsPerSample / 8;
    const int32 BytesPerFrame = BytesPerSample * NumChannels;
    const int32 NumFrames = DataSize / BytesPerFrame;
    const int32 SampleStep = FMath::Max(1, NumFrames / MaxSamples);

    WaveformData.Reserve(NumFrames / SampleStep);

    for (int32 FrameIndex = 0; FrameIndex < NumFrames; FrameIndex += SampleStep)
    {
        float SampleValue = 0.0f;
        for (int32 ChannelIndex = 0; ChannelIndex < NumChannels; ++ChannelIndex)
        {
            const int32 SampleOffset = FrameIndex * BytesPerFrame + ChannelIndex * BytesPerSample;
            if (SampleOffset + BytesPerSample > DataSize)
            {
                break;
            }
            const int16 Sample16 = static_cast<int16>(AudioData[SampleOffset] | (AudioData[SampleOffset + 1] << 8));
            SampleValue += static_cast<float>(Sample16) / 32768.0f;
        }
        SampleValue /= static_cast<float>(NumChannels);
        WaveformData.Add(FMath::Clamp(SampleValue, -1.0f, 1.0f));
    }

    return WaveformData;
}

TArray<float> FAudioUtils::ExtractStereoWaveformData(const USoundWave* SoundWave)
{
    TArray<float> WaveformData;
    if (!SoundWave || !SoundWave->RawPCMData || SoundWave->RawPCMDataSize == 0)
    {
        return WaveformData;
    }

    const int32 NumChannels = SoundWave->NumChannels;
    const int32 DataSize = SoundWave->RawPCMDataSize;
    const uint8* AudioData = SoundWave->RawPCMData;
    const int32 BitsPerSample = 16; // Known limitation: Currently supports 16-bit audio only. 24-bit and 32-bit formats are not yet supported.
    const int32 MaxSamples = 1000; // Limit the number of samples for performance

    const int32 BytesPerSample = BitsPerSample / 8;
    const int32 BytesPerFrame = BytesPerSample * NumChannels;
    const int32 NumFrames = DataSize / BytesPerFrame;
    const int32 SampleStep = FMath::Max(1, NumFrames / MaxSamples);

    // For stereo: return interleaved L/R data, for mono: return single channel data
    if (NumChannels >= 2)
    {
        // Stereo or multi-channel (only use L/R)
        WaveformData.Reserve((NumFrames / SampleStep) * 2);

        for (int32 FrameIndex = 0; FrameIndex < NumFrames; FrameIndex += SampleStep)
        {
            // Left channel (channel 0)
            const int32 LeftSampleOffset = FrameIndex * BytesPerFrame + 0 * BytesPerSample;
            if (LeftSampleOffset + BytesPerSample <= DataSize)
            {
                const int16 LeftSample16 = static_cast<int16>(AudioData[LeftSampleOffset] | (AudioData[LeftSampleOffset + 1] << 8));
                const float LeftSampleValue = static_cast<float>(LeftSample16) / 32768.0f;
                WaveformData.Add(FMath::Clamp(LeftSampleValue, -1.0f, 1.0f));
            }

            // Right channel (channel 1)
            const int32 RightSampleOffset = FrameIndex * BytesPerFrame + 1 * BytesPerSample;
            if (RightSampleOffset + BytesPerSample <= DataSize)
            {
                const int16 RightSample16 = static_cast<int16>(AudioData[RightSampleOffset] | (AudioData[RightSampleOffset + 1] << 8));
                const float RightSampleValue = static_cast<float>(RightSample16) / 32768.0f;
                WaveformData.Add(FMath::Clamp(RightSampleValue, -1.0f, 1.0f));
            }
        }
    }
    else
    {
        // For mono channel: return single channel data in the same way as the existing logic
        WaveformData.Reserve(NumFrames / SampleStep);

        for (int32 FrameIndex = 0; FrameIndex < NumFrames; FrameIndex += SampleStep)
        {
            const int32 SampleOffset = FrameIndex * BytesPerFrame;
            if (SampleOffset + BytesPerSample <= DataSize)
            {
                const int16 Sample16 = static_cast<int16>(AudioData[SampleOffset] | (AudioData[SampleOffset + 1] << 8));
                const float SampleValue = static_cast<float>(Sample16) / 32768.0f;
                WaveformData.Add(FMath::Clamp(SampleValue, -1.0f, 1.0f));
            }
        }
    }

    return WaveformData;
}

USoundWave* FAudioUtils::ParseWavAndCreateSoundWave(const TArray<uint8>& WavData, const FString& Name)
{
    // Validate WAV file header
    if (WavData.Num() < 44)  // WAV header is at least 44 bytes
    {
        UE_LOG(LogTemp, Error, TEXT("Audio data is too short"));
        return nullptr;
    }

    // Check WAV header "RIFF"
    if (WavData[0] != 'R' || WavData[1] != 'I' || WavData[2] != 'F' || WavData[3] != 'F')
    {
        UE_LOG(LogTemp, Error, TEXT("Invalid WAV header (RIFF signature missing)"));
        return nullptr;
    }

    // Check WAV header "WAVE"
    if (WavData[8] != 'W' || WavData[9] != 'A' || WavData[10] != 'V' || WavData[11] != 'E')
    {
        UE_LOG(LogTemp, Error, TEXT("Invalid WAV header (WAVE signature missing)"));
        return nullptr;
    }

    // Extract information from WAV file header
    uint32 SampleRate = 0;
    uint16 NumChannels = 0;
    uint16 BitsPerSample = 0;
    uint16 FormatTag = 0;

    // Format tag (bytes 20-21)
    FormatTag = WavData[20] | (WavData[21] << 8);
    
    // Number of channels (bytes 22-23)
    NumChannels = WavData[22] | (WavData[23] << 8);

    // Sample rate (bytes 24-27)
    SampleRate = WavData[24] | (WavData[25] << 8) | (WavData[26] << 16) | (WavData[27] << 24);

    // Bit depth (bytes 34-35)
    BitsPerSample = WavData[34] | (WavData[35] << 8);

    // Check format type (1 = PCM)
    if (FormatTag != 1)
    {
        UE_LOG(LogTemp, Warning, TEXT("Non-PCM audio format: %d, processing may be incomplete"), FormatTag);
    }

    // Find data chunk
    int32 DataOffset = 44;  // Default WAV header size
    bool bFoundDataChunk = false;

    // Search for data chunk (more robust method)
    for (int32 i = 12; i < WavData.Num() - 8; i++)
    {
        if (WavData[i] == 'd' && WavData[i+1] == 'a' && WavData[i+2] == 't' && WavData[i+3] == 'a')
        {
            DataOffset = i + 8;  // "data" + 4 bytes size after
            bFoundDataChunk = true;
            UE_LOG(LogTemp, Log, TEXT("Data chunk found - offset: %d"), DataOffset);
            break;
        }
    }

    if (!bFoundDataChunk)
    {
        UE_LOG(LogTemp, Warning, TEXT("Data chunk not found, using default offset"));
    }

    // Data size (data chunk + 4 bytes)
    uint32 DataSize = 0;
    if (bFoundDataChunk && DataOffset >= 8)
    {
        DataSize = WavData[DataOffset-4] | (WavData[DataOffset-3] << 8) | 
                  (WavData[DataOffset-2] << 16) | (WavData[DataOffset-1] << 24);
    }
    else
    {
        // Estimate size using default method
        DataSize = WavData[40] | (WavData[41] << 8) | (WavData[42] << 16) | (WavData[43] << 24);
    }

    // Validate data size
    if (DataSize == 0 || DataOffset + DataSize > (uint32)WavData.Num())
    {
        UE_LOG(LogTemp, Warning, TEXT("Data size is invalid, using remaining entire data"));
        DataSize = WavData.Num() - DataOffset;
    }

    VS_DEV_LOG(Log, TEXT("WAV file header analysis - format: %d, sample rate: %d, channels: %d, bit depth: %d, data size: %d"),
        (int32)FormatTag, (int32)SampleRate, (int32)NumChannels, (int32)BitsPerSample, (int32)DataSize);

    // Extract audio data (data after header)
    TArray<uint8> AudioData;
    AudioData.Append(WavData.GetData() + DataOffset, FMath::Min((int32)DataSize, WavData.Num() - DataOffset));

    VS_DEV_LOG(Log, TEXT("Audio data extraction completed - size: %d bytes"), AudioData.Num());

    // Validate sample rate and number of channels
    if (SampleRate == 0 || SampleRate > 192000)
    {
        UE_LOG(LogTemp, Warning, TEXT("Invalid sample rate: %d, defaulting to 44100"), SampleRate);
        SampleRate = 44100;
    }

    if (NumChannels == 0 || NumChannels > 8)
    {
        UE_LOG(LogTemp, Warning, TEXT("Invalid number of channels: %d, defaulting to 2"), NumChannels);
        NumChannels = 2;
    }

    if (BitsPerSample != 8 && BitsPerSample != 16 && BitsPerSample != 24 && BitsPerSample != 32)
    {
        UE_LOG(LogTemp, Warning, TEXT("Unsupported bit depth: %d, defaulting to 16"), BitsPerSample);
        BitsPerSample = 16;
    }

    // Create USoundWaveProcedural (for playback and PCM access)
    USoundWaveProcedural* SoundWave = NewObject<USoundWaveProcedural>(USoundWaveProcedural::StaticClass());
    if (!SoundWave)
    {
        UE_LOG(LogTemp, Error, TEXT("SoundWaveProcedural object creation failed"));
        return nullptr;
    }

    // Set name
    SoundWave->Rename(*FString::Printf(TEXT("%s_%d"), *Name, FMath::Rand()));
    UE_LOG(LogTemp, Log, TEXT("SoundWave name: %s"), *SoundWave->GetName());

    // Set default properties
    SoundWave->SetSampleRate(SampleRate);
    SoundWave->NumChannels = NumChannels;
    SoundWave->bLooping = false;
    SoundWave->bCanProcessAsync = true;
    SoundWave->VirtualizationMode = EVirtualizationMode::PlayWhenSilent;
    
    // Convert PCM data to 16-bit for procedural playback
    TArray<uint8> PcmData16;
    const uint8* PcmSource = AudioData.GetData();
    int32 PcmDataSize = AudioData.Num();

    if (BitsPerSample == 8)
    {
        PcmData16.SetNumUninitialized(AudioData.Num() * 2);
        for (int32 i = 0; i < AudioData.Num(); ++i)
        {
            int16 Sample16 = ((int16)AudioData[i] - 128) * 256;
            PcmData16[i * 2] = Sample16 & 0xFF;
            PcmData16[i * 2 + 1] = (Sample16 >> 8) & 0xFF;
        }
        PcmSource = PcmData16.GetData();
        PcmDataSize = PcmData16.Num();
        BitsPerSample = 16;
    }
    else if (BitsPerSample == 24)
    {
        const int32 SampleCount = AudioData.Num() / 3;
        PcmData16.SetNumUninitialized(SampleCount * 2);
        for (int32 i = 0; i < SampleCount; ++i)
        {
            const int32 ByteIndex = i * 3;
            int32 Sample24 = (int32)(AudioData[ByteIndex] | (AudioData[ByteIndex + 1] << 8) | (AudioData[ByteIndex + 2] << 16));
            if (Sample24 & 0x00800000)
            {
                Sample24 |= ~0x00FFFFFF;
            }
            const int16 Sample16 = static_cast<int16>(Sample24 >> 8);
            PcmData16[i * 2] = Sample16 & 0xFF;
            PcmData16[i * 2 + 1] = (Sample16 >> 8) & 0xFF;
        }
        PcmSource = PcmData16.GetData();
        PcmDataSize = PcmData16.Num();
        BitsPerSample = 16;
        UE_LOG(LogTemp, Warning, TEXT("Converted 24-bit PCM to 16-bit for procedural playback"));
    }
    else if (BitsPerSample == 32)
    {
        const int32 SampleCount = AudioData.Num() / 4;
        PcmData16.SetNumUninitialized(SampleCount * 2);
        if (FormatTag == 3)
        {
            for (int32 i = 0; i < SampleCount; ++i)
            {
                float SampleFloat = 0.0f;
                FMemory::Memcpy(&SampleFloat, AudioData.GetData() + (i * 4), sizeof(float));
                SampleFloat = FMath::Clamp(SampleFloat, -1.0f, 1.0f);
                const int16 Sample16 = static_cast<int16>(SampleFloat * 32767.0f);
                PcmData16[i * 2] = Sample16 & 0xFF;
                PcmData16[i * 2 + 1] = (Sample16 >> 8) & 0xFF;
            }
            UE_LOG(LogTemp, Warning, TEXT("Converted 32-bit float PCM to 16-bit for procedural playback"));
        }
        else
        {
            for (int32 i = 0; i < SampleCount; ++i)
            {
                int32 Sample32 = 0;
                FMemory::Memcpy(&Sample32, AudioData.GetData() + (i * 4), sizeof(int32));
                const int16 Sample16 = static_cast<int16>(Sample32 >> 16);
                PcmData16[i * 2] = Sample16 & 0xFF;
                PcmData16[i * 2 + 1] = (Sample16 >> 8) & 0xFF;
            }
            UE_LOG(LogTemp, Warning, TEXT("Converted 32-bit PCM to 16-bit for procedural playback"));
        }
        PcmSource = PcmData16.GetData();
        PcmDataSize = PcmData16.Num();
        BitsPerSample = 16;
    }

    // Set RawPCMData (for visualization)
    SoundWave->RawPCMDataSize = PcmDataSize;
    SoundWave->RawPCMData = static_cast<uint8*>(FMemory::Malloc(PcmDataSize));
    FMemory::Memcpy(SoundWave->RawPCMData, PcmSource, PcmDataSize);

    // Add audio data to Procedural queue
    SoundWave->QueueAudio(PcmSource, PcmDataSize);

    // 재생 시간 계산
    float BytesPerSec = SampleRate * NumChannels * (BitsPerSample / 8);
    float DurationInSeconds = BytesPerSec > 0 ? ((float)PcmDataSize / BytesPerSec) : 0.0f;
    SoundWave->Duration = DurationInSeconds;

    UE_LOG(LogTemp, Log, TEXT("Audio length calculation: %.2f seconds"), DurationInSeconds);

    // Set other properties
    SoundWave->SoundGroup = ESoundGroup::SOUNDGROUP_Default;

    UE_LOG(LogTemp, Log, TEXT("SoundWaveProcedural created - RawPCMDataSize: %d, QueuedBytes: %d"), SoundWave->RawPCMDataSize, PcmDataSize);

    UE_LOG(LogTemp, Log, TEXT("Audio data loaded successfully: %s (sample rate: %d, channels: %d, bit depth: %d, length: %.2f seconds)"),
        *Name, (int32)SampleRate, (int32)NumChannels, (int32)BitsPerSample, SoundWave->Duration);

    return SoundWave;
}

USoundWave* FAudioUtils::CreateSoundWaveFromFile(const FString& FilePath)
{
    // Check if file exists
    if (!FPlatformFileManager::Get().GetPlatformFile().FileExists(*FilePath))
    {
        UE_LOG(LogTemp, Error, TEXT("File not found: %s"), *FilePath);
        return nullptr;
    }

    // Check file extension
    FString Extension = FPaths::GetExtension(FilePath).ToLower();
    if (Extension != TEXT("wav") && Extension != TEXT("mp3"))
    {
        UE_LOG(LogTemp, Error, TEXT("Unsupported audio file format: %s (only wav or mp3 supported)"), *Extension);
        return nullptr;
    }

    // Read file
    TArray<uint8> RawFileData;
    if (!FFileHelper::LoadFileToArray(RawFileData, *FilePath))
    {
        UE_LOG(LogTemp, Error, TEXT("File read failed: %s"), *FilePath);
        return nullptr;
    }

	VS_DEV_LOG(Log, TEXT("[AudioUtils::CreateSoundWaveFromFile] source=SaveWav path=%s bytes=%d"), *FilePath, RawFileData.Num());

    // Handle MP3 file processing
    if (Extension == TEXT("mp3"))
    {
        return CreateSoundWaveFromMp3File(FilePath, RawFileData);
    }

    VS_DEV_LOG(Log, TEXT("WAV file loaded successfully: %s - size: %d bytes"), *FilePath, RawFileData.Num());

    // Call common WAV parser
    return ParseWavAndCreateSoundWave(RawFileData, FPaths::GetBaseFilename(FilePath));
}

TArray<uint8> FAudioUtils::DecodeMp3ToPCM(const TArray<uint8>& Mp3Data, uint32& OutSampleRate, uint16& OutNumChannels)
{
    TArray<uint8> PCMData;
    
    if (Mp3Data.Num() == 0)
    {
        UE_LOG(LogTemp, Error, TEXT("DecodeMp3ToPCM: Empty MP3 data"));
        return PCMData;
    }
    
    mp3dec_t mp3d;
    mp3dec_init(&mp3d);
    
    mp3dec_frame_info_t info;
    int samples;
    mp3d_sample_t pcm[MINIMP3_MAX_SAMPLES_PER_FRAME];
    
    TArray<int16> AllSamples;
    const uint8* data = Mp3Data.GetData();
    int size = Mp3Data.Num();
    int offset = 0;
    
    OutSampleRate = 44100; // Default value
    OutNumChannels = 2;    // Default value
    
    // Decode by frame
    while (offset < size)
    {
        samples = mp3dec_decode_frame(&mp3d, data + offset, size - offset, pcm, &info);
        if (samples > 0)
        {
            for (int i = 0; i < samples * info.channels; i++)
            {
                AllSamples.Add(pcm[i]);
            }
            OutSampleRate = info.hz;
            OutNumChannels = info.channels;
        }
        offset += info.frame_bytes;
        if (info.frame_bytes == 0) break;
    }
    
    if (AllSamples.Num() == 0)
    {
        UE_LOG(LogTemp, Error, TEXT("DecodeMp3ToPCM: No samples decoded"));
        return PCMData;
    }
    
    // Convert int16 array to uint8 array
    PCMData.SetNumUninitialized(AllSamples.Num() * 2);
    FMemory::Memcpy(PCMData.GetData(), AllSamples.GetData(), PCMData.Num());
    
    UE_LOG(LogTemp, Log, TEXT("DecodeMp3ToPCM: Decoded %d samples (%d bytes), %d Hz, %d channels"), 
        AllSamples.Num(), PCMData.Num(), OutSampleRate, OutNumChannels);
    
    return PCMData;
}

USoundWave* FAudioUtils::CreateSoundWaveFromMp3File(const FString& FilePath, const TArray<uint8>& RawFileData)
{
    VS_DEV_LOG(Log, TEXT("MP3 file load started: %s - size: %d bytes"), *FilePath, RawFileData.Num());

    // Decode MP3 to PCM
    uint32 SampleRate = 44100;
    uint16 NumChannels = 2;
    TArray<uint8> PCMData = DecodeMp3ToPCM(RawFileData, SampleRate, NumChannels);
    
    if (PCMData.Num() == 0)
    {
        UE_LOG(LogTemp, Error, TEXT("MP3 decoding failed: %s"), *FilePath);
        return nullptr;
    }
    
    UE_LOG(LogTemp, Log, TEXT("MP3 decoding successful: %d bytes PCM, %d Hz, %d channels"), 
        PCMData.Num(), SampleRate, NumChannels);

    // Create USoundWaveProcedural (same as WAV)
    USoundWaveProcedural* SoundWave = NewObject<USoundWaveProcedural>(USoundWaveProcedural::StaticClass());
    if (!SoundWave)
    {
        UE_LOG(LogTemp, Error, TEXT("SoundWaveProcedural creation failed"));
        return nullptr;
    }

    // Set file name
    FString FileName = FPaths::GetBaseFilename(FilePath);
    SoundWave->Rename(*FString::Printf(TEXT("%s_%d"), *FileName, FMath::Rand()));

    // Set default properties
    SoundWave->SetSampleRate(SampleRate);
    SoundWave->NumChannels = NumChannels;
    SoundWave->Duration = (float)PCMData.Num() / (SampleRate * NumChannels * 2.0f); // 16-bit
    SoundWave->bLooping = false;
    SoundWave->bCanProcessAsync = true;
    SoundWave->VirtualizationMode = EVirtualizationMode::PlayWhenSilent;
    SoundWave->SoundGroup = ESoundGroup::SOUNDGROUP_Default;

    // Set RawPCMData (for waveform display) - same as WAV!
    SoundWave->RawPCMDataSize = PCMData.Num();
    SoundWave->RawPCMData = static_cast<uint8*>(FMemory::Malloc(PCMData.Num()));
    FMemory::Memcpy(SoundWave->RawPCMData, PCMData.GetData(), PCMData.Num());

    // QueueAudio (for playback)
    SoundWave->QueueAudio(PCMData.GetData(), PCMData.Num());

    UE_LOG(LogTemp, Log, TEXT("MP3 SoundWave created: %s (Duration: %.2fs, SampleRate: %d, Channels: %d)"), 
        *FileName, SoundWave->Duration, SampleRate, NumChannels);

    return SoundWave;
}

TArray<float> FAudioUtils::ExtractWaveformFromContentBrowserAsset(USoundWave* SoundWave)
{
    TArray<float> WaveformData;
    if (!SoundWave)
    {
        UE_LOG(LogTemp, Error, TEXT("ExtractWaveformFromContentBrowserAsset: SoundWave is NULL"));
        return WaveformData;
    }

    UE_LOG(LogTemp, Warning, TEXT("ExtractWaveformFromContentBrowserAsset: Processing %s"), *SoundWave->GetName());

    // Check if RawPCMData exists (created from Base64)
    if (SoundWave->RawPCMData && SoundWave->RawPCMDataSize > 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("ExtractWaveformFromContentBrowserAsset: Using existing RawPCMData"));
        return ExtractWaveformData(SoundWave);
    }

    // **New logic: direct PCM decompression from uasset**
    UE_LOG(LogTemp, Warning, TEXT("ExtractWaveformFromContentBrowserAsset: Attempting direct decompression"));
    
    TArray<uint8> DecodedPCM = DecompressSoundWaveToPCM(SoundWave);
    if (DecodedPCM.Num() > 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("ExtractWaveformFromContentBrowserAsset: Direct decompression successful, %d bytes"), DecodedPCM.Num());
        
        // Set RawPCMData temporarily to reuse existing logic
        USoundWaveProcedural* TempSoundWave = NewObject<USoundWaveProcedural>();
        TempSoundWave->SetSampleRate(SoundWave->GetSampleRateForCurrentPlatform());
        TempSoundWave->NumChannels = SoundWave->NumChannels;
        TempSoundWave->RawPCMDataSize = DecodedPCM.Num();
        TempSoundWave->RawPCMData = static_cast<uint8*>(FMemory::Malloc(DecodedPCM.Num()));
        FMemory::Memcpy(TempSoundWave->RawPCMData, DecodedPCM.GetData(), DecodedPCM.Num());
        
        WaveformData = ExtractWaveformData(TempSoundWave);
        
        UE_LOG(LogTemp, Warning, TEXT("ExtractWaveformFromContentBrowserAsset: SUCCESS via direct decompression! Extracted %d waveform samples"), WaveformData.Num());
        return WaveformData;
    }

    // Fallback: original file path based logic
    if (SoundWave->AssetImportData)
    {
        UE_LOG(LogTemp, Warning, TEXT("ExtractWaveformFromContentBrowserAsset: Has AssetImportData"));
        
        const TArray<FAssetImportInfo::FSourceFile>& SourceFiles = SoundWave->AssetImportData->SourceData.SourceFiles;
        UE_LOG(LogTemp, Warning, TEXT("ExtractWaveformFromContentBrowserAsset: SourceFiles count: %d"), SourceFiles.Num());
        
        if (SourceFiles.Num() > 0)
        {
            FString SourceFilePath = SourceFiles[0].RelativeFilename;
            UE_LOG(LogTemp, Warning, TEXT("ExtractWaveformFromContentBrowserAsset: Original path: %s"), *SourceFilePath);
            
            if (!SourceFilePath.IsEmpty())
            {
                // Try converting to absolute path first
                FString AbsolutePath = FPaths::ConvertRelativePathToFull(SourceFilePath);
                UE_LOG(LogTemp, Warning, TEXT("ExtractWaveformFromContentBrowserAsset: Absolute path attempt: %s"), *AbsolutePath);
                
                // If file exists, use directly
                if (FPaths::FileExists(AbsolutePath))
                {
                    UE_LOG(LogTemp, Warning, TEXT("ExtractWaveformFromContentBrowserAsset: File exists at absolute path, loading..."));
                    SourceFilePath = AbsolutePath;
                }
                else
                {
                    // Content Browser asset-specific existing logic (relative path handling)
                    FString AssetPath = SoundWave->GetPathName();
                    FString AssetDirectory = FPaths::GetPath(AssetPath);
                    
                    // Get actual path of Content folder
                    FString ContentDir = FPaths::ProjectContentDir();
                    
                    // Convert /Game/ path to Content/ path
                    FString RelativeAssetPath = AssetDirectory;
                    if (RelativeAssetPath.StartsWith(TEXT("/Game/")))
                    {
                        RelativeAssetPath = RelativeAssetPath.RightChop(6); // Remove "/Game/"
                    }
                    else if (RelativeAssetPath.StartsWith(TEXT("/Game")))
                    {
                        RelativeAssetPath = RelativeAssetPath.RightChop(5); // Remove "/Game"
                    }
                    
                    // Construct actual folder path
                    FString ActualDirectory = FPaths::Combine(ContentDir, RelativeAssetPath);
                    
                    // Full path of the file
                    SourceFilePath = FPaths::Combine(ActualDirectory, FPaths::GetCleanFilename(SourceFilePath));
                    
                    UE_LOG(LogTemp, Warning, TEXT("ExtractWaveformFromContentBrowserAsset: Asset path: %s"), *AssetPath);
                    UE_LOG(LogTemp, Warning, TEXT("ExtractWaveformFromContentBrowserAsset: Asset directory: %s"), *AssetDirectory);
                    UE_LOG(LogTemp, Warning, TEXT("ExtractWaveformFromContentBrowserAsset: Content dir: %s"), *ContentDir);
                    UE_LOG(LogTemp, Warning, TEXT("ExtractWaveformFromContentBrowserAsset: Actual directory: %s"), *ActualDirectory);
                    UE_LOG(LogTemp, Warning, TEXT("ExtractWaveformFromContentBrowserAsset: Final path: %s"), *SourceFilePath);
                }
                
                if (FPaths::FileExists(SourceFilePath))
                {
                    UE_LOG(LogTemp, Warning, TEXT("ExtractWaveformFromContentBrowserAsset: File exists, loading..."));
                    
                    // Read audio file as binary
                    TArray<uint8> FileData;
                    if (FFileHelper::LoadFileToArray(FileData, *SourceFilePath))
                    {
                        UE_LOG(LogTemp, Warning, TEXT("ExtractWaveformFromContentBrowserAsset: File loaded, size: %d bytes"), FileData.Num());
                        
                        // Parse WAV directly without Base64 roundtrip!
                        USoundWave* TempWave = ParseWavAndCreateSoundWave(FileData, TEXT("TempWaveform"));
                        if (TempWave && TempWave->RawPCMData && TempWave->RawPCMDataSize > 0)
                        {
                            WaveformData = ExtractWaveformData(TempWave);
                            UE_LOG(LogTemp, Warning, TEXT("ExtractWaveformFromContentBrowserAsset: SUCCESS! Extracted %d waveform samples"), WaveformData.Num());
                            return WaveformData;
                        }
                        else
                        {
                            UE_LOG(LogTemp, Error, TEXT("ExtractWaveformFromContentBrowserAsset: Failed to parse WAV data"));
                        }
                    }
                    else
                    {
                        UE_LOG(LogTemp, Error, TEXT("ExtractWaveformFromContentBrowserAsset: Failed to load file: %s"), *SourceFilePath);
                    }
                }
                else
                {
                    UE_LOG(LogTemp, Error, TEXT("ExtractWaveformFromContentBrowserAsset: File not found: %s"), *SourceFilePath);
                }
            }
            else
            {
                UE_LOG(LogTemp, Error, TEXT("ExtractWaveformFromContentBrowserAsset: SourceFilePath is empty"));
            }
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("ExtractWaveformFromContentBrowserAsset: No SourceFiles"));
        }
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("ExtractWaveformFromContentBrowserAsset: No AssetImportData"));
    }
    
    UE_LOG(LogTemp, Error, TEXT("ExtractWaveformFromContentBrowserAsset: FAILED to extract waveform for %s"), *SoundWave->GetName());
    return WaveformData;
}

TArray<float> FAudioUtils::ExtractStereoWaveformFromContentBrowserAsset(USoundWave* SoundWave)
{
    TArray<float> WaveformData;
    if (!SoundWave)
    {
        UE_LOG(LogTemp, Error, TEXT("ExtractStereoWaveformFromContentBrowserAsset: SoundWave is NULL"));
        return WaveformData;
    }

    UE_LOG(LogTemp, Warning, TEXT("ExtractStereoWaveformFromContentBrowserAsset: Processing %s"), *SoundWave->GetName());

    // Check if RawPCMData exists (created from Base64)
    if (SoundWave->RawPCMData && SoundWave->RawPCMDataSize > 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("ExtractStereoWaveformFromContentBrowserAsset: Using existing RawPCMData"));
        return ExtractStereoWaveformData(SoundWave);
    }

    // **New logic: direct PCM decompression from uasset**
    UE_LOG(LogTemp, Warning, TEXT("ExtractStereoWaveformFromContentBrowserAsset: Attempting direct decompression"));
    
    TArray<uint8> DecodedPCM = DecompressSoundWaveToPCM(SoundWave);
    if (DecodedPCM.Num() > 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("ExtractStereoWaveformFromContentBrowserAsset: Direct decompression successful, %d bytes"), DecodedPCM.Num());
        
        // Set RawPCMData temporarily to reuse existing logic
        USoundWaveProcedural* TempSoundWave = NewObject<USoundWaveProcedural>();
        TempSoundWave->SetSampleRate(SoundWave->GetSampleRateForCurrentPlatform());
        TempSoundWave->NumChannels = SoundWave->NumChannels;
        TempSoundWave->RawPCMDataSize = DecodedPCM.Num();
        TempSoundWave->RawPCMData = static_cast<uint8*>(FMemory::Malloc(DecodedPCM.Num()));
        FMemory::Memcpy(TempSoundWave->RawPCMData, DecodedPCM.GetData(), DecodedPCM.Num());
        
        WaveformData = ExtractStereoWaveformData(TempSoundWave);
        
        UE_LOG(LogTemp, Warning, TEXT("ExtractStereoWaveformFromContentBrowserAsset: SUCCESS via direct decompression! Extracted %d waveform samples"), WaveformData.Num());
        return WaveformData;
    }

    // Fallback: original file path based logic
    if (SoundWave->AssetImportData)
    {
        UE_LOG(LogTemp, Warning, TEXT("ExtractStereoWaveformFromContentBrowserAsset: Has AssetImportData"));

        const TArray<FAssetImportInfo::FSourceFile>& SourceFiles = SoundWave->AssetImportData->SourceData.SourceFiles;
        UE_LOG(LogTemp, Warning, TEXT("ExtractStereoWaveformFromContentBrowserAsset: SourceFiles count: %d"), SourceFiles.Num());

        if (SourceFiles.Num() > 0)
        {
            FString SourceFilePath = SourceFiles[0].RelativeFilename;
            UE_LOG(LogTemp, Warning, TEXT("ExtractStereoWaveformFromContentBrowserAsset: Original path: %s"), *SourceFilePath);

            if (!SourceFilePath.IsEmpty())
            {
                // Find WAV file in the same folder as SoundWave asset
                FString AssetPath = SoundWave->GetPathName();
                FString AssetDirectory = FPaths::GetPath(AssetPath);

                // Get actual path of Content folder
                FString ContentDir = FPaths::ProjectContentDir();

                // Convert /Game/ path to Content/ path
                FString RelativeAssetPath = AssetDirectory;
                if (RelativeAssetPath.StartsWith(TEXT("/Game/")))
                {
                    RelativeAssetPath = RelativeAssetPath.RightChop(6); // Remove "/Game/"
                }
                else if (RelativeAssetPath.StartsWith(TEXT("/Game")))
                {
                    RelativeAssetPath = RelativeAssetPath.RightChop(5); // Remove "/Game"
                }

                // Construct actual folder path
                FString ActualDirectory = FPaths::Combine(ContentDir, RelativeAssetPath);

                // Full path of the WAV file
                SourceFilePath = FPaths::Combine(ActualDirectory, FPaths::GetCleanFilename(SourceFilePath));

                UE_LOG(LogTemp, Warning, TEXT("ExtractStereoWaveformFromContentBrowserAsset: Asset path: %s"), *AssetPath);
                UE_LOG(LogTemp, Warning, TEXT("ExtractStereoWaveformFromContentBrowserAsset: Asset directory: %s"), *AssetDirectory);
                UE_LOG(LogTemp, Warning, TEXT("ExtractStereoWaveformFromContentBrowserAsset: Content dir: %s"), *ContentDir);
                UE_LOG(LogTemp, Warning, TEXT("ExtractStereoWaveformFromContentBrowserAsset: Actual directory: %s"), *ActualDirectory);
                UE_LOG(LogTemp, Warning, TEXT("ExtractStereoWaveformFromContentBrowserAsset: Final WAV path: %s"), *SourceFilePath);

                if (FPaths::FileExists(SourceFilePath))
                {
                    UE_LOG(LogTemp, Warning, TEXT("ExtractStereoWaveformFromContentBrowserAsset: File exists, loading..."));

                    // Read WAV file as binary
                    TArray<uint8> FileData;
                    if (FFileHelper::LoadFileToArray(FileData, *SourceFilePath))
                    {
                        UE_LOG(LogTemp, Warning, TEXT("ExtractStereoWaveformFromContentBrowserAsset: File loaded, size: %d bytes"), FileData.Num());

                        // Parse WAV directly without Base64 roundtrip!
                        USoundWave* TempWave = ParseWavAndCreateSoundWave(FileData, TEXT("TempWaveform"));
                        if (TempWave && TempWave->RawPCMData && TempWave->RawPCMDataSize > 0)
                        {
                            WaveformData = ExtractStereoWaveformData(TempWave);
                            UE_LOG(LogTemp, Warning, TEXT("ExtractStereoWaveformFromContentBrowserAsset: SUCCESS! Extracted %d stereo waveform samples"), WaveformData.Num());
                            return WaveformData;
                        }
                        else
                        {
                            UE_LOG(LogTemp, Error, TEXT("ExtractStereoWaveformFromContentBrowserAsset: Failed to parse WAV data"));
                        }
                    }
                    else
                    {
                        UE_LOG(LogTemp, Error, TEXT("ExtractStereoWaveformFromContentBrowserAsset: Failed to load file: %s"), *SourceFilePath);
                    }
                }
                else
                {
                    UE_LOG(LogTemp, Error, TEXT("ExtractStereoWaveformFromContentBrowserAsset: File not found: %s"), *SourceFilePath);
                }
            }
            else
            {
                UE_LOG(LogTemp, Error, TEXT("ExtractStereoWaveformFromContentBrowserAsset: SourceFilePath is empty"));
            }
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("ExtractStereoWaveformFromContentBrowserAsset: No SourceFiles"));
        }
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("ExtractStereoWaveformFromContentBrowserAsset: No AssetImportData"));
    }

    UE_LOG(LogTemp, Error, TEXT("ExtractStereoWaveformFromContentBrowserAsset: FAILED to extract stereo waveform for %s"), *SoundWave->GetName());
    return WaveformData;
}

bool FAudioUtils::OpenAudioFileDialog(TArray<FString>& OutFilePaths, const FString& DialogTitle)
{
    IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
    if (!DesktopPlatform)
    {
        return false;
    }

    // Get last directory path (use project directory if not found)
    FString DefaultPath = FPaths::ProjectContentDir();

    // Set file filter - only WAV files supported
    FString FileTypes = NSLOCTEXT("VarcoSound", "AudioFileFilter", "Audio files (*.wav)|*.wav").ToString();

    // Get parent window handle - modified method
    void* ParentWindowHandle = nullptr;
    if (FSlateApplication::IsInitialized())
    {
        TSharedPtr<SWindow> ParentWindow = FSlateApplication::Get().GetActiveTopLevelWindow();
        if (ParentWindow.IsValid() && ParentWindow->GetNativeWindow().IsValid())
        {
            ParentWindowHandle = ParentWindow->GetNativeWindow()->GetOSWindowHandle();
        }
    }

    // Open file dialog
    return DesktopPlatform->OpenFileDialog(
        ParentWindowHandle,
        DialogTitle,
        DefaultPath,
        TEXT(""),
        FileTypes,
        EFileDialogFlags::None,
        OutFilePaths
    );
}

TArray<uint8> FAudioUtils::DecompressSoundWaveToPCM(USoundWave* SoundWave)
{
    TArray<uint8> PCMData;
    
    if (!SoundWave)
    {
        UE_LOG(LogTemp, Error, TEXT("DecompressSoundWaveToPCM: SoundWave is NULL"));
        return PCMData;
    }

    // Decode compressed data to PCM
    TArray<uint8> RawWaveData;
    uint32 SampleRate = 0;
    uint16 NumChannels = 0;  // uint32 -> uint16 modification
    
    // Use USoundWave's GetImportedSoundWaveData
    if (!SoundWave->GetImportedSoundWaveData(RawWaveData, SampleRate, NumChannels))
    {
        UE_LOG(LogTemp, Warning, TEXT("DecompressSoundWaveToPCM: GetImportedSoundWaveData failed for %s"), *SoundWave->GetName());
        return PCMData;
    }

    UE_LOG(LogTemp, Log, TEXT("DecompressSoundWaveToPCM: Decoded %d bytes, %d channels, %d Hz"), 
        RawWaveData.Num(), NumChannels, SampleRate);

    return RawWaveData;
}

FString FAudioUtils::GetSoundWaveBase64(USoundWave* SoundWave)
{
    if (!SoundWave)
    {
        return FString();
    }

    // 1. Check if original source file exists, and if so, convert file content directly to Base64
    if (FAssetSelectionHelpers::HasValidSourceFile(SoundWave))
    {
        FString FilePath = FAssetSelectionHelpers::GetSoundWaveSourceFilePath(SoundWave);
        TArray<uint8> FileData;
        if (FFileHelper::LoadFileToArray(FileData, *FilePath))
        {
             UE_LOG(LogTemp, Log, TEXT("GetSoundWaveBase64: Loaded from file %s"), *FilePath);
             return FBase64::Encode(FileData);
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("GetSoundWaveBase64: Failed to load existing file %s, falling back to uasset conversion"), *FilePath);
        }
    }

    // 2. If file doesn't exist or load fails, convert uasset data
    return ConvertSoundWaveToWavBase64(SoundWave);
}

FString FAudioUtils::ConvertSoundWaveToWavBase64(USoundWave* SoundWave)
{
    if (!SoundWave)
    {
        UE_LOG(LogTemp, Error, TEXT("ConvertSoundWaveToWavBase64: SoundWave is NULL"));
        return FString();
    }

    // 1. PCM decoding
    TArray<uint8> PCMData = DecompressSoundWaveToPCM(SoundWave);
    if (PCMData.Num() == 0)
    {
        UE_LOG(LogTemp, Error, TEXT("ConvertSoundWaveToWavBase64: Failed to decompress PCM for %s"), *SoundWave->GetName());
        return FString();
    }

    // 2. WAV header structure
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

    // 3. Set header
    WAVHeader Header;
    Header.NumChannels = SoundWave->NumChannels;
    Header.SampleRate = SoundWave->GetSampleRateForCurrentPlatform();
    Header.BitsPerSample = 16;
    Header.ByteRate = Header.SampleRate * Header.NumChannels * Header.BitsPerSample / 8;
    Header.BlockAlign = Header.NumChannels * Header.BitsPerSample / 8;
    Header.Subchunk2Size = PCMData.Num();
    Header.ChunkSize = 36 + Header.Subchunk2Size;

    // 4. Combine header + PCM data
    TArray<uint8> WavData;
    WavData.SetNum(sizeof(WAVHeader) + PCMData.Num());
    FMemory::Memcpy(WavData.GetData(), &Header, sizeof(WAVHeader));
    FMemory::Memcpy(WavData.GetData() + sizeof(WAVHeader), PCMData.GetData(), PCMData.Num());

    // 5. Base64 encoding
    FString Base64String = FBase64::Encode(WavData);
    
    UE_LOG(LogTemp, Log, TEXT("ConvertSoundWaveToWavBase64: Converted %s to base64 (PCM: %d bytes, WAV: %d bytes, Base64: %d chars)"),
        *SoundWave->GetName(), PCMData.Num(), WavData.Num(), Base64String.Len());

    return Base64String;
} 