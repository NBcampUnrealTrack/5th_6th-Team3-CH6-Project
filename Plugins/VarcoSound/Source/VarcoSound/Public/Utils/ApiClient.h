// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Http.h"
#include "Utils/VarcoApiTypes.h"
#include "JsonObjectConverter.h"
#include "Utils/ToastNotification.h"
#include "Utils/ApiEndpoints.h"
#include "Interfaces/IHttpResponse.h"
#include "Serialization/JsonSerializer.h"

class USoundWave;

// API response handling delegate definitions (dedicated delegate pattern)
DECLARE_DELEGATE_OneParam(FOnGenApiResponse, const TArray<FString>&);
DECLARE_DELEGATE_OneParam(FOnLoopingApiResponse, const FString&);
DECLARE_DELEGATE_OneParam(FOnVariationApiResponse, const TArray<FString>&);
DECLARE_DELEGATE_OneParam(FOnMono2StereoApiResponse, const TArray<FString>&);
DECLARE_DELEGATE_OneParam(FOnMonsterVoiceApiResponse, const FString&);
DECLARE_DELEGATE_OneParam(FOnPromptBoosterApiResponse, const TArray<FString>&);
DECLARE_DELEGATE_OneParam(FOnImage2SfxApiResponse, const FString&);
DECLARE_DELEGATE_TwoParams(FOnMusicTaskCreated, bool /*bSuccess*/, const FString& /*TaskIdOrError*/);
DECLARE_DELEGATE_TwoParams(FOnMusicStatusReceived, bool /*bSuccess*/, const FBgmMusicStatusResponse& /*Response*/);

// API response validation result structure
struct FApiResponseResult
{
    bool bSuccess;
    int32 StatusCode;
    FString ErrorMessage;
    FString ResponseBody;
    
    FApiResponseResult()
        : bSuccess(false)
        , StatusCode(0)
        , ErrorMessage(TEXT(""))
        , ResponseBody(TEXT(""))
    {}
};

struct FBgmMusicDownloadResult
{
    FString FilePath;
    FString FileName;
    FString ErrorMessage;
};

class VARCOSOUND_API FApiClient
{
public:
    // File size limits (in bytes)
    static constexpr int64 MAX_AUDIO_FILE_SIZE = 10 * 1024 * 1024;  // 10 MB
    static constexpr int64 MAX_IMAGE_FILE_SIZE = 50 * 1024 * 1024;  // 50 MB
    
    FApiClient();
    ~FApiClient();
    
    // API key setting and retrieval
    void SetApiKey(const FString& InApiKey);
    FString GetApiKey() const;
    
    // Send sound generation API request
    void SendGenerateRequest(const FString& Prompt, int32 NumSamples, FOnGenApiResponse InResponseDelegate);
    
    // Send looping API request
    void SendLoopingRequest(const FString& FilePath, FOnLoopingApiResponse InResponseDelegate);
    void SendLoopingRequest(USoundWave* InWave, FOnLoopingApiResponse InResponseDelegate);
    
    // Send variation API request
    void SendVariationRequest(const FString& FilePath, int32 NumSample, float Strength, FOnVariationApiResponse InResponseDelegate);
    void SendVariationRequest(USoundWave* InWave, int32 NumSample, float Strength, FOnVariationApiResponse InResponseDelegate);
    void SendVariationRequestFromBase64(const FString& SourceBase64, int32 NumSample, float Strength, FOnVariationApiResponse InResponseDelegate);
    
    // Send mono2stereo API request (direct Base64 pass)
    void SendMono2StereoRequest(const FString& SourceBase64, FOnMono2StereoApiResponse InResponseDelegate);
    void SendMono2StereoRequestFromFile(const FString& FilePath, FOnMono2StereoApiResponse InResponseDelegate);
    
