// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#if WITH_EDITOR

// Viewport capture options
struct FViewportCaptureOptions
{
	int32 Width = 1024;
	int32 Height = 1024;
	bool bHideGizmos = true;
	bool bDisableAA = true;
};

// Actor capture options
struct FActorCaptureOptions
{
	int32 Width = 1920;
	int32 Height = 1920;
	bool bTransparentBG = true;
	bool bCropToBounds = true;
	float FOV = 35.f;
};

// Capture active editor viewport to PNG bytes
VARCOSOUND_API bool CaptureEditorViewportToPng(const FViewportCaptureOptions& Opts, TArray<uint8>& OutPng);

// Capture selected actors only with transparent background to PNG bytes
VARCOSOUND_API bool CaptureSelectedActorsToPng(const FActorCaptureOptions& Opts, TArray<uint8>& OutPng);

// Convert PNG bytes to Base64 string
VARCOSOUND_API bool PngBytesToBase64(const TArray<uint8>& Png, FString& OutBase64);

#endif // WITH_EDITOR


