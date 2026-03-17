// Copyright Epic Games, Inc. All Rights Reserved.

#include "Utils/ApiClient.h"
#include "VarcoSound.h"
#include "Modules/ModuleManager.h"
#include "Utils/AudioUtils.h"
#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Misc/FileHelper.h"
#include "HAL/PlatformFileManager.h"
#include "Misc/Paths.h"
#include "Async/Async.h"
#include "Misc/Guid.h"
#include "Utils/ApiEndpoints.h"
#include "Misc/Base64.h"
#include "Utils/VarcoSoundSettings.h"
#include "Utils/ToastNotification.h"
#include "Utils/VarcoSoundPathUtils.h"
#include "Interfaces/IPluginManager.h"

FApiClient::FApiClient()
    : bIsLoading(false)
    , ApiKey(VarcoSoundSettings::LoadApiKey())
    , bHasCachedUserAgent(false)
{
}

FApiClient::~FApiClient()
{
}

void FApiClient::SetApiKey(const FString& InApiKey)
{
    const FString NormalizedKey = InApiKey.TrimStartAndEnd();
    if (ApiKey.Equals(NormalizedKey, ESearchCase::CaseSensitive))
    {
        return;
    }

    ApiKey = NormalizedKey;
    VarcoSoundSettings::SaveApiKey(ApiKey);
}

FString FApiClient::GetApiKey() const
{
    if (ApiKey.IsEmpty())
    {
        return VarcoSoundSettings::LoadApiKey();
    }
    return ApiKey;
}

bool FApiClient::AddApiKeyHeader(IHttpRequest& HttpRequest) const
{
    FString EffectiveKey = ApiKey;
    if (EffectiveKey.IsEmpty())
    {
        EffectiveKey = VarcoSoundSettings::LoadApiKey();
    }
    if (EffectiveKey.IsEmpty())
    {
		const FText Message = NSLOCTEXT("VarcoSound", "ApiKeyNotSet", "API Key is not set. Please enter your API key in Settings.");
		
        VarcoSoundToast::ShowWarning(Message);
		if (FVarcoSoundModule* Module = FModuleManager::GetModulePtr<FVarcoSoundModule>("VarcoSound"))
		{
			Module->ShowSettingsForApiKeyIssue(Message);
		}
        UE_LOG(LogTemp, Warning, TEXT("OPENAPI_KEY is not set. Please enter your API key in Settings."));
        return false;
    }

    HttpRequest.SetHeader(TEXT("OPENAPI_KEY"), EffectiveKey);
    return true;
}

void FApiClient::AddUserAgentHeader(IHttpRequest& HttpRequest) const
{
    const FString UserAgent = GetUserAgentString();
    if (!UserAgent.IsEmpty())
    {
        HttpRequest.SetHeader(TEXT("User-Agent"), UserAgent);
    }
}

bool FApiClient::AddCommonHeaders(IHttpRequest& HttpRequest) const
{
    if (!AddApiKeyHeader(HttpRequest))
    {
        return false;
    }
    AddUserAgentHeader(HttpRequest);
    return true;
}

FString FApiClient::GetUserAgentString() const
{
    if (bHasCachedUserAgent)
    {
        return CachedUserAgent;
    }

    const FString VersionName = GetPluginVersionName();
    const FString OsName = GetOsNameForUserAgent();
    CachedUserAgent = FString::Printf(TEXT("VARCO Sound Unreal/%s/%s"), *VersionName, *OsName);
    bHasCachedUserAgent = true;
    return CachedUserAgent;
}

FString FApiClient::GetPluginVersionName() const
{
    // uplugin의 VersionName을 우선 사용
    TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin(TEXT("VarcoSound"));
    if (Plugin.IsValid())
    {
        const FString VersionName = Plugin->GetDescriptor().VersionName;
        if (!VersionName.IsEmpty())
        {
            return VersionName;
        }
    }
    return TEXT("unknown");
}

