// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Sound/SoundWave.h"
#include "UObject/ObjectMacros.h"
#include "AssetRegistry/AssetData.h"
#include "Tabs/IVarcoSoundTab.h"

class FApiClient;
class SWaveformDisplay;
class SAudioResultView;
class SAudioPlayerControls;
class SObjectPropertyEntryBox;
class UAudioComponent;
class SVerticalBox;

class VARCOSOUND_API SAutoLoopTab : public SCompoundWidget, public IVarcoSoundTab
{
public:
	SLATE_BEGIN_ARGS(SAutoLoopTab) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	virtual ~SAutoLoopTab();

	// IVarcoSoundTab interface
	virtual void PauseAllPlayback() override;
	virtual void SetApiKey(const FString& InApiKey) override;

private:
	// UI Event Handlers
	FReply OnCreateLoopButtonClicked();
	bool CanGenerate() const;
	
	// Asset selection
	void OnAssetSelected(const FAssetData& AssetData);
	FString GetCurrentAssetPath() const;
	
	// API Response Handler
	void OnLoopingApiResponse(const FString& AudioBase64);
	
	// Helper functions
	void UpdateInputAudioPlayer();
	void ResetPlaybackState();

private:
	// API Client
	TSharedPtr<FApiClient> ApiClient;
	
	// Audio data
	USoundWave* InputSoundWave;
	
	// UI Widgets
	TSharedPtr<SObjectPropertyEntryBox> AssetSelectorWidget;
	TSharedPtr<SAudioPlayerControls> InputAudioPlayerWidget;
	TSharedPtr<SAudioResultView> OutputAudioResultView;
	TSharedPtr<SVerticalBox> OutputSectionContainer;
}; 