    // Send monster voice conversion API request
    void SendMonsterVoiceRequest(const FString& SourceFilePath, const FString& TargetFilePath, float ConversionRatio, FOnMonsterVoiceApiResponse InResponseDelegate);
    void SendMonsterVoiceRequest(USoundWave* Source, USoundWave* Target, float ConversionRatio, FOnMonsterVoiceApiResponse InResponseDelegate);
    void SendMonsterVoiceRequestFromBase64(const FString& SourceBase64, const FString& TargetFilePath, float ConversionRatio, FOnMonsterVoiceApiResponse InResponseDelegate);
    void SendMonsterVoiceRequestFromBase64(const FString& SourceBase64, USoundWave* Target, float ConversionRatio, FOnMonsterVoiceApiResponse InResponseDelegate);
    
    // Send enhance-text-prompt API request
    void SendPromptBoosterRequest(const FString& Prompt, FOnPromptBoosterApiResponse InResponseDelegate);

    // Send Image2Sfx API request (prompt + image base64 JSON)
    void SendImage2SfxRequest(const FString& Prompt, const FString& ImageBase64, FOnImage2SfxApiResponse InResponseDelegate);
    
    // Music Generation API (multipart: optional file + description_form)
    void SendMusicGenerationRequest(const FString& Prompt, const FString& ImageBase64, FOnMusicTaskCreated InResponseDelegate);
    void GetMusicGenerationStatus(const FString& TaskId, FOnMusicStatusReceived InResponseDelegate);
    void DownloadMusicFile(const FString& MusicId, const FString& DesiredFileName, TFunction<void(bool /*bSuccess*/, const FBgmMusicDownloadResult&)> InResponseDelegate);
    
    // Set and check loading status
    void SetIsLoading(bool bLoading);
    bool IsLoading() const;

private:
    // Internal method for Monster Voice request
    void SendMonsterVoiceRequestInternal(const FString& SourceBase64, const FString& ReferenceBase64, float ConversionRatio, FOnMonsterVoiceApiResponse InResponseDelegate);