FString FApiClient::GetOsNameForUserAgent() const
{
#if PLATFORM_WINDOWS
    return TEXT("Windows");
#elif PLATFORM_MAC
    return TEXT("macOS");
#elif PLATFORM_LINUX
    return TEXT("Linux");
#else
    // 기타 플랫폼은 엔진의 플랫폼 이름을 그대로 사용
    return FString(FPlatformProperties::PlatformName());
#endif
}


bool FApiClient::LoadFileAsBase64(const FString& FilePath, FString& OutBase64, int64 MaxFileSize) const
{
    OutBase64.Empty();
    
    IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
    int64 FileSize = PlatformFile.FileSize(*FilePath);
    
    if (FileSize < 0)
    {
        VarcoSoundToast::ShowError(NSLOCTEXT("VarcoSound", "FileNotFound", "File not found or inaccessible."));
        UE_LOG(LogTemp, Error, TEXT("File not found: %s"), *FilePath);
        return false;
    }
    
    if (FileSize > MaxFileSize)
    {
        VarcoSoundToast::ShowError(FText::Format(
            NSLOCTEXT("VarcoSound", "FileTooLarge", "File is too large ({0} MB). Maximum size is {1} MB."),
            FText::AsNumber(FileSize / 1024.0 / 1024.0),
            FText::AsNumber(MaxFileSize / 1024.0 / 1024.0)));
        UE_LOG(LogTemp, Error, TEXT("File too large: %s (%.2f MB, limit %.2f MB)"), *FilePath, FileSize / 1024.0 / 1024.0, MaxFileSize / 1024.0 / 1024.0);
        return false;
    }
    
    TArray<uint8> FileData;
    if (!FFileHelper::LoadFileToArray(FileData, *FilePath))
    {
        VarcoSoundToast::ShowError(NSLOCTEXT("VarcoSound", "FileLoadFailed", "Failed to read file."));
        UE_LOG(LogTemp, Error, TEXT("Failed to load file: %s"), *FilePath);
        return false;
    }

    OutBase64 = FBase64::Encode(FileData.GetData(), FileData.Num());
    return true;
}

FApiResponseResult FApiClient::ValidateApiResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful, const FString& ApiName) const
{
    FApiResponseResult Result;
    
    if (!bWasSuccessful || !Response.IsValid())
    {
        Result.bSuccess = false;
        Result.ErrorMessage = FString::Printf(TEXT("%s API response: Network error"), *ApiName);
        VarcoSoundToast::ShowError(NSLOCTEXT("VarcoSound", "NetworkError", "Network error. Please check your internet connection."));
        UE_LOG(LogTemp, Error, TEXT("%s"), *Result.ErrorMessage);
        return Result;
    }
    
    Result.StatusCode = Response->GetResponseCode();
    Result.ResponseBody = Response->GetContentAsString();
    
    if (Result.StatusCode >= 200 && Result.StatusCode < 300)
    {
        Result.bSuccess = true;
    }
    else
    {
        Result.bSuccess = false;
        Result.ErrorMessage = FString::Printf(TEXT("%s API response: Error %d"), *ApiName, Result.StatusCode);
        
        FString UserMessage = TEXT("Request failed.");
        switch (Result.StatusCode)
        {
			case 400: UserMessage = TEXT("Bad request."); break;
            case 401: UserMessage = TEXT("Invalid API Key."); break;
            case 403: UserMessage = TEXT("API Key expired or quota exceeded."); break;
            case 429: UserMessage = TEXT("Too many requests."); break;
            case 500: UserMessage = TEXT("Server error."); break;
            default:  UserMessage = FString::Printf(TEXT("Error code: %d"), Result.StatusCode); break;
        }

		// Provide a quick Settings shortcut for API errors across all tabs
		// (User requested to include 400 as well)
		const bool bShowSettingsShortcut = (Result.StatusCode == 400 || Result.StatusCode == 401 || Result.StatusCode == 403);
		if (bShowSettingsShortcut)
		{
			const FText Hint = NSLOCTEXT("VarcoSound", "ApiKeySettingsHint", "Please enter your API key in Settings.");
			const FText Combined = FText::Format(
				NSLOCTEXT("VarcoSound", "ApiKeyErrorCombined", "{0} {1}"),
				FText::FromString(UserMessage),
				Hint);

			VarcoSoundToast::ShowError(Combined);
			if (FVarcoSoundModule* Module = FModuleManager::GetModulePtr<FVarcoSoundModule>("VarcoSound"))
			{
				Module->ShowSettingsForApiKeyIssue(Combined);
			}
		}
		else
		{
			VarcoSoundToast::ShowError(FText::FromString(UserMessage));
		}
        UE_LOG(LogTemp, Error, TEXT("%s - %s: %s"), *ApiName, *UserMessage, *Result.ResponseBody);
    }
    
    return Result;
}

