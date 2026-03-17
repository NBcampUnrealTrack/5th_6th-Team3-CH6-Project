// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/SWaveformDisplay.h"
#include "Rendering/DrawElements.h"
#include "Framework/Application/SlateApplication.h"
#include "InputCoreTypes.h"

void SWaveformDisplay::Construct(const FArguments& InArgs)
{
    WaveformData = InArgs._WaveformData;
    PlaybackPercentage = InArgs._PlaybackPercentage;
    IsStereo = InArgs._IsStereo;
    AmplitudeScale = InArgs._AmplitudeScale;
    WaveformColorAttr = InArgs._WaveformColor;
}

SWaveformDisplay::~SWaveformDisplay()
{
    // 바인딩 해제 안전 처리
    OnSeekRequested.Unbind();
    OnTogglePlayPauseRequested.Unbind();
}

int32 SWaveformDisplay::OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
    const TArray<float>& CurrentWaveform = WaveformData.Get();
    const bool bIsStereo = IsStereo.Get();

    const FVector2D LocalSize = AllottedGeometry.GetLocalSize();
    if (LocalSize.X <= 0.0f || LocalSize.Y <= 0.0f)
    {
        return LayerId;
    }

    const FLinearColor WaveColor = WaveformColorAttr.Get(FLinearColor(0.8f, 0.8f, 0.8f, 1.0f));
    const FLinearColor BaselineColor = WaveColor;

    if (CurrentWaveform.Num() > 0)
    {
        const int32 Columns = FMath::Max(1, (int32)FMath::RoundToInt(LocalSize.X));
        const float VisualScale = FMath::Clamp(AmplitudeScale.Get(0.8f), 0.0f, 1.0f); // 사용자 조절값

        if (bIsStereo && CurrentWaveform.Num() >= 2)
        {
            const float ChannelHeight = LocalSize.Y / 2.0f;
            const float HalfChannel = ChannelHeight * 0.5f;
            const float LeftCenterY = HalfChannel;
            const float RightCenterY = ChannelHeight + HalfChannel;

            // 요청: 최대 바 높이 = 반 높이 * VisualScale
            const float MaxBarHeight = HalfChannel * VisualScale;

            const int32 NumFrames = CurrentWaveform.Num() / 2;
            if (NumFrames > 0)
            {
                float MaxAbsLeft = 0.0f, MaxAbsRight = 0.0f;
                for (int32 i = 0; i < NumFrames; ++i)
                {
                    MaxAbsLeft = FMath::Max(MaxAbsLeft, FMath::Abs(CurrentWaveform[i * 2]));
                    MaxAbsRight = FMath::Max(MaxAbsRight, FMath::Abs(CurrentWaveform[i * 2 + 1]));
                }
                if (MaxAbsLeft < 1e-6f) MaxAbsLeft = 1e-6f;
                if (MaxAbsRight < 1e-6f) MaxAbsRight = 1e-6f;

                // Left channel
                for (int32 x = 0; x < Columns; ++x)
                {
                    int32 StartFrame = FMath::FloorToInt(((float)x / (float)Columns) * (float)NumFrames);
                    int32 EndFrame = FMath::CeilToInt(((float)(x + 1) / (float)Columns) * (float)NumFrames);
                    StartFrame = FMath::Clamp(StartFrame, 0, NumFrames);
                    EndFrame = FMath::Clamp(EndFrame, 0, NumFrames);
                    if (EndFrame <= StartFrame) continue;

                    float RmsSum = 0.0f, Peak = 0.0f; int32 Count = 0;
                    for (int32 i = StartFrame; i < EndFrame; ++i)
                    {
                        const float S = FMath::Abs(CurrentWaveform[i * 2]);
                        RmsSum += S * S;
                        Peak = FMath::Max(Peak, S);
                        ++Count;
                    }
                    if (Count == 0) continue;

                    const float Rms = FMath::Sqrt(RmsSum / (float)Count);
                    const float FinalAmp = (Rms / MaxAbsLeft) * 0.7f + (Peak / MaxAbsLeft) * 0.3f;
                    const float Height = FinalAmp * MaxBarHeight;
                    if (Height <= 0.0f) continue;

                    const float FX = (float)x;
                    const float TopY = LeftCenterY - Height;
                    const float BottomY = LeftCenterY + Height;

                    TArray<FVector2D> LinePoints;
                    LinePoints.Add(FVector2D(FX, TopY));
                    LinePoints.Add(FVector2D(FX, BottomY));

                    FSlateDrawElement::MakeLines(
                        OutDrawElements,
                        LayerId,
                        AllottedGeometry.ToPaintGeometry(),
                        LinePoints,
                        ESlateDrawEffect::None,
                        WaveColor,
                        true,
                        1.0f
                    );
                }

                // Right channel
                for (int32 x = 0; x < Columns; ++x)
                {
                    int32 StartFrame = FMath::FloorToInt(((float)x / (float)Columns) * (float)NumFrames);
                    int32 EndFrame = FMath::CeilToInt(((float)(x + 1) / (float)Columns) * (float)NumFrames);
                    StartFrame = FMath::Clamp(StartFrame, 0, NumFrames);
                    EndFrame = FMath::Clamp(EndFrame, 0, NumFrames);
                    if (EndFrame <= StartFrame) continue;

                    float RmsSum = 0.0f, Peak = 0.0f; int32 Count = 0;
                    for (int32 i = StartFrame; i < EndFrame; ++i)
                    {
                        const float S = FMath::Abs(CurrentWaveform[i * 2 + 1]);
                        RmsSum += S * S;
                        Peak = FMath::Max(Peak, S);
                        ++Count;
                    }
                    if (Count == 0) continue;

                    const float Rms = FMath::Sqrt(RmsSum / (float)Count);
                    const float FinalAmp = (Rms / MaxAbsRight) * 0.7f + (Peak / MaxAbsRight) * 0.3f;
                    const float Height = FinalAmp * MaxBarHeight;
                    if (Height <= 0.0f) continue;

                    const float FX = (float)x;
                    const float TopY = RightCenterY - Height;
                    const float BottomY = RightCenterY + Height;

                    TArray<FVector2D> LinePoints;
                    LinePoints.Add(FVector2D(FX, TopY));
                    LinePoints.Add(FVector2D(FX, BottomY));

                    FSlateDrawElement::MakeLines(
                        OutDrawElements,
                        LayerId,
                        AllottedGeometry.ToPaintGeometry(),
                        LinePoints,
                        ESlateDrawEffect::None,
                        WaveColor,
                        true,
                        1.0f
                    );
                }

                // Baselines (same color as waveform)
                {
                    TArray<FVector2D> BaselinePts;
                    BaselinePts.Add(FVector2D(0.0f, LeftCenterY));
                    BaselinePts.Add(FVector2D(LocalSize.X, LeftCenterY));
                    FSlateDrawElement::MakeLines(
                        OutDrawElements,
                        LayerId + 1,
                        AllottedGeometry.ToPaintGeometry(),
                        BaselinePts,
                        ESlateDrawEffect::None,
                        BaselineColor,
                        true,
                        1.0f
                    );
                }
                {
                    TArray<FVector2D> BaselinePts;
                    BaselinePts.Add(FVector2D(0.0f, RightCenterY));
                    BaselinePts.Add(FVector2D(LocalSize.X, RightCenterY));
                    FSlateDrawElement::MakeLines(
                        OutDrawElements,
                        LayerId + 1,
                        AllottedGeometry.ToPaintGeometry(),
                        BaselinePts,
                        ESlateDrawEffect::None,
                        BaselineColor,
                        true,
                        1.0f
                    );
                }
            }
        }
        else
        {
            const int32 NumSamples = CurrentWaveform.Num();
            if (NumSamples > 0)
            {
                float MaxAbs = 0.0f;
                for (int32 i = 0; i < NumSamples; ++i)
                {
                    MaxAbs = FMath::Max(MaxAbs, FMath::Abs(CurrentWaveform[i]));
                }
                if (MaxAbs < 1e-6f) MaxAbs = 1e-6f;

                const float YCenter = LocalSize.Y * 0.5f;
                const float MaxBarHeight = (LocalSize.Y * 0.5f) * VisualScale;

                for (int32 x = 0; x < Columns; ++x)
                {
                    int32 StartIndex = FMath::FloorToInt(((float)x / (float)Columns) * (float)NumSamples);
                    int32 EndIndex = FMath::CeilToInt(((float)(x + 1) / (float)Columns) * (float)NumSamples);
                    StartIndex = FMath::Clamp(StartIndex, 0, NumSamples);
                    EndIndex = FMath::Clamp(EndIndex, 0, NumSamples);
                    if (EndIndex <= StartIndex) continue;

                    float RmsSum = 0.0f, Peak = 0.0f; int32 Count = 0;
                    for (int32 i = StartIndex; i < EndIndex; ++i)
                    {
                        const float S = FMath::Abs(CurrentWaveform[i]);
                        RmsSum += S * S;
                        Peak = FMath::Max(Peak, S);
                        ++Count;
                    }
                    if (Count == 0) continue;

                    const float Rms = FMath::Sqrt(RmsSum / (float)Count);
                    const float FinalAmp = (Rms / MaxAbs) * 0.7f + (Peak / MaxAbs) * 0.3f;
                    const float Height = FinalAmp * MaxBarHeight;
                    if (Height <= 0.0f) continue;

                    const float FX = (float)x;
                    const float TopY = YCenter - Height;
                    const float BottomY = YCenter + Height;

                    TArray<FVector2D> LinePoints;
                    LinePoints.Add(FVector2D(FX, TopY));
                    LinePoints.Add(FVector2D(FX, BottomY));

                    FSlateDrawElement::MakeLines(
                        OutDrawElements,
                        LayerId,
                        AllottedGeometry.ToPaintGeometry(),
                        LinePoints,
                        ESlateDrawEffect::None,
                        WaveColor,
                        true,
                        1.0f
                    );
                }

                // Baseline (same color)
                TArray<FVector2D> BaselinePts;
                BaselinePts.Add(FVector2D(0.0f, YCenter));
                BaselinePts.Add(FVector2D(LocalSize.X, YCenter));
                FSlateDrawElement::MakeLines(
                    OutDrawElements,
                    LayerId + 1,
                    AllottedGeometry.ToPaintGeometry(),
                    BaselinePts,
                    ESlateDrawEffect::None,
                    BaselineColor,
                    true,
                    1.0f
                );
            }
        }
    }

    // Cursor
    const float CurrentPlaybackPercentage = PlaybackPercentage.Get();
    if (CurrentPlaybackPercentage > 0.0f)
    {
        const float CursorX = LocalSize.X * CurrentPlaybackPercentage;

        TArray<FVector2D> CursorLinePoints;
        CursorLinePoints.Add(FVector2D(CursorX, 0.0f));
        CursorLinePoints.Add(FVector2D(CursorX, LocalSize.Y));

        FSlateDrawElement::MakeLines(
            OutDrawElements,
            LayerId + 2,
            AllottedGeometry.ToPaintGeometry(),
            CursorLinePoints,
            ESlateDrawEffect::None,
            FLinearColor::Red,
            true,
            2.0f
        );
    }

    return LayerId + 3;
}

