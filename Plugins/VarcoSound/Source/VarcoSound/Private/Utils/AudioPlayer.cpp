// Copyright Epic Games, Inc. All Rights Reserved.

#include "Utils/AudioPlayer.h"
#include "Utils/VarcoSoundSettings.h"
#include "Components/AudioComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Editor.h"
#include "Engine/World.h"
#include "Sound/SoundWaveProcedural.h"
#include "Sound/SoundBase.h"
#include "Async/Async.h"
#include "Containers/Ticker.h"

// Debug logging macro controlled by settings
#define VS_DEV_LOG(Verbosity, Format, ...) \
	if (VarcoSoundSettings::IsDebugLoggingEnabled()) \
	{ \
		UE_LOG(LogTemp, Verbosity, Format, ##__VA_ARGS__); \
	}

FAudioPlayer::~FAudioPlayer()
{
	Stop();
}

void FAudioPlayer::SetSound(USoundBase* NewSound, FOnAudioPlaybackPercentNative::FDelegate OnPlaybackPercent)
{
	// Clear previous before stopping to avoid copying invalid delegate target
	VS_DEV_LOG(Log, TEXT("[Player::SetSound] prev=%p new=%p src=%s"), CurrentSound.Get(), NewSound, *SourceTag);
	PlaybackPercentDelegate.Unbind();
	Stop();
	CurrentSound = NewSound;
	PlaybackPercentDelegate = OnPlaybackPercent;
	bIsPaused = false;
	LastPlaybackPercent = 0.0f;
	bPlaybackCompletionHandled = false;
	PlaybackSessionId.Store(0);
	bSawPlaybackProgress = false;

	if (CurrentSound)
	{
		const bool bIsProcedural = Cast<USoundWaveProcedural>(CurrentSound) != nullptr;
		int32 RawSize = 0;
		int32 NumChannels = 0;
		int32 SampleRate = 0;
		float Duration = 0.0f;
		if (const USoundWave* SoundWave = Cast<USoundWave>(CurrentSound))
		{
			RawSize = SoundWave->RawPCMDataSize;
			NumChannels = SoundWave->NumChannels;
			SampleRate = SoundWave->GetSampleRateForCurrentPlatform();
			Duration = SoundWave->Duration;
		}
		VS_DEV_LOG(Log, TEXT("[Player::SetSound] name=%s procedural=%d raw=%d ch=%d sr=%d dur=%.3f src=%s"),
			*CurrentSound->GetName(), bIsProcedural ? 1 : 0, RawSize, NumChannels, SampleRate, Duration, *SourceTag);
	}
}

void FAudioPlayer::Play(float StartTime)
{
	if (!CurrentSound)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Player::Play] no sound"));
		return;
	}

	const double Now = FPlatformTime::Seconds();
	// 너무 촘촘한 중복 Play 호출 억제 (같은 start로 5ms 내 재호출 시 무시)
	if (FMath::IsNearlyEqual(StartTime, LastPlayStartTimeSec, 1e-3f) && (Now - LastPlayCallSeconds) < 0.03)
	{
		VS_DEV_LOG(Verbose, TEXT("[Player::Play] coalesced duplicate start=%.3f"), StartTime);
		return;
	}
	LastPlayCallSeconds = Now;
	LastPlayStartTimeSec = StartTime;
	CancelPendingFinish();
	bPlaybackCompletionHandled = false;
	++PlaybackSessionId;
	bSawPlaybackProgress = (StartTime > 0.0f); // 시킹 시작 시 진행한 것으로 간주
	float EffectiveStartTime = StartTime;
	bool bProceduralQueued = false;

	if (USoundWaveProcedural* ProceduralSoundWave = Cast<USoundWaveProcedural>(CurrentSound))
	{
		if (!bIsPaused)
		{
			// Stop component before mutating procedural queue to avoid race with audio render thread
			{
				FScopeLock Lock(&ComponentMutex);
				if (EditorAudioComponent && IsValid(EditorAudioComponent) && EditorAudioComponent->IsValidLowLevelFast() && !EditorAudioComponent->HasAnyFlags(RF_BeginDestroyed))
				{
					EditorAudioComponent->Stop();
				}
			}

			// 오프셋 시킹을 위해 StartTime에 해당하는 바이트부터 큐잉
			const int32 SampleRate = ProceduralSoundWave->GetSampleRateForCurrentPlatform();
			const int32 NumChannels = FMath::Max(1, ProceduralSoundWave->NumChannels);
			const int32 BytesPerSample = 2; // 16-bit PCM
			const int32 BytesPerFrame = NumChannels * BytesPerSample;
			const int32 BytesPerSecond = SampleRate * NumChannels * BytesPerSample;
			const int32 RawSize = ProceduralSoundWave->RawPCMDataSize;
			const int32 RawAlignedSize = (BytesPerFrame > 0) ? (RawSize - (RawSize % BytesPerFrame)) : RawSize;
			const float Duration = ProceduralSoundWave->Duration;

			int32 StartBytes = 0;
			if (RawAlignedSize > 0 && Duration > 0.0f && FMath::IsFinite(Duration))
			{
				const float ClampedStart = FMath::Clamp(StartTime, 0.0f, Duration);
				const float Percent = Duration > 0.0f ? (ClampedStart / Duration) : 0.0f;
				StartBytes = FMath::Clamp((int32)FMath::RoundToInt(Percent * (float)RawAlignedSize), 0, RawAlignedSize);
			}
			else if (BytesPerSecond > 0)
			{
				StartBytes = FMath::Clamp((int32)FMath::RoundToInt(StartTime * BytesPerSecond), 0, RawAlignedSize);
			}

			if (BytesPerFrame > 0)
			{
				StartBytes = StartBytes - (StartBytes % BytesPerFrame);
			}
			int32 RemainingBytes = RawAlignedSize - StartBytes;
			if (BytesPerFrame > 0)
			{
				RemainingBytes = RemainingBytes - (RemainingBytes % BytesPerFrame);
			}
			bProceduralQueued = (RemainingBytes > 0);
			EffectiveStartTime = 0.0f;
			VS_DEV_LOG(Log, TEXT("[Player::Play] ResetAudio+Queue raw=%d rawAligned=%d startBytes=%d remain=%d frame=%d start=%.3f dur=%.3f sr=%d ch=%d play=%.3f src=%s"),
				RawSize, RawAlignedSize, StartBytes, RemainingBytes, BytesPerFrame, StartTime, Duration, SampleRate, NumChannels, EffectiveStartTime, *SourceTag);
			ProceduralSoundWave->ResetAudio();
			if (ProceduralSoundWave->RawPCMData && RemainingBytes > 0)
			{
				ProceduralSoundWave->QueueAudio(ProceduralSoundWave->RawPCMData + StartBytes, RemainingBytes);
			}
			else
			{
				VS_DEV_LOG(Warning, TEXT("[Player::Play] Procedural queue skipped raw=%d remain=%d"), ProceduralSoundWave->RawPCMDataSize, RemainingBytes);
			}
		}
	}

	{
		FScopeLock Lock(&ComponentMutex);
		if (bIsPaused && EditorAudioComponent && IsValid(EditorAudioComponent) && EditorAudioComponent->IsValidLowLevel() && !EditorAudioComponent->HasAnyFlags(RF_BeginDestroyed))
		{
			VS_DEV_LOG(Log, TEXT("[Player::Play] resume"));
			EditorAudioComponent->SetPaused(false);
			bIsPaused = false;
			return;
		}

		// 재생 중/유효 컴포넌트가 있으면 컴포넌트 재사용하여 시킹. 재생 중복 초기화 방지
		if (EditorAudioComponent && IsValid(EditorAudioComponent) && EditorAudioComponent->IsValidLowLevelFast() && !EditorAudioComponent->HasAnyFlags(RF_BeginDestroyed))
		{
			VS_DEV_LOG(Log, TEXT("[Player::Play] seek existing comp=%p start=%.3f play=%.3f queued=%d src=%s"),
				EditorAudioComponent.Get(), StartTime, EffectiveStartTime, bProceduralQueued ? 1 : 0, *SourceTag);
			EditorAudioComponent->Stop();
			EditorAudioComponent->Play(EffectiveStartTime);
			bIsPaused = false;
			return;
		}
	}

	Stop();
	InitializeAudioComponent();

	{
		FScopeLock Lock(&ComponentMutex);
		if (EditorAudioComponent && IsValid(EditorAudioComponent) && EditorAudioComponent->IsValidLowLevelFast() && !EditorAudioComponent->HasAnyFlags(RF_BeginDestroyed))
		{
			VS_DEV_LOG(Log, TEXT("[Player::Play] component=%p Play start=%.3f play=%.3f queued=%d src=%s"),
				EditorAudioComponent.Get(), StartTime, EffectiveStartTime, bProceduralQueued ? 1 : 0, *SourceTag);
			EditorAudioComponent->Play(EffectiveStartTime);
			VS_DEV_LOG(Log, TEXT("[Player::Play] playing=%d"), EditorAudioComponent->IsPlaying() ? 1 : 0);
			bIsPaused = false;
		}
	}
}

