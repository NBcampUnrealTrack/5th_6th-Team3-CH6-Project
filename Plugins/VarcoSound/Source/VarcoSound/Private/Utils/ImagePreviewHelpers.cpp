// Copyright Epic Games, Inc. All Rights Reserved.

#include "Utils/ImagePreviewHelpers.h"
#include "IImageWrapper.h"
#include "IImageWrapperModule.h"
#include "Modules/ModuleManager.h"
#include "Widgets/Images/SImage.h"

bool FImagePreviewHelpers::UpdateImagePreviewFromPng(
	const TArray<uint8>& PngBytes,
	TStrongObjectPtr<UTexture2D>& CapturedPreviewTexture,
	UTexture2D*& OutInputTexture,
	TSharedPtr<FSlateBrush>& ImagePreviewBrush,
	TSharedPtr<SImage>& ImagePreviewWidget,
	float ImagePreviewFixedHeight)
{
	IImageWrapperModule& ImageWrapperModule = FModuleManager::LoadModuleChecked<IImageWrapperModule>(TEXT("ImageWrapper"));
	TSharedPtr<IImageWrapper> Wrapper = ImageWrapperModule.CreateImageWrapper(EImageFormat::PNG);
	
	if (!Wrapper.IsValid())
	{
		return false;
	}
	
	if (!Wrapper->SetCompressed(PngBytes.GetData(), PngBytes.Num()))
	{
		return false;
	}
	
	TArray<uint8> RawBGRA;
	if (!Wrapper->GetRaw(ERGBFormat::BGRA, 8, RawBGRA))
	{
		return false;
	}
	
	const int32 W = Wrapper->GetWidth();
	const int32 H = Wrapper->GetHeight();
	if (W <= 0 || H <= 0)
	{
		return false;
	}
	
	UTexture2D* TempTex = UTexture2D::CreateTransient(W, H, PF_B8G8R8A8);
	if (!TempTex)
	{
		return false;
	}
	
	// Configure for high-quality immediate display
	TempTex->NeverStream = true;
	TempTex->MipGenSettings = TMGS_NoMipmaps;
	TempTex->LODGroup = TEXTUREGROUP_UI;
	TempTex->SRGB = true;
	TempTex->Filter = TF_Bilinear;
	TempTex->CompressionSettings = TC_EditorIcon;
	
	TempTex->Source.Init(W, H, 1, 1, ETextureSourceFormat::TSF_BGRA8, RawBGRA.GetData());
	TempTex->UpdateResource();
	
	// Keep texture rooted to protect from GC
	CapturedPreviewTexture.Reset();
	CapturedPreviewTexture = TStrongObjectPtr<UTexture2D>(TempTex);
	CapturedPreviewTexture->AddToRoot();
	OutInputTexture = CapturedPreviewTexture.Get();
	
	// Update preview widget
	UpdateImagePreview(OutInputTexture, ImagePreviewBrush, ImagePreviewWidget, ImagePreviewFixedHeight);
	
	return true;
}