FVector2D SWaveformDisplay::ComputeDesiredSize(float) const
{
    return FVector2D(100.0f, 50.0f); // Provide a default desired size
}

void SWaveformDisplay::SetWaveformData(const TArray<float>& InWaveformData)
{
    WaveformData.Set(InWaveformData);
    IsStereo.Set(false);
    Invalidate(EInvalidateWidget::Paint);
}

void SWaveformDisplay::SetStereoWaveformData(const TArray<float>& InWaveformData)
{
    WaveformData.Set(InWaveformData);
    IsStereo.Set(true);
    Invalidate(EInvalidateWidget::Paint);
}

void SWaveformDisplay::SetPlaybackPercentage(float InPercentage)
{
    PlaybackPercentage.Set(InPercentage);
    Invalidate(EInvalidateWidget::Paint);
}

void SWaveformDisplay::SetAmplitudeScale(float InScale)
{
    AmplitudeScale.Set(FMath::Clamp(InScale, 0.0f, 1.0f));
    Invalidate(EInvalidateWidget::Paint);
}

void SWaveformDisplay::SetWaveformColor(const FLinearColor& InColor)
{
    WaveformColorAttr = InColor;
    Invalidate(EInvalidateWidget::Paint);
}

FReply SWaveformDisplay::OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
    if (MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
    {
        bDragging = true;
        FSlateApplication::Get().SetKeyboardFocus(SharedThis(this));

        const FVector2D LocalPos = MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition());
        const FVector2D Size = MyGeometry.GetLocalSize();
        const float Percent = Size.X > 0.0f ? FMath::Clamp(LocalPos.X / Size.X, 0.0f, 1.0f) : 0.0f;
        if (OnSeekRequested.IsBound()) { OnSeekRequested.Execute(Percent); }
        LastSeekSentPercent = Percent;
        return FReply::Handled().CaptureMouse(SharedThis(this));
    }
    return FReply::Unhandled();
}