void FAudioPlayer::Pause()
{
	FScopeLock Lock(&ComponentMutex);
	UAudioComponent* Local = EditorAudioComponent;
	if (!Local || !IsValid(Local))
	{
		return;
	}
	if (!Local->IsValidLowLevel() || Local->HasAnyFlags(RF_BeginDestroyed))
	{
		EditorAudioComponent = nullptr;
		return;
	}
	const bool bCurrentlyPlaying = !bIsPaused && Local->IsPlaying();
	if (bCurrentlyPlaying)
	{
		VS_DEV_LOG(Log, TEXT("[Player::Pause]"));
		Local->SetPaused(true);
		bIsPaused = true;
	}
}

void FAudioPlayer::Stop()
{
	VS_DEV_LOG(Log, TEXT("[Player::Stop]"));
	CancelPendingFinish();
	TeardownAudioComponent();
	bIsPaused = false;
	bSawPlaybackProgress = false;
}

bool FAudioPlayer::IsPlaying() const
{
	FScopeLock Lock(&ComponentMutex);
	UAudioComponent* Local = EditorAudioComponent;
	if (!Local || !IsValid(Local))
	{
		return false;
	}
	if (!Local->IsValidLowLevelFast() || Local->HasAnyFlags(RF_BeginDestroyed))
	{
		return false;
	}
	if (bIsPaused)
	{
		return false;
	}
	bool bResult = false;
	// Guard IsPlaying() against exceptions/crashes
	bResult = Local->IsPlaying();
	return bResult;
}

