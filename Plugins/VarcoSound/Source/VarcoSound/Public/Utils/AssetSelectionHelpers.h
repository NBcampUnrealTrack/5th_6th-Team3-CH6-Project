// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Sound/SoundWave.h"
#include "AssetRegistry/AssetData.h"
#include "EditorFramework/AssetImportData.h"
#include "Misc/Paths.h"

class USoundWave;

/**
 * Asset selection helper utilities for VarcoSound plugin
 * Provides centralized asset path retrieval functionality
 */
namespace FAssetSelectionHelpers
{
	/**
	 * Get asset path from SoundWave pointer
	 * @param SoundWave The sound wave to get path from
	 * @return Asset path string, or empty if invalid
	 */
	inline FString GetSoundWaveAssetPath(USoundWave* SoundWave)
	{
		if (SoundWave && IsValid(SoundWave))
		{
			return SoundWave->GetPathName();
		}
		return FString();
	}

	/**
	 * Get source file path from SoundWave's AssetImportData
	 * @param SoundWave The sound wave to get source file from
	 * @return Source file path, or empty if unavailable
	 */
	inline FString GetSoundWaveSourceFilePath(USoundWave* SoundWave)
	{
		if (SoundWave && SoundWave->AssetImportData)
		{
			return SoundWave->AssetImportData->GetFirstFilename();
		}
		return FString();
	}

	/**
	 * Check if source file exists for a SoundWave
	 * @param SoundWave The sound wave to check
	 * @return true if source file exists on disk
	 */
	inline bool HasValidSourceFile(USoundWave* SoundWave)
	{
		FString SourcePath = GetSoundWaveSourceFilePath(SoundWave);
		return !SourcePath.IsEmpty() && FPaths::FileExists(SourcePath);
	}

	/**
	 * Extract USoundWave from AssetData
	 * @param AssetData The asset data to extract from
	 * @return USoundWave pointer, or nullptr if invalid
	 */
	inline USoundWave* GetSoundWaveFromAssetData(const FAssetData& AssetData)
	{
		return Cast<USoundWave>(AssetData.GetAsset());
	}
}

