// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/SVarcoSoundIconWidget.h"

#include "Rendering/DrawElements.h"
#include "Styling/SlateColor.h"
#include "Styling/AppStyle.h"
#include "Styling/SlateBrush.h"
#include "Framework/Application/SlateApplication.h"
#include "Rendering/SlateRenderer.h"
#include "Rendering/RenderingCommon.h"

void SVarcoSoundIcon::Construct(const FArguments& InArgs)
{
	IconTypeAttribute = InArgs._IconType;
	IconColorAttribute = InArgs._IconColor;
	IconSizeAttribute = InArgs._IconSize;
}

void SVarcoSoundIcon::SetIconType(EVarcoSoundIconType InType)
{
	IconTypeAttribute = InType;
	Invalidate(EInvalidateWidget::Paint);
}

void SVarcoSoundIcon::SetIconColor(const FLinearColor& InColor)
{
	IconColorAttribute = InColor;
	Invalidate(EInvalidateWidget::Paint);
}

void SVarcoSoundIcon::SetIconSize(const FVector2D& InSize)
{
	IconSizeAttribute = InSize;
	Invalidate(EInvalidateWidget::Layout);
}

void SVarcoSoundIcon::SetIconTypeAttribute(TAttribute<EVarcoSoundIconType> InType)
{
	IconTypeAttribute = MoveTemp(InType);
	Invalidate(EInvalidateWidget::Paint);
}

void SVarcoSoundIcon::SetIconColorAttribute(TAttribute<FLinearColor> InColor)
{
	IconColorAttribute = MoveTemp(InColor);
	Invalidate(EInvalidateWidget::Paint);
}

void SVarcoSoundIcon::SetIconSizeAttribute(TAttribute<FVector2D> InSize)
{
	IconSizeAttribute = MoveTemp(InSize);
	Invalidate(EInvalidateWidget::Layout);
}

FVector2D SVarcoSoundIcon::ComputeDesiredSize(float LayoutScaleMultiplier) const
{
	return IconSizeAttribute.Get(FVector2D(18.0f, 18.0f));
}

FPaintGeometry SVarcoSoundIcon::MakeCenteredGeometry(const FGeometry& AllottedGeometry, const FVector2D& IconSize) const
{
	const FVector2D LocalSize = AllottedGeometry.GetLocalSize();
	const FVector2D Offset = (LocalSize - IconSize) * 0.5f;
	return AllottedGeometry.ToPaintGeometry(IconSize, FSlateLayoutTransform(Offset));
}

int32 SVarcoSoundIcon::OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
	FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	const FVector2D IconSize = IconSizeAttribute.Get(FVector2D(18.0f, 18.0f));
	const FLinearColor IconColor = IconColorAttribute.Get(FLinearColor::White) * InWidgetStyle.GetColorAndOpacityTint();
	const EVarcoSoundIconType IconType = IconTypeAttribute.Get(EVarcoSoundIconType::Play);

	switch (IconType)
	{
	case EVarcoSoundIconType::Record:
		DrawRecordIcon(OutDrawElements, LayerId, AllottedGeometry, IconSize, IconColor);
		break;
	case EVarcoSoundIconType::Stop:
		DrawStopIcon(OutDrawElements, LayerId, AllottedGeometry, IconSize, IconColor);
		break;
	case EVarcoSoundIconType::Play:
		DrawPlayIcon(OutDrawElements, LayerId, AllottedGeometry, IconSize, IconColor);
		break;
	case EVarcoSoundIconType::Pause:
		DrawPauseIcon(OutDrawElements, LayerId, AllottedGeometry, IconSize, IconColor);
		break;
	default:
		break;
	}

	return LayerId + 1;
}

