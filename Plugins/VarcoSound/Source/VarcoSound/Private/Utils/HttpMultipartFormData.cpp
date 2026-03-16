// Copyright Epic Games, Inc. All Rights Reserved.

#include "Utils/HttpMultipartFormData.h"
#include "Misc/Guid.h"

FHttpMultipartFormData::FHttpMultipartFormData()
    : bIsFinished(false)
{
    Reset();
}

FHttpMultipartFormData::~FHttpMultipartFormData()
{
}

void FHttpMultipartFormData::Reset()
{
    FormData.Empty();
    bIsFinished = false;
    // 고유한 Boundary 생성 (---------------------------UUID)
    // 하이픈 개수는 임의로 지정 가능하지만, 일반적으로 브라우저 스타일을 따름
    Boundary = FString::Printf(TEXT("---------------------------%s"), *FGuid::NewGuid().ToString());
}

void FHttpMultipartFormData::AddTextField(const FString& Name, const FString& Value)
{
    if (bIsFinished)
    {
        UE_LOG(LogTemp, Warning, TEXT("FHttpMultipartFormData: Cannot add field '%s' after content is finished."), *Name);
        return;
    }

    // --Boundary\r\n
    AppendString(FString::Printf(TEXT("--%s\r\n"), *Boundary));
    
    // Content-Disposition: form-data; name="Name"\r\n\r\n
    AppendString(FString::Printf(TEXT("Content-Disposition: form-data; name=\"%s\"\r\n\r\n"), *Name));
    
    // Value\r\n
    AppendString(FString::Printf(TEXT("%s\r\n"), *Value));
}

void FHttpMultipartFormData::AddFilePart(const FString& Name, const TArray<uint8>& FileData, const FString& FileName, const FString& ContentType)
{
    if (bIsFinished)
    {
        UE_LOG(LogTemp, Warning, TEXT("FHttpMultipartFormData: Cannot add file '%s' after content is finished."), *Name);
        return;
    }

    // --Boundary\r\n
    AppendString(FString::Printf(TEXT("--%s\r\n"), *Boundary));
    
    // Content-Disposition: form-data; name="Name"; filename="FileName"\r\n
    AppendString(FString::Printf(TEXT("Content-Disposition: form-data; name=\"%s\"; filename=\"%s\"\r\n"), *Name, *FileName));
    
    // Content-Type: ContentType\r\n\r\n
    AppendString(FString::Printf(TEXT("Content-Type: %s\r\n\r\n"), *ContentType));
    
    // Binary Data
    FormData.Append(FileData);
    
    // \r\n
    AppendString(TEXT("\r\n"));
}

const TArray<uint8>& FHttpMultipartFormData::GetContent()
{
    if (!bIsFinished)
    {
        Finish();
    }
    return FormData;
}

FString FHttpMultipartFormData::GetContentType() const
{
    return FString::Printf(TEXT("multipart/form-data; boundary=%s"), *Boundary);
}

void FHttpMultipartFormData::AppendString(const FString& InString)
{
    FTCHARToUTF8 Utf8Converter(*InString);
    if (Utf8Converter.Length() > 0)
    {
        FormData.Append((uint8*)Utf8Converter.Get(), Utf8Converter.Length());
    }
}

void FHttpMultipartFormData::Finish()
{
    if (bIsFinished)
    {
        return;
    }

    // --Boundary--\r\n
    AppendString(FString::Printf(TEXT("--%s--\r\n"), *Boundary));
    bIsFinished = true;
}

