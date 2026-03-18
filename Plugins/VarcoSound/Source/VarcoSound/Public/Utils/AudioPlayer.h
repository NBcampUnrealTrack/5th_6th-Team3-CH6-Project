// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Delegates/DelegateCombinations.h"
#include "Components/AudioComponent.h"
#include "HAL/CriticalSection.h"
#include "Containers/Ticker.h"
#include "Templates/Atomic.h"

class USoundBase;
class USoundWave;
class UAudioComponent;

/**
 * Per-instance editor audio player to avoid shared global state.
 */
class FAudioPlayer
{
public:
	FAudioPlayer() = default;
	~FAudioPlayer();

	void SetSound(USoundBase* NewSound, FOnAudioPlaybackPercentNative::FDelegate OnPlaybackPercent = FOnAudioPlaybackPercentNative::FDelegate());
	void SetSourceTag(const FString& InTag) { SourceTag = InTag; }
	const FString& GetSourceTag() const { return SourceTag; }
	void Play(float StartTime = 0.0f);
	void Pause();
	void Stop();
	bool IsPlaying() const;
	bool IsPaused() const { return bIsPaused; }

	UAudioComponent* GetAudioComponent() const;

	// Controls
	void SetLooping(bool bInLooping) { bLooping = bInLooping; }
	void SetOnFinished(TFunction<void()> InCallback) { OnFinishedCallback = MoveTemp(InCallback); }

private:
	void InitializeAudioComponent();
	void TeardownAudioComponent();
	void OnAudioFinishedInternal(UAudioComponent* FinishedComponent);
	void OnPlaybackPercentInternal(const UAudioComponent* AudioComp, const USoundWave* SoundWave, float Percent);
	void CancelPendingFinish();

private:
	mutable FCriticalSection ComponentMutex;
	TObjectPtr<UAudioComponent> EditorAudioComponent = nullptr;
	TObjectPtr<USoundBase> CurrentSound = nullptr;
	FOnAudioPlaybackPercentNative::FDelegate PlaybackPercentDelegate;
	bool bIsPaused = false;
	bool bLooping = false;
	TFunction<void()> OnFinishedCallback;

	// State for duplicate Play calls, resuming/stopping differentiation
	double LastPlayCallSeconds = 0.0;
	float LastPlayStartTimeSec = 0.0f;

	// Procedural playback completion detection
	float LastPlaybackPercent = 0.0f;
	bool bPlaybackCompletionHandled = false;
	FTSTicker::FDelegateHandle FinishTickerHandle;
	TAtomic<uint32> PlaybackSessionId { 0 };
	bool bSawPlaybackProgress = false;

	FString SourceTag = TEXT("Unknown");
}; 