#pragma once

#include "CoreMinimal.h"

namespace VarcoSoundSettings
{
	/** Load the stored OpenAPI key from the editor user settings. */
	FString LoadApiKey();

	/** Persist the OpenAPI key to the editor user settings. */
	void SaveApiKey(const FString& InApiKey);

	/** Default output root directory (project Content/VarcoSound). */
	FString GetDefaultOutputRootDirectory();

	/** Load custom output root directory, falling back to default when unset. */
	FString LoadOutputRootDirectory();

	/** Persist the output root directory (absolute path). */
	void SaveOutputRootDirectory(const FString& InDirectory);

	/** Default recording output directory (project Content/record). */
	FString GetDefaultRecordingDirectory();

	/** Default generated audio output directory (project Content/GeneratedAudio). */
	FString GetDefaultGeneratedAudioDirectory();

	/** Load custom recording output directory, falling back to default when unset. */
	FString LoadRecordingDirectory();

	/** Persist the recording output directory (absolute path). */
	void SaveRecordingDirectory(const FString& InDirectory);

	/** Load custom generated audio directory, falling back to default when unset. */
	FString LoadGeneratedAudioDirectory();

	/** Persist the generated audio directory (absolute path). */
	void SaveGeneratedAudioDirectory(const FString& InDirectory);
	
	/** Load debug logging enabled state. */
	bool IsDebugLoggingEnabled();
	
	/** Persist debug logging enabled state. */
	void SetDebugLogging(bool bEnabled);
}