UAudioComponent* FAudioPlayer::GetAudioComponent() const
{
	FScopeLock Lock(&ComponentMutex);
	UAudioComponent* Local = EditorAudioComponent;
	if (!Local || !IsValid(Local))
	{
		return nullptr;
	}
	if (!Local->IsValidLowLevelFast() || Local->HasAnyFlags(RF_BeginDestroyed))
	{
		return nullptr;
	}
	return Local;
}

void FAudioPlayer::InitializeAudioComponent()
{
	if (!GEditor || !CurrentSound)
	{
		return;
	}

	TeardownAudioComponent();

	UWorld* EditorWorld = GEditor->GetEditorWorldContext().World();
	if (!EditorWorld)
	{
		return;
	}

	UAudioComponent* NewComp = UGameplayStatics::CreateSound2D(EditorWorld, CurrentSound, 1.0f, 1.0f, 0.0f, nullptr, true);
	{
		FScopeLock Lock(&ComponentMutex);
		EditorAudioComponent = NewComp;
		if (EditorAudioComponent && IsValid(EditorAudioComponent))
		{
			VS_DEV_LOG(Log, TEXT("[Player::Init] comp=%p sound=%p"), EditorAudioComponent.Get(), CurrentSound.Get());
			
			// 내부 퍼센트 추적 콜백 먼저 등록 (Procedural 종료 감지용)
			FOnAudioPlaybackPercentNative::FDelegate InternalPercentDelegate;
			InternalPercentDelegate.BindRaw(this, &FAudioPlayer::OnPlaybackPercentInternal);
			EditorAudioComponent->OnAudioPlaybackPercentNative.Add(InternalPercentDelegate);
			
			// 외부 퍼센트 콜백 등록
			if (PlaybackPercentDelegate.IsBound())
			{
				EditorAudioComponent->OnAudioPlaybackPercentNative.Add(PlaybackPercentDelegate);
			}
			
			EditorAudioComponent->OnAudioFinishedNative.AddRaw(this, &FAudioPlayer::OnAudioFinishedInternal);
		}
		else
		{
			EditorAudioComponent = nullptr;
		}
	}
}

void FAudioPlayer::TeardownAudioComponent()
{
	UAudioComponent* Temp = nullptr;
	{
		FScopeLock Lock(&ComponentMutex);
		if (EditorAudioComponent)
		{
			Temp = EditorAudioComponent;
			EditorAudioComponent = nullptr;
		}
	}

	if (Temp)
	{
		const bool bTempValid = (Temp != nullptr) && IsValid(Temp) && Temp->IsValidLowLevelFast() && !Temp->HasAnyFlags(RF_BeginDestroyed);
		if (bTempValid)
		{
			bool bPlaying = false;
			bPlaying = Temp->IsPlaying();
			if (bPlaying)
			{
				VS_DEV_LOG(Log, TEXT("[Player::Teardown] stop comp=%p"), Temp);
				Temp->Stop();
			}
			Temp->OnAudioPlaybackPercentNative.Clear();
			Temp->OnAudioFinishedNative.Clear();
		}
	}
}

