// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SLeafWidget.h"
#include "Delegates/DelegateCombinations.h"

class VARCOSOUND_API SWaveformDisplay : public SLeafWidget
{
public:
    SLATE_BEGIN_ARGS(SWaveformDisplay)
        : _WaveformData()
        , _PlaybackPercentage(0.0f)
        , _IsStereo(false)
        , _AmplitudeScale(0.7f)
        , _WaveformColor(FLinearColor(0.8f, 0.8f, 0.8f, 1.0f))
    {}
        SLATE_ATTRIBUTE(TArray<float>, WaveformData)
        SLATE_ATTRIBUTE(float, PlaybackPercentage)
        SLATE_ATTRIBUTE(bool, IsStereo)
        SLATE_ATTRIBUTE(float, AmplitudeScale) // 0..1, 반높이에 대한 비율(기본 0.7)
        SLATE_ATTRIBUTE(FLinearColor, WaveformColor)
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs);
    virtual ~SWaveformDisplay() override;

    // SWidget overrides
    virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;
    virtual FVector2D ComputeDesiredSize(float) const override;
    virtual bool SupportsKeyboardFocus() const override { return true; }
    virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
    virtual FReply OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
    virtual FReply OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
    virtual FReply OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) override;

    // Set the waveform data to display (mono)
    void SetWaveformData(const TArray<float>& InWaveformData);

    // Set the stereo waveform data to display (interleaved L/R samples)
    void SetStereoWaveformData(const TArray<float>& InWaveformData);

    // Set the playback percentage to display
    void SetPlaybackPercentage(float InPercentage);

    // Set visual amplitude scale (0..1). 0.7 means 최대 바 높이가 반높이의 70%.
    void SetAmplitudeScale(float InScale);

    void SetWaveformColor(const FLinearColor& InColor);

    // Interaction delegates
    DECLARE_DELEGATE_OneParam(FOnSeekRequested, float /*Percent*/);
    DECLARE_DELEGATE(FOnTogglePlayPauseRequested);
    void SetOnSeekRequested(FOnSeekRequested InDelegate) { OnSeekRequested = InDelegate; }
    void SetOnTogglePlayPauseRequested(FOnTogglePlayPauseRequested InDelegate) { OnTogglePlayPauseRequested = InDelegate; }

private:
    TAttribute<TArray<float>> WaveformData;
    TAttribute<float> PlaybackPercentage;
    TAttribute<bool> IsStereo;
    TAttribute<float> AmplitudeScale;
    TAttribute<FLinearColor> WaveformColorAttr;

    // Interaction state
    bool bDragging = false;
    float LastSeekSentPercent = -1.0f;
    FOnSeekRequested OnSeekRequested;
    FOnTogglePlayPauseRequested OnTogglePlayPauseRequested;
}; 