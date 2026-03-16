#pragma once

#include "CoreMinimal.h"

enum class EAudioResultViewTabType : uint8;

namespace VarcoSoundPathUtils
{
	/** Replace filename-illegal characters and trim length. */
	FString SanitizeFilename(const FString& Input, int32 MaxLength = 50);

	/** Timestamp string in yyyyMMdd_HHmmss. */
	FString MakeTimestamp_yyyyMMdd_HHmmss();

	/** Output root directory (user-configured, default Content/VarcoSound). */
	FString GetOutputRootDir();

	/** Output dir for a given audio result tab type. */
	FString GetOutputDirForTab(EAudioResultViewTabType TabType);

	/** Output dir for BGM exports (OutputRoot/BGM). */
	FString GetBgmOutputDir();

	/** Cache dir for BGM downloads (Saved/VarcoSound/Cache/BGM). */
	FString GetBgmCacheDir();

	/** Ensure directory exists (CreateDirectoryTree). */
	bool EnsureDirectoryExists(const FString& Dir);
}


