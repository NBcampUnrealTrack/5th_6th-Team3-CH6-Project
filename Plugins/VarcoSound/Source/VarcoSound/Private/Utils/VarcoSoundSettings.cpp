#include "Utils/VarcoSoundSettings.h"

#include "Misc/ConfigCacheIni.h"
#include "Misc/Paths.h"

namespace VarcoSoundSettings
{
	static const TCHAR* const ConfigSection = TEXT("VarcoSound.Settings");
	static const TCHAR* const ConfigKey = TEXT("OpenApiKey");
	static const TCHAR* const OutputRootDirKey = TEXT("OutputRootDir");
	static const TCHAR* const RecordingDirKey = TEXT("RecordingOutputDir");
	static const TCHAR* const GeneratedDirKey = TEXT("GeneratedOutputDir");
	static const TCHAR* const DebugLoggingKey = TEXT("EnableDebugLogging");

	static FString NormalizeDirectory(const FString& InDirectory)
	{
		FString CleanDirectory = InDirectory;
		CleanDirectory.TrimStartAndEndInline();

		if (CleanDirectory.IsEmpty())
		{
			return FString();
		}

		FPaths::NormalizeDirectoryName(CleanDirectory);
		return FPaths::ConvertRelativePathToFull(CleanDirectory);
	}

	FString LoadApiKey()
	{
		FString StoredKey;
		if (GConfig)
		{
			GConfig->GetString(ConfigSection, ConfigKey, StoredKey, GEditorPerProjectIni);
		}

		return StoredKey;
	}

	void SaveApiKey(const FString& InApiKey)
	{
		if (GConfig)
		{
			GConfig->SetString(ConfigSection, ConfigKey, *InApiKey, GEditorPerProjectIni);
			GConfig->Flush(false, GEditorPerProjectIni);
		}
	}

	FString GetDefaultOutputRootDirectory()
	{
		return FPaths::ConvertRelativePathToFull(FPaths::Combine(FPaths::ProjectContentDir(), TEXT("VarcoSound")));
	}

	FString LoadOutputRootDirectory()
	{
		FString StoredDirectory;
		if (GConfig)
		{
			GConfig->GetString(ConfigSection, OutputRootDirKey, StoredDirectory, GEditorPerProjectIni);
		}

		FString Normalized = NormalizeDirectory(StoredDirectory);
		if (Normalized.IsEmpty())
		{
			return GetDefaultOutputRootDirectory();
		}
		return Normalized;
	}

	void SaveOutputRootDirectory(const FString& InDirectory)
	{
		if (GConfig)
		{
			FString Normalized = NormalizeDirectory(InDirectory);
			if (Normalized.IsEmpty())
			{
				Normalized = GetDefaultOutputRootDirectory();
			}

			GConfig->SetString(ConfigSection, OutputRootDirKey, *Normalized, GEditorPerProjectIni);
			GConfig->Flush(false, GEditorPerProjectIni);
		}
	}

	FString GetDefaultRecordingDirectory()
	{
		return FPaths::ConvertRelativePathToFull(FPaths::Combine(FPaths::ProjectContentDir(), TEXT("Record")));
	}

	FString GetDefaultGeneratedAudioDirectory()
	{
		return FPaths::ConvertRelativePathToFull(FPaths::Combine(FPaths::ProjectContentDir(), TEXT("GeneratedAudio")));
	}

	FString LoadRecordingDirectory()
	{
		FString StoredDirectory;
		if (GConfig)
		{
			GConfig->GetString(ConfigSection, RecordingDirKey, StoredDirectory, GEditorPerProjectIni);
		}

		FString Normalized = NormalizeDirectory(StoredDirectory);
		if (Normalized.IsEmpty())
		{
			return GetDefaultRecordingDirectory();
		}

		return Normalized;
	}

	void SaveRecordingDirectory(const FString& InDirectory)
	{
		if (GConfig)
		{
			FString Normalized = NormalizeDirectory(InDirectory);
			if (Normalized.IsEmpty())
			{
				Normalized = GetDefaultRecordingDirectory();
			}

			GConfig->SetString(ConfigSection, RecordingDirKey, *Normalized, GEditorPerProjectIni);
			GConfig->Flush(false, GEditorPerProjectIni);
		}
	}

	FString LoadGeneratedAudioDirectory()
	{
		FString StoredDirectory;
		if (GConfig)
		{
			GConfig->GetString(ConfigSection, GeneratedDirKey, StoredDirectory, GEditorPerProjectIni);
		}

		FString Normalized = NormalizeDirectory(StoredDirectory);
		if (Normalized.IsEmpty())
		{
			return GetDefaultGeneratedAudioDirectory();
		}

		return Normalized;
	}

	void SaveGeneratedAudioDirectory(const FString& InDirectory)
	{
		if (GConfig)
		{
			FString Normalized = NormalizeDirectory(InDirectory);
			if (Normalized.IsEmpty())
			{
				Normalized = GetDefaultGeneratedAudioDirectory();
			}

			GConfig->SetString(ConfigSection, GeneratedDirKey, *Normalized, GEditorPerProjectIni);
			GConfig->Flush(false, GEditorPerProjectIni);
		}
	}
	
	bool IsDebugLoggingEnabled()
	{
		bool bEnabled = false;
		if (GConfig)
		{
			GConfig->GetBool(ConfigSection, DebugLoggingKey, bEnabled, GEditorPerProjectIni);
		}
		return bEnabled;
	}
	
	void SetDebugLogging(bool bEnabled)
	{
		if (GConfig)
		{
			GConfig->SetBool(ConfigSection, DebugLoggingKey, bEnabled, GEditorPerProjectIni);
			GConfig->Flush(false, GEditorPerProjectIni);
		}
	}
}