void FAudioPlayer::OnPlaybackPercentInternal(const UAudioComponent* AudioComp, const USoundWave* SoundWave, float Percent)
{
	const float PreviousPercent = LastPlaybackPercent;
	LastPlaybackPercent = Percent;

	if (Percent > 0.02f && Percent < 0.98f)
	{
		if (!bSawPlaybackProgress)
		{
			VS_DEV_LOG(Log, TEXT("[Player::Percent] first-progress pct=%.3f prev=%.3f src=%s"), Percent, PreviousPercent, *SourceTag);
		}
		bSawPlaybackProgress = true;
	}

	// Procedural 재생이 99.9% 이상 진행되면 종료 처리
	if (Cast<USoundWaveProcedural>(CurrentSound))
	{
		float CompletionPercent = 0.999f;
		if (const USoundWave* CurrentWave = Cast<USoundWave>(CurrentSound))
		{
			const float Duration = CurrentWave->Duration;
			if (Duration > 0.0f && LastPlayStartTimeSec > 0.0f && FMath::IsFinite(Duration))
			{
				const float RemainingSeconds = FMath::Clamp(Duration - LastPlayStartTimeSec, 0.0f, Duration);
				const float RemainingPercent = (Duration > 0.0f) ? (RemainingSeconds / Duration) : 0.0f;
				if (RemainingPercent > KINDA_SMALL_NUMBER)
				{
					CompletionPercent = RemainingPercent;
				}
			}
		}

		if (Percent >= CompletionPercent && !bPlaybackCompletionHandled)
		{
			if (!bSawPlaybackProgress)
			{
				VS_DEV_LOG(Verbose, TEXT("[Player::Percent] Ignoring premature completion percent=%.3f prev=%.3f"), Percent, PreviousPercent);
				return;
			}
			bPlaybackCompletionHandled = true;
			const uint32 CapturedSessionId = PlaybackSessionId.Load();
			VS_DEV_LOG(Log, TEXT("[Player::Percent] Procedural playback completed at %.3f (threshold=%.3f session=%u)"), Percent, CompletionPercent, CapturedSessionId);
			
			// 짧은 딜레이 후 종료 처리 (IsPlaying이 false로 바뀔 시간 확보)
			AsyncTask(ENamedThreads::GameThread, [this, CapturedSessionId]()
			{
				CancelPendingFinish();
				FinishTickerHandle = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([this, CapturedSessionId](float DeltaTime) mutable -> bool
				{
					if (CapturedSessionId != PlaybackSessionId.Load())
					{
						VS_DEV_LOG(Log, TEXT("[Player::FinishTicker] Session mismatch (captured=%u current=%u), skipping"), CapturedSessionId, PlaybackSessionId.Load());
						FinishTickerHandle.Reset();
						return false;
					}
					bIsPaused = false;
					bSawPlaybackProgress = false;
					TeardownAudioComponent();
					if (OnFinishedCallback)
					{
						OnFinishedCallback();
					}
					FinishTickerHandle.Reset();
					return false; // 한 번만 실행
				}), 0.05f); // 50ms 딜레이
			});
		}
	}
}

void FAudioPlayer::OnAudioFinishedInternal(UAudioComponent* FinishedComponent)
{
	UAudioComponent* Local = GetAudioComponent();
	if (!Local || FinishedComponent != Local)
	{
		return;
	}

	// Procedural은 이미 OnPlaybackPercentInternal에서 처리되므로 여기서는 무시
	if (Cast<USoundWaveProcedural>(CurrentSound))
	{
		VS_DEV_LOG(Log, TEXT("[Player::Finished] Procedural - handled by percent callback"));
		return;
	}

	if (bLooping)
	{
		// Restart from beginning
		VS_DEV_LOG(Log, TEXT("[Player::Finished] loop restart"));
		Play(0.0f);
		return;
	}

	// Transition to stopped state
	bIsPaused = false;
	TeardownAudioComponent();
	bSawPlaybackProgress = false;

	if (OnFinishedCallback)
	{
		VS_DEV_LOG(Log, TEXT("[Player::Finished] callback"));
		AsyncTask(ENamedThreads::GameThread, [Cb = OnFinishedCallback]() { Cb(); });
	}
} 

void FAudioPlayer::CancelPendingFinish()
{
	if (FinishTickerHandle.IsValid())
	{
		FTSTicker::GetCoreTicker().RemoveTicker(FinishTickerHandle);
		FinishTickerHandle.Reset();
	}
}