FReply SWaveformDisplay::OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
    if (MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && bDragging)
    {
        bDragging = false;
        return FReply::Handled().ReleaseMouseCapture();
    }
    return FReply::Unhandled();
}

FReply SWaveformDisplay::OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
    if (bDragging)
    {
        const FVector2D LocalPos = MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition());
        const FVector2D Size = MyGeometry.GetLocalSize();
        const float Percent = Size.X > 0.0f ? FMath::Clamp(LocalPos.X / Size.X, 0.0f, 1.0f) : 0.0f;
        // 과도한 중복 호출 방지(픽셀 변화 없을 때 억제)
        if (!FMath::IsNearlyEqual(Percent, LastSeekSentPercent, 1e-3f))
        {
            if (OnSeekRequested.IsBound()) { OnSeekRequested.Execute(Percent); }
            LastSeekSentPercent = Percent;
        }
        return FReply::Handled();
    }
    return FReply::Unhandled();
}

FReply SWaveformDisplay::OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent)
{
    if (InKeyEvent.GetKey() == EKeys::SpaceBar)
    {
        if (OnTogglePlayPauseRequested.IsBound()) { OnTogglePlayPauseRequested.Execute(); }
        // 스페이스 처리는 이 위젯에서 소비하여 다른 버튼으로 전파/재클릭되지 않도록 함
        return FReply::Handled();
    }
    return FReply::Unhandled();
} 