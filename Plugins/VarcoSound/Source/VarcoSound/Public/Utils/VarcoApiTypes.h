// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "JsonObjectConverter.h"
#include "VarcoApiTypes.generated.h"

/**
 * @brief 기본 API 응답 구조체
 */
USTRUCT()
struct FVarcoBaseResponse
{
    GENERATED_BODY()

    // Success or error message is initially determined by HTTP status code, but in case of an error message included in the API body, it is prepared.
    // In case of an error message included in the API body, it is prepared.
    UPROPERTY()
    FString message;
};

/**
 * @brief Audio generation request (Text-to-Audio)
 */
USTRUCT()
struct FVarcoGenRequest
{
    GENERATED_BODY()

    UPROPERTY()
    FString version = TEXT("v1");

    UPROPERTY()
    FString prompt;

    UPROPERTY()
    int32 num_sample = 1;
};

/**
 * @brief Single audio response item
 */
USTRUCT()
struct FVarcoAudioItem
{
    GENERATED_BODY()

    UPROPERTY()
    FString audio; // Base64 Encoded Audio
};

/**
 * @brief Audio generation response (Array of items)
 * @note 최상위 JSON이 배열인 경우 TArray<FVarcoAudioItem>으로 파싱해야 합니다.
 */

/**
 * @brief Looping request
 */
USTRUCT()
struct FVarcoLoopingRequest
{
    GENERATED_BODY()

    UPROPERTY()
    FString source; // Base64 Encoded Audio

    // Internal structure definition for optional parameters is also possible, but if needed, it can be added.
    // Currently, it is simply processed or added if needed.
};

/**
 * @brief Looping response
 * @details {"audio": "base64..."} format
 */
USTRUCT()
struct FVarcoLoopingResponse
{
    GENERATED_BODY()

    UPROPERTY()
    FString audio;
};

/**
 * @brief Variation generation request (Variation)
 */
USTRUCT()
struct FVarcoVariationRequest
{
    GENERATED_BODY()

    UPROPERTY()
    FString source; // Base64 Encoded Audio

    UPROPERTY()
    int32 num_sample = 1;

    UPROPERTY()
    float strength = 0.5f;
};

/**
 * @brief Mono -> Stereo conversion request
 */
USTRUCT()
struct FVarcoMono2StereoRequest
{
    GENERATED_BODY()

    UPROPERTY()
    FString source; // Base64 Encoded Audio
};

/**
 * @brief Mono -> Stereo response
 */
USTRUCT()
struct FVarcoMono2StereoResponse
{
    GENERATED_BODY()

    UPROPERTY()
    FString audio;
};

/**
 * @brief Monster voice conversion request (Monster Voice)
 */
USTRUCT()
struct FVarcoMonsterVoiceRequest
{
    GENERATED_BODY()

    UPROPERTY()
    FString source; // Base64 Source Audio

    UPROPERTY()
    FString reference; // Base64 Target Voice Reference

    UPROPERTY()
    float ratio = 1.0f;

    UPROPERTY()
    bool enhance = true;
};

/**
 * @brief Monster voice conversion response
 */
USTRUCT()
struct FVarcoMonsterVoiceResponse
{
    GENERATED_BODY()

    UPROPERTY()
    FString audio;
};

/**
 * @brief Prompt booster request
 */
USTRUCT()
struct FVarcoPromptBoosterRequest
{
    GENERATED_BODY()

    UPROPERTY()
    FString prompt;
};

/**
 * @brief Image -> Sound effect prompt generation request (Image2Sfx)
 */
USTRUCT()
struct FVarcoImage2SfxRequest
{
    GENERATED_BODY()

    UPROPERTY()
    FString prompt;

    UPROPERTY()
    FString image; // Base64 Encoded Image
};

/**
 * @brief Image -> Sound effect prompt response
 * @note The response structure may be variable or not a simple string list, so it is received as FJsonObject or defined as needed.
 *       It may be variable or not a simple string list, so it is received as FJsonObject or defined as needed.
 *       Currently, the result string is simply passed.
 */

/**
 * @brief Background music asynchronous generation request (Image + Prompt)
 */
USTRUCT()
struct FBgmGenerateMusicRequest
{
    GENERATED_BODY()

    UPROPERTY()
    FString image;

    UPROPERTY()
    FString prompt;
};

/**
 * @brief Background music initial generation response (task_id)
 */
USTRUCT()
struct FBgmGenerateMusicResponse
{
    GENERATED_BODY()

    UPROPERTY()
    FString task_id;
};

/**
 * @brief Background music track information
 */
USTRUCT()
struct FBgmMusicInfo
{
    GENERATED_BODY()

    UPROPERTY()
    FString id;

    UPROPERTY()
    FString title;

    UPROPERTY()
    FString created_at;
};

/**
 * @brief Background music status response
 */
USTRUCT()
struct FBgmMusicStatusResponse
{
    GENERATED_BODY()

    UPROPERTY()
    FString id;

    UPROPERTY()
    FString status;

    UPROPERTY()
    FString created_at;

    UPROPERTY()
    FString updated_at;

    UPROPERTY()
    TArray<FBgmMusicInfo> musics;
};

