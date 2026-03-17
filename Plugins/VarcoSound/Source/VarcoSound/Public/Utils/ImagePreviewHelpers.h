// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/Texture2D.h"
#include "Styling/SlateBrush.h"
#include "UObject/StrongObjectPtr.h"
#include "Widgets/Images/SImage.h"
#include "Types/SlateStructs.h"

/**
 * Image preview helper utilities for VarcoSound plugin
 * Provides centralized image preview and capture functionality
 */
namespace FImagePreviewHelpers
{
	/**
	 * Update image preview widget with texture
	 * @param InTexture The texture to display (can be nullptr to clear)
	 * @param ImagePreviewBrush The Slate brush to update
	 * @param ImagePreviewWidget The image widget to update
	 * @param ImagePreviewFixedHeight Fixed height for preview
	 */
	inline void UpdateImagePreview(
		UTexture2D* InTexture,
		TSharedPtr<FSlateBrush>& ImagePreviewBrush,
		TSharedPtr<SImage>& ImagePreviewWidget,
		float ImagePreviewFixedHeight)
	{
		if (!ImagePreviewBrush.IsValid())
		{
			ImagePreviewBrush = MakeShared<FSlateBrush>();
		}

		if (InTexture && IsValid(InTexture))
		{
			ImagePreviewBrush->SetResourceObject(InTexture);
			const int32 SrcW = InTexture->GetSizeX();
			const int32 SrcH = InTexture->GetSizeY();
			if (SrcW > 0 && SrcH > 0)
			{
				const float TargetH = ImagePreviewFixedHeight;
				const float TargetW = TargetH * (static_cast<float>(SrcW) / static_cast<float>(SrcH));
				ImagePreviewBrush->ImageSize = FVector2D(TargetW, TargetH);
			}
			else
			{
				ImagePreviewBrush->ImageSize = FVector2D(ImagePreviewFixedHeight, ImagePreviewFixedHeight);
			}
			if (ImagePreviewWidget.IsValid())
			{
				ImagePreviewWidget->SetImage(ImagePreviewBrush.Get());
				ImagePreviewWidget->Invalidate(EInvalidateWidget::Layout);
			}
		}
		else
		{
			if (ImagePreviewWidget.IsValid())
			{
				ImagePreviewWidget->SetImage(nullptr);
				ImagePreviewWidget->Invalidate(EInvalidateWidget::Layout);
			}
		}
	}

	/**
	 * Update image preview from PNG bytes
	 * @param PngBytes Raw PNG data
	 * @param CapturedPreviewTexture Strong object pointer to hold the created texture
	 * @param OutInputTexture Output texture pointer
	 * @param ImagePreviewBrush The Slate brush to update
	 * @param ImagePreviewWidget The image widget to update
	 * @param ImagePreviewFixedHeight Fixed height for preview
	 * @return true if successful
	 */
	VARCOSOUND_API bool UpdateImagePreviewFromPng(
		const TArray<uint8>& PngBytes,
		TStrongObjectPtr<UTexture2D>& CapturedPreviewTexture,
		UTexture2D*& OutInputTexture,
		TSharedPtr<FSlateBrush>& ImagePreviewBrush,
		TSharedPtr<SImage>& ImagePreviewWidget,
		float ImagePreviewFixedHeight);

	/**
	 * Calculate preview width based on aspect ratio
	 * @param ImagePreviewBrush The brush containing size information
	 * @param ImagePreviewFixedHeight Fixed height for preview
	 * @return Calculated width
	 */
	inline FOptionalSize GetImagePreviewWidth(
		const TSharedPtr<FSlateBrush>& ImagePreviewBrush,
		float ImagePreviewFixedHeight)
	{
		if (ImagePreviewBrush.IsValid() && ImagePreviewBrush->ImageSize.Y > 0.0f)
		{
			const float Aspect = ImagePreviewBrush->ImageSize.X / FMath::Max(1.0f, ImagePreviewBrush->ImageSize.Y);
			return FOptionalSize(ImagePreviewFixedHeight * Aspect);
		}
		return FOptionalSize(ImagePreviewFixedHeight);
	}

	/**
	 * Get preview height
	 * @param ImagePreviewFixedHeight Fixed height for preview
	 * @return Height value
	 */
	inline FOptionalSize GetImagePreviewHeight(float ImagePreviewFixedHeight)
	{
		return FOptionalSize(ImagePreviewFixedHeight);
	}

	/**
	 * Cleanup captured preview texture
	 * @param CapturedPreviewTexture Strong object pointer holding the texture
	 */
	inline void CleanupCapturedTexture(TStrongObjectPtr<UTexture2D>& CapturedPreviewTexture)
	{
		if (CapturedPreviewTexture.IsValid())
		{
			if (UTexture2D* Tex = CapturedPreviewTexture.Get())
			{
				if (Tex->IsRooted())
				{
					Tex->RemoveFromRoot();
				}
			}
			CapturedPreviewTexture.Reset();
		}
	}
}

