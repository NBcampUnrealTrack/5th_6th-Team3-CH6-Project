// Copyright Epic Games, Inc. All Rights Reserved.

#ifndef VarcoSound_AUDIOUTILS_H
#define VarcoSound_AUDIOUTILS_H

#include "CoreMinimal.h"
#include "Delegates/DelegateCombinations.h"
#include "Components/AudioComponent.h" // FOnAudioPlaybackPercentNative 때문에 추가

class USoundBase;
class USoundWave;
class UAudioComponent;

class FAudioUtils
{
public:
    // 새로운 오디오 플레이어 API
    static void SetSound(USoundBase* NewSound, FOnAudioPlaybackPercentNative::FDelegate OnPlaybackPercent = FOnAudioPlaybackPercentNative::FDelegate());
    static void Play(float StartTime = 0.0f);
    static void Pause();
    static void Stop();
    static bool IsPlaying();
    static bool IsPaused();
    
    // 유틸리티 함수
    static USoundWave* CreateSoundWaveFromBase64(const FString& Base64String);
    static USoundWave* CreateSoundWaveFromFile(const FString& FilePath);
    static USoundWave* CreateSoundWaveFromMp3File(const FString& FilePath, const TArray<uint8>& RawFileData);
    static USoundWave* ParseWavAndCreateSoundWave(const TArray<uint8>& WavData, const FString& Name);
    static TArray<uint8> DecodeMp3ToPCM(const TArray<uint8>& Mp3Data, uint32& OutSampleRate, uint16& OutNumChannels);
    static TArray<float> ExtractWaveformData(const USoundWave* SoundWave);
    static TArray<float> ExtractStereoWaveformData(const USoundWave* SoundWave);
    static TArray<float> ExtractWaveformFromContentBrowserAsset(USoundWave* SoundWave);
    static TArray<float> ExtractStereoWaveformFromContentBrowserAsset(USoundWave* SoundWave);
    static bool OpenAudioFileDialog(TArray<FString>& OutFilePaths, const FString& DialogTitle = TEXT("Open Audio File"));
    
    /** uasset 오디오를 WAV 형식 base64로 변환 (API 전송용) */
    static FString ConvertSoundWaveToWavBase64(USoundWave* SoundWave);

    /** USoundWave에서 Base64 WAV 문자열 추출 (파일 유무에 따라 최적의 방법 선택) */
    static FString GetSoundWaveBase64(USoundWave* SoundWave);
    
    /** uasset 오디오를 PCM으로 디코딩 */
    static TArray<uint8> DecompressSoundWaveToPCM(USoundWave* SoundWave);

    // 내부 접근용 (UI 업데이트 등)
    static UAudioComponent* GetEditorAudioComponent();

private:
    static void InitializeAudioComponent();
    static void TeardownAudioComponent();

    static TObjectPtr<UAudioComponent> EditorAudioComponent;
    static TObjectPtr<USoundBase> CurrentSound;
    static FOnAudioPlaybackPercentNative::FDelegate PlaybackPercentDelegate;
    static bool bIsPaused;
};

#endif //VarcoSound_AUDIOUTILS_H