void FApiClient::SetIsLoading(bool bLoading)
{
    bIsLoading = bLoading;
}

bool FApiClient::IsLoading() const
{
    return bIsLoading;
}

// ----------------------------------------------------------------------------------
// API Implementations using Generic Helpers
// ----------------------------------------------------------------------------------

void FApiClient::SendGenerateRequest(const FString& Prompt, int32 NumSamples, FOnGenApiResponse InResponseDelegate)
{
    FVarcoGenRequest Request;
    Request.prompt = Prompt;
    Request.num_sample = NumSamples;

    SendJsonRequestArrayResponse<FVarcoGenRequest, FVarcoAudioItem>(
        SPApi::Gen(),
        Request,
        [InResponseDelegate](const TArray<FVarcoAudioItem>& Items)
        {
            TArray<FString> AudioList;
            for (const auto& Item : Items) AudioList.Add(Item.audio);
            if (InResponseDelegate.IsBound()) InResponseDelegate.Execute(AudioList);
        },
        [InResponseDelegate](const FString& Error)
        {
            if (InResponseDelegate.IsBound()) InResponseDelegate.Execute(TArray<FString>());
        }
    );
}

void FApiClient::SendLoopingRequest(const FString& FilePath, FOnLoopingApiResponse InResponseDelegate)
{
    FString SourceBase64;
    if (!LoadFileAsBase64(FilePath, SourceBase64))
    {
        if (InResponseDelegate.IsBound()) InResponseDelegate.Execute(FString());
        return;
    }

    FVarcoLoopingRequest Request;
    Request.source = SourceBase64;

    SendJsonRequest<FVarcoLoopingRequest, FVarcoLoopingResponse>(
        SPApi::Looping(),
        Request,
        [InResponseDelegate](const FVarcoLoopingResponse& Response)
        {
            if (InResponseDelegate.IsBound()) InResponseDelegate.Execute(Response.audio);
        },
        [InResponseDelegate](const FString& Error)
        {
            if (InResponseDelegate.IsBound()) InResponseDelegate.Execute(FString());
        }
    );
}

void FApiClient::SendLoopingRequest(USoundWave* InWave, FOnLoopingApiResponse InResponseDelegate)
{
    FString SourceBase64 = FAudioUtils::GetSoundWaveBase64(InWave);
    if (SourceBase64.IsEmpty())
    {
        if (InResponseDelegate.IsBound()) InResponseDelegate.Execute(FString());
        return;
    }

    FVarcoLoopingRequest Request;
    Request.source = SourceBase64;

    SendJsonRequest<FVarcoLoopingRequest, FVarcoLoopingResponse>(
        SPApi::Looping(),
        Request,
        [InResponseDelegate](const FVarcoLoopingResponse& Response)
        {
            if (InResponseDelegate.IsBound()) InResponseDelegate.Execute(Response.audio);
        },
        [InResponseDelegate](const FString& Error)
        {
            if (InResponseDelegate.IsBound()) InResponseDelegate.Execute(FString());
        }
    );
}

void FApiClient::SendVariationRequest(const FString& FilePath, int32 NumSample, float Strength, FOnVariationApiResponse InResponseDelegate)
{
    FString SourceBase64;
    if (!LoadFileAsBase64(FilePath, SourceBase64))
    {
        if (InResponseDelegate.IsBound()) InResponseDelegate.Execute(TArray<FString>());
        return;
    }
    SendVariationRequestFromBase64(SourceBase64, NumSample, Strength, InResponseDelegate);
}