    /**
     * @brief Generic JSON request sending function
     * @tparam RequestType Request data structure (USTRUCT)
     * @tparam ResponseType Response data structure (USTRUCT)
     * @param Endpoint API endpoint URL
     * @param RequestPayload Request data
     * @param OnSuccess Callback to call on success
     * @param OnFailure Callback to call on failure (optional)
     */
    template <typename RequestType, typename ResponseType>
    void SendJsonRequest(
        const FString& Endpoint, 
        const RequestType& RequestPayload, 
        TFunction<void(const ResponseType&)> OnSuccess, 
        TFunction<void(const FString&)> OnFailure = nullptr)
    {
        SetIsLoading(true);

        FString OutputString;
        if (!FJsonObjectConverter::UStructToJsonObjectString(RequestPayload, OutputString))
        {
            SetIsLoading(false);
            UE_LOG(LogTemp, Error, TEXT("JSON Serialization Failed for %s"), *Endpoint);
            if (OnFailure) OnFailure(TEXT("JSON Serialization Failed"));
            return;
        }

        TSharedRef<IHttpRequest, ESPMode::ThreadSafe> HttpRequest = FHttpModule::Get().CreateRequest();
        HttpRequest->SetVerb("POST");
        HttpRequest->SetURL(Endpoint);
        HttpRequest->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
        HttpRequest->SetTimeout(60.0f);
        if (!AddCommonHeaders(*HttpRequest))
        {
            SetIsLoading(false);
            if (OnFailure) OnFailure(TEXT("API Key is not set"));
            return;
        }
        HttpRequest->SetContentAsString(OutputString);

        UE_LOG(LogTemp, Verbose, TEXT("API Request to %s: %s"), *Endpoint, *OutputString);

        HttpRequest->OnProcessRequestComplete().BindLambda(
            [this, Endpoint, OnSuccess, OnFailure](FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
            {
                SetIsLoading(false);
                
                FApiResponseResult Result = ValidateApiResponse(Request, Response, bWasSuccessful, Endpoint);
                if (!Result.bSuccess)
                {
                    if (OnFailure) OnFailure(Result.ErrorMessage);
                    return;
                }

                // Try parsing single object
                ResponseType ResponseStruct;
                if (FJsonObjectConverter::JsonObjectStringToUStruct(Result.ResponseBody, &ResponseStruct, 0, 0))
                {
                    OnSuccess(ResponseStruct);
                    return;
                }
                
                UE_LOG(LogTemp, Error, TEXT("JSON Deserialization Failed for %s: %s"), *Endpoint, *Result.ResponseBody);
                
                if (OnFailure) OnFailure(TEXT("JSON Deserialization Failed"));
            }
        );

        HttpRequest->ProcessRequest();
    }

    /**
     * @brief Template for handling responses with top-level JSON arrays (Gen, Variation, etc.)
     */
    template <typename RequestType, typename ItemType>
    void SendJsonRequestArrayResponse(
        const FString& Endpoint, 
        const RequestType& RequestPayload, 
        TFunction<void(const TArray<ItemType>&)> OnSuccess, 
        TFunction<void(const FString&)> OnFailure = nullptr)
    {
        SetIsLoading(true);

        FString OutputString;
        FJsonObjectConverter::UStructToJsonObjectString(RequestPayload, OutputString);

        TSharedRef<IHttpRequest, ESPMode::ThreadSafe> HttpRequest = FHttpModule::Get().CreateRequest();
        HttpRequest->SetVerb("POST");
        HttpRequest->SetURL(Endpoint);
        HttpRequest->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
        HttpRequest->SetTimeout(60.0f);
        if (!AddCommonHeaders(*HttpRequest))
        {
            SetIsLoading(false);
            if (OnFailure) OnFailure(TEXT("API Key is not set"));
            return;
        }
        HttpRequest->SetContentAsString(OutputString);

        UE_LOG(LogTemp, Verbose, TEXT("API Array Request to %s"), *Endpoint);

        HttpRequest->OnProcessRequestComplete().BindLambda(
            [this, Endpoint, OnSuccess, OnFailure](FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
            {
                SetIsLoading(false);
                
                FApiResponseResult Result = ValidateApiResponse(Request, Response, bWasSuccessful, Endpoint);
                if (!Result.bSuccess)
                {
                    if (OnFailure) OnFailure(Result.ErrorMessage);
                    return;
                }

                TArray<TSharedPtr<FJsonValue>> JsonArray;
                TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Result.ResponseBody);
                if (FJsonSerializer::Deserialize(Reader, JsonArray))
                {
                    TArray<ItemType> OutArray;
                    if (FJsonObjectConverter::JsonArrayToUStruct(JsonArray, &OutArray, 0, 0))
                    {
                        OnSuccess(OutArray);
                        return;
                    }
                }

                UE_LOG(LogTemp, Error, TEXT("JSON Array Deserialization Failed for %s"), *Endpoint);
                if (OnFailure) OnFailure(TEXT("JSON Array Deserialization Failed"));
            }
        );

        HttpRequest->ProcessRequest();
    }

    // API key header adding helper
    bool AddApiKeyHeader(IHttpRequest& HttpRequest) const;
    // User-Agent header adding helper (VARCO Sound Unreal/<VersionName>/<OS>)
    void AddUserAgentHeader(IHttpRequest& HttpRequest) const;
    // Apply common headers (OPENAPI_KEY + User-Agent)
    bool AddCommonHeaders(IHttpRequest& HttpRequest) const;
    FString GetUserAgentString() const;
    FString GetPluginVersionName() const;
    FString GetOsNameForUserAgent() const;
	bool LoadFileAsBase64(const FString& FilePath, FString& OutBase64, int64 MaxFileSize = MAX_AUDIO_FILE_SIZE) const;
    FString ExtractFilenameFromContentDisposition(const FString& HeaderValue) const;
    
    // Common API response validation utility
    FApiResponseResult ValidateApiResponse(
        FHttpRequestPtr Request,
        FHttpResponsePtr Response,
        bool bWasSuccessful,
        const FString& ApiName
    ) const;
    
    // Loading status
    bool bIsLoading;
    
    // API key
    FString ApiKey;

    // User-Agent cache (avoid recalculating for each request)
    mutable bool bHasCachedUserAgent;
    mutable FString CachedUserAgent;
};
