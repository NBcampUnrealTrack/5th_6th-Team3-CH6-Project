#include "Utils/VarcoSoundPathUtils.h"

#include "HAL/PlatformFilemanager.h"
#include "Misc/Paths.h"
#include "Utils/VarcoSoundSettings.h"
#include "UI/SAudioResultView.h"

namespace VarcoSoundPathUtils
{
	FString SanitizeFilename(const FString& Input, int32 MaxLength)
	{
		if (Input.IsEmpty())
		{
			return TEXT("untitled");
		}

		FString Result = Input;
		Result = FPaths::GetBaseFilename(Result);

		Result = Result.Replace(TEXT(" "), TEXT("_"));
		Result = Result.Replace(TEXT("/"), TEXT("_"));
		Result = Result.Replace(TEXT("\\"), TEXT("_"));
		Result = Result.Replace(TEXT(":"), TEXT("_"));
		Result = Result.Replace(TEXT("*"), TEXT("_"));
		Result = Result.Replace(TEXT("?"), TEXT("_"));
		Result = Result.Replace(TEXT("\""), TEXT("_"));
		Result = Result.Replace(TEXT("<"), TEXT("_"));
		Result = Result.Replace(TEXT(">"), TEXT("_"));
		Result = Result.Replace(TEXT("|"), TEXT("_"));
		Result = Result.Replace(TEXT("\r"), TEXT(""));
		Result = Result.Replace(TEXT("\n"), TEXT("_"));
		Result = Result.Replace(TEXT("\t"), TEXT("_"));

		while (Result.Contains(TEXT("__")))
		{
			Result = Result.Replace(TEXT("__"), TEXT("_"));
		}

		Result = Result.TrimStartAndEnd();
		Result.TrimStartInline();
		Result.TrimEndInline();
		if (Result.StartsWith(TEXT("_")))
		{
			Result = Result.RightChop(1);
		}
		if (Result.EndsWith(TEXT("_")))
		{
			Result = Result.LeftChop(1);
		}

		const int32 SafeMax = FMath::Max(1, MaxLength);
		if (Result.Len() > SafeMax)
		{
			Result = Result.Left(SafeMax);
			if (Result.EndsWith(TEXT("_")))
			{
				Result = Result.LeftChop(1);
			}
		}

		if (Result.IsEmpty())
		{
			Result = TEXT("untitled");
		}
		return Result;
	}

	FString MakeTimestamp_yyyyMMdd_HHmmss()
	{
		return FDateTime::Now().ToString(TEXT("%Y%m%d_%H%M%S"));
	}

	FString GetOutputRootDir()
	{
		return VarcoSoundSettings::LoadOutputRootDirectory();
	}

	static FString CombineOutput(const TCHAR* Subdir)
	{
		return FPaths::Combine(GetOutputRootDir(), Subdir);
	}

	FString GetOutputDirForTab(EAudioResultViewTabType TabType)
	{
		switch (TabType)
		{
		case EAudioResultViewTabType::Generator:
			return CombineOutput(TEXT("GeneratedSound"));
		case EAudioResultViewTabType::AutoLoop:
			return CombineOutput(TEXT("Looping"));
		case EAudioResultViewTabType::AutoVariation:
			return CombineOutput(TEXT("Variation"));
		case EAudioResultViewTabType::MonsterVoice:
			return CombineOutput(TEXT("MonsterVoice"));
		case EAudioResultViewTabType::BackgroundMusic:
			return CombineOutput(TEXT("BGM"));
		default:
			return CombineOutput(TEXT("GeneratedSound"));
		}
	}

	FString GetBgmOutputDir()
	{
		return CombineOutput(TEXT("BGM"));
	}

	FString GetBgmCacheDir()
	{
		return FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("VarcoSound"), TEXT("Cache"), TEXT("BGM"));
	}

	bool EnsureDirectoryExists(const FString& Dir)
	{
		IPlatformFile& PF = FPlatformFileManager::Get().GetPlatformFile();
		if (PF.DirectoryExists(*Dir))
		{
			return true;
		}
		return PF.CreateDirectoryTree(*Dir);
	}
}