void FApiClient::SendVariationRequest(USoundWave* InWave, int32 NumSample, float Strength, FOnVariationApiResponse InResponseDelegate)
{
    FString SourceBase64 = FAudioUtils::GetSoundWaveBase64(InWave);
    if (SourceBase64.IsEmpty())
    {
        if (InResponseDelegate.IsBound()) InResponseDelegate.Execute(TArray<FString>());
        return;
    }
    SendVariationRequestFromBase64(SourceBase64, NumSample, Strength, InResponseDelegate);
}

void FApiClient::SendVariationRequestFromBase64(const FString& SourceBase64, int32 NumSample, float Strength, FOnVariationApiResponse InResponseDelegate)
{
    if (SourceBase64.IsEmpty())
    {
        if (InResponseDelegate.IsBound()) InResponseDelegate.Execute(TArray<FString>());
        return;
    }

    FVarcoVariationRequest Request;
    Request.source = SourceBase64;
    Request.num_sample = FMath::Clamp(NumSample, 1, 3);
    Request.strength = Strength;

    SendJsonRequestArrayResponse<FVarcoVariationRequest, FVarcoAudioItem>(
        SPApi::Variation(),
        Request,
        [InResponseDelegate](const TArray<FVarcoAudioItem>& Items)
        {
            TArray<FString> AudioList;
            for (const auto& Item : Items) AudioList.Add(Item.audio);
            if (InResponseDelegate.IsBound()) InResponseDelegate.Execute(AudioList);
        },
        [InResponseDelegate](const FString& Error)
        {
            if (InResponseDelegate.IsBound()) InResponseDelegate.Execute(TArray<FString>());
        }
    );
}

void FApiClient::SendMono2StereoRequest(const FString& SourceBase64, FOnMono2StereoApiResponse InResponseDelegate)
{
    if (SourceBase64.IsEmpty())
    {
        if (InResponseDelegate.IsBound()) InResponseDelegate.Execute(TArray<FString>());
        return;
    }

    FVarcoMono2StereoRequest Request;
    Request.source = SourceBase64;

    SendJsonRequest<FVarcoMono2StereoRequest, FVarcoMono2StereoResponse>(
        SPApi::Mono2Stereo(),
        Request,
        [InResponseDelegate](const FVarcoMono2StereoResponse& Response)
        {
            TArray<FString> Result;
            Result.Add(Response.audio);
            if (InResponseDelegate.IsBound()) InResponseDelegate.Execute(Result);
        },
        [InResponseDelegate](const FString& Error)
        {
            if (InResponseDelegate.IsBound()) InResponseDelegate.Execute(TArray<FString>());
        }
    );
}

void FApiClient::SendMono2StereoRequestFromFile(const FString& FilePath, FOnMono2StereoApiResponse InResponseDelegate)
{
    FString SourceBase64;
    if (!LoadFileAsBase64(FilePath, SourceBase64))
    {
        if (InResponseDelegate.IsBound()) InResponseDelegate.Execute(TArray<FString>());
        return;
    }
    SendMono2StereoRequest(SourceBase64, InResponseDelegate);
}

void FApiClient::SendMonsterVoiceRequest(const FString& SourceFilePath, const FString& TargetFilePath, float ConversionRatio, FOnMonsterVoiceApiResponse InResponseDelegate)
{
    FString SourceBase64, ReferenceBase64;
    if (!LoadFileAsBase64(SourceFilePath, SourceBase64) || !LoadFileAsBase64(TargetFilePath, ReferenceBase64))
    {
        if (InResponseDelegate.IsBound()) InResponseDelegate.Execute(FString());
        return;
    }
    SendMonsterVoiceRequestInternal(SourceBase64, ReferenceBase64, ConversionRatio, InResponseDelegate);
}