void SVarcoSoundIcon::DrawRecordIcon(FSlateWindowElementList& OutDrawElements, int32 LayerId, const FGeometry& AllottedGeometry,
	const FVector2D& IconSize, const FLinearColor& IconColor) const
{
	if (!FSlateApplication::IsInitialized())
	{
		return;
	}

	FSlateRenderer* Renderer = FSlateApplication::Get().GetRenderer();
	const FSlateBrush* SolidBrush = FAppStyle::Get().GetBrush("WhiteBrush");
	if (!Renderer || !SolidBrush)
	{
		return;
	}

	const FSlateResourceHandle ResourceHandle = Renderer->GetResourceHandle(*SolidBrush);
	const FSlateRenderTransform& RenderTransform = AllottedGeometry.GetAccumulatedRenderTransform();
	const FColor VertexColor = IconColor.ToFColor(true);

	const FVector2D LocalSize = AllottedGeometry.GetLocalSize();
	const FVector2D Offset = (LocalSize - IconSize) * 0.5f;
	const FVector2D Center = Offset + (IconSize * 0.5f);
	const float Radius = 0.5f * IconSize.GetMin();
	const int32 NumSegments = FMath::Clamp(FMath::RoundToInt(Radius * 4.0f), 24, 64);

	TArray<FSlateVertex> Vertices;
	TArray<SlateIndex> Indices;
	Vertices.Reserve(NumSegments + 1);
	Indices.Reserve(NumSegments * 3);

	auto MakeVertex = [&RenderTransform, VertexColor](const FVector2D& InLocalPosition)
	{
		return FSlateVertex::Make<ESlateVertexRounding::Disabled>(RenderTransform, FVector2f(InLocalPosition), FVector2f::ZeroVector, VertexColor);
	};

	const SlateIndex CenterIndex = static_cast<SlateIndex>(Vertices.Add(MakeVertex(Center)));

	for (int32 SegmentIndex = 0; SegmentIndex < NumSegments; ++SegmentIndex)
	{
		const float Angle = (2.0f * PI) * (static_cast<float>(SegmentIndex) / static_cast<float>(NumSegments));
		const FVector2D PerimeterPoint = Center + FVector2D(FMath::Cos(Angle) * Radius, FMath::Sin(Angle) * Radius);
		Vertices.Add(MakeVertex(PerimeterPoint));
	}

	for (int32 SegmentIndex = 0; SegmentIndex < NumSegments; ++SegmentIndex)
	{
		const SlateIndex Current = static_cast<SlateIndex>(CenterIndex + SegmentIndex + 1);
		const SlateIndex Next = static_cast<SlateIndex>(CenterIndex + ((SegmentIndex + 1) % NumSegments) + 1);
		Indices.Add(CenterIndex);
		Indices.Add(Current);
		Indices.Add(Next);
	}

	FSlateDrawElement::MakeCustomVerts(
		OutDrawElements,
		LayerId,
		ResourceHandle,
		Vertices,
		Indices,
		nullptr,
		0,
		1,
		ESlateDrawEffect::None);
}

void SVarcoSoundIcon::DrawStopIcon(FSlateWindowElementList& OutDrawElements, int32 LayerId, const FGeometry& AllottedGeometry,
	const FVector2D& IconSize, const FLinearColor& IconColor) const
{
	const FSlateBrush* WhiteBrush = FAppStyle::GetBrush("WhiteBrush");
	const FPaintGeometry Geometry = MakeCenteredGeometry(AllottedGeometry, IconSize);
	FSlateDrawElement::MakeBox(
		OutDrawElements,
		LayerId,
		Geometry,
		WhiteBrush,
		ESlateDrawEffect::None,
		IconColor
	);
}

void SVarcoSoundIcon::DrawPauseIcon(FSlateWindowElementList& OutDrawElements, int32 LayerId, const FGeometry& AllottedGeometry,
	const FVector2D& IconSize, const FLinearColor& IconColor) const
{
	if (!FSlateApplication::IsInitialized())
	{
		return;
	}

	FSlateRenderer* Renderer = FSlateApplication::Get().GetRenderer();
	const FSlateBrush* SolidBrush = FAppStyle::Get().GetBrush("WhiteBrush");
	if (!Renderer || !SolidBrush)
	{
		return;
	}

	const FSlateResourceHandle ResourceHandle = Renderer->GetResourceHandle(*SolidBrush);
	const FSlateRenderTransform& RenderTransform = AllottedGeometry.GetAccumulatedRenderTransform();
	const FColor VertexColor = IconColor.ToFColor(true);

	const FVector2D LocalSize = AllottedGeometry.GetLocalSize();
	const FVector2D Offset = (LocalSize - IconSize) * 0.5f;
	const float BarWidth = IconSize.X * 0.28f;
	const float Gap = IconSize.X - (BarWidth * 2.0f);
	const FVector2D BarSize(BarWidth, IconSize.Y);

	auto MakeVertex = [&RenderTransform, VertexColor](const FVector2D& InLocalPosition)
	{
		return FSlateVertex::Make<ESlateVertexRounding::Disabled>(RenderTransform, FVector2f(InLocalPosition), FVector2f::ZeroVector, VertexColor);
	};

	auto AddQuad = [&MakeVertex](TArray<FSlateVertex>& Vertices, TArray<SlateIndex>& Indices, const FVector2D& TopLeft, const FVector2D& Size)
	{
		const SlateIndex BaseIndex = static_cast<SlateIndex>(Vertices.Num());
		Vertices.Add(MakeVertex(TopLeft));
		Vertices.Add(MakeVertex(TopLeft + FVector2D(Size.X, 0.0f)));
		Vertices.Add(MakeVertex(TopLeft + Size));
		Vertices.Add(MakeVertex(TopLeft + FVector2D(0.0f, Size.Y)));

		Indices.Add(BaseIndex);
		Indices.Add(BaseIndex + 1);
		Indices.Add(BaseIndex + 2);
		Indices.Add(BaseIndex);
		Indices.Add(BaseIndex + 2);
		Indices.Add(BaseIndex + 3);
	};

	TArray<FSlateVertex> Vertices;
	TArray<SlateIndex> Indices;
	Vertices.Reserve(8);
	Indices.Reserve(12);

	const FVector2D LeftTop(FMath::RoundToFloat(Offset.X), FMath::RoundToFloat(Offset.Y));
	const FVector2D RightTop(FMath::RoundToFloat(Offset.X + BarWidth + Gap), FMath::RoundToFloat(Offset.Y));

	AddQuad(Vertices, Indices, LeftTop, BarSize);
	AddQuad(Vertices, Indices, RightTop, BarSize);

	FSlateDrawElement::MakeCustomVerts(
		OutDrawElements,
		LayerId,
		ResourceHandle,
		Vertices,
		Indices,
		nullptr,
		0,
		1,
		ESlateDrawEffect::None);
}

