#pragma once

#include "CoreMinimal.h"

// Centralized API endpoint definitions=
namespace SPApi
{
	// Base URL (Environment/Deployment-specific switching only changes this constant)
	inline constexpr const TCHAR* BaseUrl = TEXT("https://openapi.ai.nc.com/sound/varco/v1/api");

	// Internal join utility: BaseUrl + Path("/foo/") => "https://.../foo/"
	inline FString Join(const TCHAR* Path)
	{
		FString Base(BaseUrl);
		const bool bBaseEndsWithSlash = Base.EndsWith(TEXT("/"));
		const bool bPathStartsWithSlash = FString(Path).StartsWith(TEXT("/"));
		if (bBaseEndsWithSlash && bPathStartsWithSlash)
		{
			Base.LeftChopInline(1, EAllowShrinking::No);
		}
		else if (!bBaseEndsWithSlash && !bPathStartsWithSlash)
		{
			Base.Append(TEXT("/"));
		}
		return Base + Path;
	}

	// Each API endpoint (change paths here only if needed)
	inline FString Gen() { return Join(TEXT("/generate")); }
	inline FString Looping() { return Join(TEXT("/looping")); }
	inline FString Variation() { return Join(TEXT("/variation")); }
	inline FString Mono2Stereo() { return Join(TEXT("/mono2stereo")); }
	inline FString Conversion() { return Join(TEXT("/conversion")); }
	inline FString PromptBooster() { return Join(TEXT("/enhance-text-prompt")); }
	inline FString Image2Sfx() { return Join(TEXT("/image2prompt")); }

	// Background music async polling endpoints
	inline FString BgmGenerateMusic() { return Join(TEXT("/generate-music")); }
	inline FString BgmGeneratedMusicStatus(const FString& TaskId) { return Join(*FString::Printf(TEXT("/generated-music/%s"), *TaskId)); }
	inline FString BgmMusicDownload(const FString& MusicId) { return Join(*FString::Printf(TEXT("/music/%s/download"), *MusicId)); }
}