void FApiClient::SendMonsterVoiceRequest(USoundWave* Source, USoundWave* Target, float ConversionRatio, FOnMonsterVoiceApiResponse InResponseDelegate)
{
    FString SourceBase64 = FAudioUtils::GetSoundWaveBase64(Source);
    FString ReferenceBase64 = FAudioUtils::GetSoundWaveBase64(Target);
    
    if (SourceBase64.IsEmpty() || ReferenceBase64.IsEmpty())
    {
        if (InResponseDelegate.IsBound()) InResponseDelegate.Execute(FString());
        return;
    }
    SendMonsterVoiceRequestInternal(SourceBase64, ReferenceBase64, ConversionRatio, InResponseDelegate);
}

void FApiClient::SendMonsterVoiceRequestFromBase64(const FString& SourceBase64, const FString& TargetFilePath, float ConversionRatio, FOnMonsterVoiceApiResponse InResponseDelegate)
{
    FString ReferenceBase64;
    if (!LoadFileAsBase64(TargetFilePath, ReferenceBase64))
    {
        if (InResponseDelegate.IsBound()) InResponseDelegate.Execute(FString());
        return;
    }

    SendMonsterVoiceRequestInternal(SourceBase64, ReferenceBase64, ConversionRatio, InResponseDelegate);
}

void FApiClient::SendMonsterVoiceRequestFromBase64(const FString& SourceBase64, USoundWave* Target, float ConversionRatio, FOnMonsterVoiceApiResponse InResponseDelegate)
{
    FString ReferenceBase64 = FAudioUtils::GetSoundWaveBase64(Target);
    if (SourceBase64.IsEmpty() || ReferenceBase64.IsEmpty())
    {
        if (InResponseDelegate.IsBound()) InResponseDelegate.Execute(FString());
        return;
    }
    SendMonsterVoiceRequestInternal(SourceBase64, ReferenceBase64, ConversionRatio, InResponseDelegate);
}

void FApiClient::SendMonsterVoiceRequestInternal(const FString& SourceBase64, const FString& ReferenceBase64, float ConversionRatio, FOnMonsterVoiceApiResponse InResponseDelegate)
{
    FVarcoMonsterVoiceRequest Request;
    Request.source = SourceBase64;
    Request.reference = ReferenceBase64;
    Request.ratio = ConversionRatio;
    Request.enhance = true;

    SendJsonRequest<FVarcoMonsterVoiceRequest, FVarcoMonsterVoiceResponse>(
        SPApi::Conversion(),
        Request,
        [InResponseDelegate](const FVarcoMonsterVoiceResponse& Response)
        {
            if (InResponseDelegate.IsBound()) InResponseDelegate.Execute(Response.audio);
        },
        [InResponseDelegate](const FString& Error)
        {
            if (InResponseDelegate.IsBound()) InResponseDelegate.Execute(FString());
        }
    );
}

void FApiClient::SendPromptBoosterRequest(const FString& Prompt, FOnPromptBoosterApiResponse InResponseDelegate)
{
    FVarcoPromptBoosterRequest Request;
    Request.prompt = Prompt;

    SetIsLoading(true);
    
    FString OutputString;
    FJsonObjectConverter::UStructToJsonObjectString(Request, OutputString);

    TSharedRef<IHttpRequest, ESPMode::ThreadSafe> HttpRequest = FHttpModule::Get().CreateRequest();
    HttpRequest->SetVerb("POST");
    HttpRequest->SetURL(SPApi::PromptBooster());
    HttpRequest->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
    HttpRequest->SetTimeout(30.0f);
    if (!AddCommonHeaders(*HttpRequest))
    {
        SetIsLoading(false);
        if (InResponseDelegate.IsBound()) InResponseDelegate.Execute(TArray<FString>());
        return;
    }
    HttpRequest->SetContentAsString(OutputString);

    HttpRequest->OnProcessRequestComplete().BindLambda(
        [this, InResponseDelegate](FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
        {
            SetIsLoading(false);
            FApiResponseResult Result = ValidateApiResponse(Request, Response, bWasSuccessful, TEXT("Prompt Booster"));
            
            TArray<FString> PromptSuggestions;
            if (Result.bSuccess)
            {
                TArray<TSharedPtr<FJsonValue>> JsonArray;
                TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Result.ResponseBody);
                if (FJsonSerializer::Deserialize(Reader, JsonArray))
                {
                    for (const auto& Val : JsonArray)
                    {
                        FString Str;
                        if (Val->TryGetString(Str)) PromptSuggestions.Add(Str);
                    }
                }
            }
            
            if (InResponseDelegate.IsBound()) InResponseDelegate.Execute(PromptSuggestions);
        }
    );
    HttpRequest->ProcessRequest();
}