void SVarcoSoundIcon::DrawPlayIcon(FSlateWindowElementList& OutDrawElements, int32 LayerId, const FGeometry& AllottedGeometry,
	const FVector2D& IconSize, const FLinearColor& IconColor) const
{
	if (!FSlateApplication::IsInitialized())
	{
		return;
	}

	FSlateRenderer* Renderer = FSlateApplication::Get().GetRenderer();
	const FSlateBrush* SolidBrush = FAppStyle::Get().GetBrush("WhiteBrush");
	if (!Renderer || !SolidBrush)
	{
		return;
	}

	const FSlateResourceHandle ResourceHandle = Renderer->GetResourceHandle(*SolidBrush);
	const FSlateRenderTransform& RenderTransform = AllottedGeometry.GetAccumulatedRenderTransform();
	const FColor VertexColor = IconColor.ToFColor(true);

	const FVector2D LocalSize = AllottedGeometry.GetLocalSize();
	const FVector2D Offset = (LocalSize - IconSize) * 0.5f;

	const auto ScaleNormalized = [&](const FVector2D& Normalized)->FVector2D
	{
		const FVector2D CenterNorm(0.5f, 0.5f);
		FVector2D Scaled = (Normalized - CenterNorm) * 1.5f + CenterNorm;
		Scaled.X = FMath::Clamp(Scaled.X, 0.0f, 1.0f);
		Scaled.Y = FMath::Clamp(Scaled.Y, 0.0f, 1.0f);
		return Offset + FVector2D(Scaled.X * IconSize.X, Scaled.Y * IconSize.Y);
	};

	const FVector2D P0 = ScaleNormalized(FVector2D(0.26f, 0.2f));
	const FVector2D P1 = ScaleNormalized(FVector2D(0.26f, 0.8f));
	const FVector2D P2 = ScaleNormalized(FVector2D(0.86f, 0.5f));

	TArray<FSlateVertex> Vertices;
	TArray<SlateIndex> Indices;
	Vertices.Reserve(3);
	Indices.Reserve(3);

	auto MakeVertex = [&RenderTransform, VertexColor](const FVector2D& InLocalPosition)
	{
		return FSlateVertex::Make<ESlateVertexRounding::Disabled>(RenderTransform, FVector2f(InLocalPosition), FVector2f::ZeroVector, VertexColor);
	};

	const SlateIndex BaseIndex = static_cast<SlateIndex>(Vertices.Add(MakeVertex(P0)));
	Vertices.Add(MakeVertex(P1));
	Vertices.Add(MakeVertex(P2));

	Indices.Add(BaseIndex);
	Indices.Add(static_cast<SlateIndex>(BaseIndex + 1));
	Indices.Add(static_cast<SlateIndex>(BaseIndex + 2));

	FSlateDrawElement::MakeCustomVerts(
		OutDrawElements,
		LayerId,
		ResourceHandle,
		Vertices,
		Indices,
		nullptr,
		0,
		1,
		ESlateDrawEffect::None);
}


