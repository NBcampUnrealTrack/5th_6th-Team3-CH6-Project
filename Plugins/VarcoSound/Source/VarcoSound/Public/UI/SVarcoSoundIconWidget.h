// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SLeafWidget.h"

/**
 * 사운드 팔레트 전역 아이콘 타입
 */
enum class EVarcoSoundIconType : uint8
{
	Record,
	Stop,
	Play,
	Pause
};

/**
 * 간결한 도형 기반 Slate 아이콘 위젯 (SVG 에셋 없이 일관된 렌더링 제공)
 */
class VARCOSOUND_API SVarcoSoundIcon : public SLeafWidget
{
public:
	SLATE_BEGIN_ARGS(SVarcoSoundIcon)
		: _IconType(EVarcoSoundIconType::Play)
		, _IconColor(FLinearColor::White)
		, _IconSize(FVector2D(18.0f, 18.0f))
	{}
		SLATE_ATTRIBUTE(EVarcoSoundIconType, IconType)
		SLATE_ATTRIBUTE(FLinearColor, IconColor)
		SLATE_ATTRIBUTE(FVector2D, IconSize)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	void SetIconType(EVarcoSoundIconType InType);
	void SetIconColor(const FLinearColor& InColor);
	void SetIconSize(const FVector2D& InSize);

	void SetIconTypeAttribute(TAttribute<EVarcoSoundIconType> InType);
	void SetIconColorAttribute(TAttribute<FLinearColor> InColor);
	void SetIconSizeAttribute(TAttribute<FVector2D> InSize);

	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;

	virtual FVector2D ComputeDesiredSize(float LayoutScaleMultiplier) const override;

private:
	FPaintGeometry MakeCenteredGeometry(const FGeometry& AllottedGeometry, const FVector2D& IconSize) const;
	void DrawRecordIcon(FSlateWindowElementList& OutDrawElements, int32 LayerId, const FGeometry& AllottedGeometry, const FVector2D& IconSize, const FLinearColor& IconColor) const;
	void DrawStopIcon(FSlateWindowElementList& OutDrawElements, int32 LayerId, const FGeometry& AllottedGeometry, const FVector2D& IconSize, const FLinearColor& IconColor) const;
	void DrawPlayIcon(FSlateWindowElementList& OutDrawElements, int32 LayerId, const FGeometry& AllottedGeometry, const FVector2D& IconSize, const FLinearColor& IconColor) const;
	void DrawPauseIcon(FSlateWindowElementList& OutDrawElements, int32 LayerId, const FGeometry& AllottedGeometry, const FVector2D& IconSize, const FLinearColor& IconColor) const;

	TAttribute<EVarcoSoundIconType> IconTypeAttribute;
	TAttribute<FLinearColor> IconColorAttribute;
	TAttribute<FVector2D> IconSizeAttribute;
};