void FApiClient::SendImage2SfxRequest(const FString& Prompt, const FString& ImageBase64, FOnImage2SfxApiResponse InResponseDelegate)
{
    FVarcoImage2SfxRequest Request;
    Request.prompt = Prompt.IsEmpty() ? TEXT("foley, embience, sfx") : Prompt;
    Request.image = ImageBase64;
    
    SetIsLoading(true);
    FString OutputString;
    FJsonObjectConverter::UStructToJsonObjectString(Request, OutputString);
    
    TSharedRef<IHttpRequest, ESPMode::ThreadSafe> HttpRequest = FHttpModule::Get().CreateRequest();
    HttpRequest->SetVerb("POST");
    HttpRequest->SetURL(SPApi::Image2Sfx());
    HttpRequest->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
    HttpRequest->SetTimeout(60.0f);
    if (!AddCommonHeaders(*HttpRequest))
    {
        SetIsLoading(false);
        if (InResponseDelegate.IsBound()) InResponseDelegate.Execute(FString());
        return;
    }
    HttpRequest->SetContentAsString(OutputString);

    HttpRequest->OnProcessRequestComplete().BindLambda(
        [this, InResponseDelegate](FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
        {
            SetIsLoading(false);
            FApiResponseResult Result = ValidateApiResponse(Request, Response, bWasSuccessful, TEXT("Image2Sfx"));
            if (InResponseDelegate.IsBound()) InResponseDelegate.Execute(Result.bSuccess ? Result.ResponseBody : FString());
        }
    );
    HttpRequest->ProcessRequest();
}

FString FApiClient::ExtractFilenameFromContentDisposition(const FString& HeaderValue) const
{
    FString Filename;
    const FString Token = TEXT("filename=");
    int32 Index = HeaderValue.Find(Token, ESearchCase::IgnoreCase, ESearchDir::FromStart);
    if (Index != INDEX_NONE)
    {
        Filename = HeaderValue.Mid(Index + Token.Len()).TrimStartAndEnd();
        Filename.RemoveFromStart(TEXT("\""));
        Filename.RemoveFromEnd(TEXT("\""));
    }
    return Filename;
}

void FApiClient::SendMusicGenerationRequest(const FString& Prompt, const FString& ImageBase64, FOnMusicTaskCreated InResponseDelegate)
{
    SetIsLoading(true);

    FString OutputString;
    // Build JSON dynamically so we can omit fields by mode:
    // - {"image": "...", "prompt": "..."}
    // - {"image": "..."}
    // - {"prompt": "..."}
    TSharedPtr<FJsonObject> Payload = MakeShared<FJsonObject>();
    const FString TrimmedPrompt = Prompt.TrimStartAndEnd();
    if (!TrimmedPrompt.IsEmpty())
    {
        Payload->SetStringField(TEXT("prompt"), TrimmedPrompt);
    }
    if (!ImageBase64.IsEmpty())
    {
        Payload->SetStringField(TEXT("image"), ImageBase64);
    }

    if (Payload->Values.Num() == 0)
    {
        SetIsLoading(false);
        UE_LOG(LogTemp, Error, TEXT("[BGM Generate] Request payload is empty (no prompt/image)."));
        if (InResponseDelegate.IsBound()) { InResponseDelegate.Execute(false, TEXT("EmptyPayload")); }
        return;
    }

    {
        TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
        if (!FJsonSerializer::Serialize(Payload.ToSharedRef(), Writer))
        {
            SetIsLoading(false);
            UE_LOG(LogTemp, Error, TEXT("[BGM Generate] JSON serialization failed"));
            if (InResponseDelegate.IsBound()) { InResponseDelegate.Execute(false, TEXT("SerializationFailed")); }
            return;
        }
    }

    if (OutputString.IsEmpty())
    {
        SetIsLoading(false);
        UE_LOG(LogTemp, Error, TEXT("[BGM Generate] JSON serialization produced empty string"));
        if (InResponseDelegate.IsBound()) { InResponseDelegate.Execute(false, TEXT("SerializationFailed")); }
        return;
    }

    TSharedRef<IHttpRequest, ESPMode::ThreadSafe> HttpRequest = FHttpModule::Get().CreateRequest();
    HttpRequest->SetVerb("POST");
    HttpRequest->SetURL(SPApi::BgmGenerateMusic());
    HttpRequest->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
    HttpRequest->SetTimeout(30.0f);
    if (!AddCommonHeaders(*HttpRequest))
    {
        SetIsLoading(false);
        if (InResponseDelegate.IsBound()) { InResponseDelegate.Execute(false, TEXT("API Key is not set")); }
        return;
    }
    HttpRequest->SetContentAsString(OutputString);

    UE_LOG(LogTemp, Log, TEXT("[BGM Generate] Sending request to %s"), *SPApi::BgmGenerateMusic());

    HttpRequest->OnProcessRequestComplete().BindLambda(
        [this, InResponseDelegate](FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
        {
            SetIsLoading(false);
            FApiResponseResult Result = ValidateApiResponse(Request, Response, bWasSuccessful, TEXT("BgmGenerateMusic"));
            if (!Result.bSuccess)
            {
                UE_LOG(LogTemp, Error, TEXT("[BGM Generate] Request failed: %s"), *Result.ErrorMessage);
                if (InResponseDelegate.IsBound()) { InResponseDelegate.Execute(false, Result.ErrorMessage); }
                return;
            }

            FBgmGenerateMusicResponse Parsed;
            if (FJsonObjectConverter::JsonObjectStringToUStruct(Result.ResponseBody, &Parsed, 0, 0) && !Parsed.task_id.IsEmpty())
            {
                UE_LOG(LogTemp, Log, TEXT("[BGM Generate] Task created: %s"), *Parsed.task_id);
                if (InResponseDelegate.IsBound()) { InResponseDelegate.Execute(true, Parsed.task_id); }
                return;
            }

            UE_LOG(LogTemp, Error, TEXT("[BGM Generate] Failed to parse task_id from response: %s"), *Result.ResponseBody);
            if (InResponseDelegate.IsBound()) { InResponseDelegate.Execute(false, TEXT("InvalidResponse")); }
        }
    );
    HttpRequest->ProcessRequest();
}

void FApiClient::GetMusicGenerationStatus(const FString& TaskId, FOnMusicStatusReceived InResponseDelegate)
{
    TSharedRef<IHttpRequest, ESPMode::ThreadSafe> HttpRequest = FHttpModule::Get().CreateRequest();
    HttpRequest->SetVerb("GET");
    HttpRequest->SetURL(SPApi::BgmGeneratedMusicStatus(TaskId));
    HttpRequest->SetTimeout(30.0f);
    if (!AddCommonHeaders(*HttpRequest))
    {
        if (InResponseDelegate.IsBound()) 
        {
            FBgmMusicStatusResponse EmptyResponse;
            InResponseDelegate.Execute(false, EmptyResponse);
        }
        return;
    }

    UE_LOG(LogTemp, Log, TEXT("[BGM Status] Polling task %s..."), *TaskId);

    HttpRequest->OnProcessRequestComplete().BindLambda(
        [this, TaskId, InResponseDelegate](FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
        {
            FApiResponseResult Result = ValidateApiResponse(Request, Response, bWasSuccessful, TEXT("BgmMusicStatus"));
            FBgmMusicStatusResponse Parsed;
            if (Result.bSuccess && FJsonObjectConverter::JsonObjectStringToUStruct(Result.ResponseBody, &Parsed, 0, 0))
            {
                UE_LOG(LogTemp, Log, TEXT("[BGM Status] Task %s status: %s"), *TaskId, *Parsed.status);
                if (InResponseDelegate.IsBound()) { InResponseDelegate.Execute(true, Parsed); }
            }
            else
            {
                UE_LOG(LogTemp, Error, TEXT("[BGM Status] Failed to parse status for task %s: %s"), *TaskId, *Result.ResponseBody);
                if (InResponseDelegate.IsBound()) { InResponseDelegate.Execute(false, Parsed); }
            }
        }
    );
    HttpRequest->ProcessRequest();
}

void FApiClient::DownloadMusicFile(const FString& MusicId, const FString& DesiredFileName, TFunction<void(bool /*bSuccess*/, const FBgmMusicDownloadResult&)> InResponseDelegate)
{
    FBgmMusicDownloadResult DownloadResult;

    TSharedRef<IHttpRequest, ESPMode::ThreadSafe> HttpRequest = FHttpModule::Get().CreateRequest();
    HttpRequest->SetVerb("GET");
    HttpRequest->SetURL(SPApi::BgmMusicDownload(MusicId));
    HttpRequest->SetTimeout(120.0f);
    if (!AddCommonHeaders(*HttpRequest))
    {
        DownloadResult.ErrorMessage = TEXT("API Key is not set");
        if (InResponseDelegate) { InResponseDelegate(false, DownloadResult); }
        return;
    }

    UE_LOG(LogTemp, Log, TEXT("[BGM Download] Starting download for music ID: %s"), *MusicId);

    HttpRequest->OnProcessRequestComplete().BindLambda(
        [this, MusicId, DesiredFileName, InResponseDelegate, DownloadResult](FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful) mutable
        {
            if (!bWasSuccessful || !Response.IsValid())
            {
                DownloadResult.ErrorMessage = TEXT("Network error");
                UE_LOG(LogTemp, Error, TEXT("[BGM Download] Network error for music ID: %s"), *MusicId);
                if (InResponseDelegate) { InResponseDelegate(false, DownloadResult); }
                return;
            }

            const int32 StatusCode = Response->GetResponseCode();
            if (StatusCode < 200 || StatusCode >= 300)
            {
                DownloadResult.ErrorMessage = FString::Printf(TEXT("HTTP %d"), StatusCode);
                UE_LOG(LogTemp, Error, TEXT("[BGM Download] HTTP %d for music ID: %s"), StatusCode, *MusicId);
                if (InResponseDelegate) { InResponseDelegate(false, DownloadResult); }
                return;
            }

            FString FileName = DesiredFileName.TrimStartAndEnd();
            if (FileName.IsEmpty())
            {
                const FString Disposition = Response->GetHeader(TEXT("Content-Disposition"));
                FileName = ExtractFilenameFromContentDisposition(Disposition);
            }
            if (FileName.IsEmpty())
            {
                FileName = FString::Printf(TEXT("%s.mp3"), *MusicId);
            }

            const FString SaveDir = VarcoSoundPathUtils::GetBgmOutputDir();
            IPlatformFile& PF = FPlatformFileManager::Get().GetPlatformFile();
            if (!PF.DirectoryExists(*SaveDir))
            {
                PF.CreateDirectoryTree(*SaveDir);
            }

            const FString SavePath = FPaths::Combine(SaveDir, FileName);
            const TArray<uint8>& Content = Response->GetContent();
            if (!FFileHelper::SaveArrayToFile(Content, *SavePath))
            {
                DownloadResult.ErrorMessage = TEXT("Failed to save file");
                UE_LOG(LogTemp, Error, TEXT("[BGM Download] Failed to save file for music ID: %s"), *MusicId);
                if (InResponseDelegate) { InResponseDelegate(false, DownloadResult); }
                return;
            }

            DownloadResult.FileName = FileName;
            DownloadResult.FilePath = SavePath;

            UE_LOG(LogTemp, Log, TEXT("[BGM Download] Success: %s saved to %s"), *FileName, *SavePath);
            if (InResponseDelegate) { InResponseDelegate(true, DownloadResult); }
        }
    );

    HttpRequest->ProcessRequest();
}

