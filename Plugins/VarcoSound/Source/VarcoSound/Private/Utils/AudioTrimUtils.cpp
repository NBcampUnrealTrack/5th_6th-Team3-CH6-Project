#include "Utils/AudioTrimUtils.h"

#include "Sound/SoundWave.h"
#include "Sound/SoundWaveProcedural.h"

static FORCEINLINE float ReadInt16AsNormalizedFloat(const uint8* Data)
{
    const int16 Sample16 = static_cast<int16>(Data[0] | (Data[1] << 8));
    return FMath::Clamp(static_cast<float>(Sample16) / 32768.0f, -1.0f, 1.0f);
}

USoundWave* FAudioTrimUtils::TrimSilenceFromEnds(const USoundWave* Source,
                                                  float vadThreshold,
                                                  float preRollSeconds,
                                                  float postRollSeconds,
                                                  float frameDurationSeconds)
{
    if (!Source)
    {
        return nullptr;
    }

    // Raw PCM 필요
    if (!Source->RawPCMData || Source->RawPCMDataSize <= 0)
    {
        return nullptr;
    }

    const uint8* AudioData = Source->RawPCMData;
    const int32 DataSizeBytes = Source->RawPCMDataSize;

    const int32 numChannels = FMath::Max(1, Source->NumChannels);
    const int32 bytesPerSample = 2; // 16-bit
    const int32 bytesPerFrame = bytesPerSample * numChannels; // 한 타임샘플(모든 채널 포함)

    if (bytesPerFrame <= 0)
    {
        return nullptr;
    }

    const int32 numFrames = DataSizeBytes / bytesPerFrame;
    if (numFrames <= 0)
    {
        return nullptr;
    }

    const int32 sampleRate = FMath::Max(1, Source->GetSampleRateForCurrentPlatform());

    // 프레임 윈도우(시간 단위 -> 타임샘플 개수)
    const int32 windowFrames = FMath::Max(1, static_cast<int32>(frameDurationSeconds * static_cast<float>(sampleRate)));
    const int32 preRollFrames = FMath::Max(0, static_cast<int32>(preRollSeconds * static_cast<float>(sampleRate)));
    const int32 postRollFrames = FMath::Max(0, static_cast<int32>(postRollSeconds * static_cast<float>(sampleRate)));

    auto ComputeWindowRMS = [&](int32 startFrameInclusive) -> float
    {
        const int32 endFrameExclusive = FMath::Min(startFrameInclusive + windowFrames, numFrames);
        if (endFrameExclusive <= startFrameInclusive)
        {
            return 0.0f;
        }

        double sumSquares = 0.0;
        int64 countSamples = 0;

        for (int32 f = startFrameInclusive; f < endFrameExclusive; ++f)
        {
            const int32 frameByteOffset = f * bytesPerFrame;
            for (int32 c = 0; c < numChannels; ++c)
            {
                const int32 sampleByteOffset = frameByteOffset + c * bytesPerSample;
                if (sampleByteOffset + 1 < DataSizeBytes)
                {
                    const float sample = ReadInt16AsNormalizedFloat(AudioData + sampleByteOffset);
                    sumSquares += static_cast<double>(sample) * static_cast<double>(sample);
                    ++countSamples;
                }
            }
        }

        if (countSamples == 0)
        {
            return 0.0f;
        }

        const double meanSquares = sumSquares / static_cast<double>(countSamples);
        return static_cast<float>(FMath::Sqrt(meanSquares));
    };

    // 뒤에서부터 활성 구간 찾기
    int32 lastActiveFrame = -1;
    for (int32 f = numFrames - windowFrames; f >= 0; f -= windowFrames)
    {
        const float rms = ComputeWindowRMS(f);
        if (rms > vadThreshold)
        {
            lastActiveFrame = FMath::Min(numFrames, f + postRollFrames);
            break;
        }
    }

    // 모든 프레임이 무음이면 트림하지 않음
    if (lastActiveFrame == -1)
    {
        return nullptr;
    }

    // 앞에서부터 활성 구간 찾기
    int32 firstActiveFrame = -1;
    for (int32 f = 0; f < numFrames; f += windowFrames)
    {
        const float rms = ComputeWindowRMS(f);
        if (rms > vadThreshold)
        {
            firstActiveFrame = FMath::Max(0, f - preRollFrames);
            break;
        }
    }

    if (firstActiveFrame == -1)
    {
        return nullptr;
    }

    if (firstActiveFrame >= lastActiveFrame)
    {
        // 비정상 상황: 트림 후 길이가 0이거나 음수
        return nullptr;
    }

    const int32 startByte = firstActiveFrame * bytesPerFrame;
    const int32 endByte = lastActiveFrame * bytesPerFrame;
    const int32 trimmedSizeBytes = FMath::Clamp(endByte - startByte, 0, DataSizeBytes - startByte);

    if (trimmedSizeBytes <= 0)
    {
        return nullptr;
    }

    // 새 Procedural 사운드웨이브 생성
    USoundWaveProcedural* Trimmed = NewObject<USoundWaveProcedural>(USoundWaveProcedural::StaticClass());
    if (!Trimmed)
    {
        return nullptr;
    }

    Trimmed->Rename(*FString::Printf(TEXT("SoundWave_Trimmed_%s_%d"), *Source->GetName(), FMath::Rand()));

    Trimmed->SetSampleRate(sampleRate);
    Trimmed->NumChannels = numChannels;
    Trimmed->bLooping = false;
    Trimmed->bCanProcessAsync = true;
    Trimmed->VirtualizationMode = EVirtualizationMode::PlayWhenSilent;

    // Raw PCM 복사 (시각화/재생 공용)
    Trimmed->RawPCMDataSize = trimmedSizeBytes;
    Trimmed->RawPCMData = static_cast<uint8*>(FMemory::Malloc(trimmedSizeBytes));
    FMemory::Memcpy(Trimmed->RawPCMData, AudioData + startByte, trimmedSizeBytes);

    // Procedural 큐에 추가 (재생)
    Trimmed->QueueAudio(Trimmed->RawPCMData, trimmedSizeBytes);

    const float bytesPerSecond = static_cast<float>(sampleRate * numChannels * bytesPerSample);
    Trimmed->Duration = bytesPerSecond > 0.0f ? (static_cast<float>(trimmedSizeBytes) / bytesPerSecond) : 0.0f;
    Trimmed->SoundGroup = ESoundGroup::SOUNDGROUP_Default;

    return Trimmed;
} 