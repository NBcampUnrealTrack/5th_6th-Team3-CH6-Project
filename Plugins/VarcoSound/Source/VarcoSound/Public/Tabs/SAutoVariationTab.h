// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Input/SNumericEntryBox.h"
#include "Widgets/Input/SSlider.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Layout/SExpandableArea.h"
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

class VARCOSOUND_API SAutoVariationTab : public SCompoundWidget, public IVarcoSoundTab
{
public:
	SLATE_BEGIN_ARGS(SAutoVariationTab) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	virtual ~SAutoVariationTab();

	// IVarcoSoundTab interface
	virtual void PauseAllPlayback() override;
	virtual void SetApiKey(const FString& InApiKey) override;

private:
	// UI Event Handlers
	FReply OnCreateVariationButtonClicked();
	
	// Asset selection
	void OnAssetSelected(const FAssetData& AssetData);
	FString GetCurrentAssetPath() const;
	
	
	
	// Parameter getters
	TOptional<int32> GetNumSampleValue() const;
	void OnNumSampleValueChanged(int32 NewValue);
	
	float GetStrengthValue() const;
	void OnStrengthValueChanged(float NewValue);
	
	// API Response Handler
	void OnVariationApiResponse(const TArray<FString>& AudioBase64Array);
	
	// Helper functions
	void UpdateInputAudioPlayer();

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
	TSharedPtr<SExpandableArea> ParametersExpandableArea;
	TSharedPtr<SNumericEntryBox<int32>> NumSampleSpinBox;
	TSharedPtr<SSlider> StrengthSlider;
	TSharedPtr<STextBlock> StrengthValueText;
	
	// Parameters
	int32 NumSampleValue;
	float StrengthValue;
}; 