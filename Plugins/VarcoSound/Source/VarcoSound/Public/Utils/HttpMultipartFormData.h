// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * @brief HTTP Multipart form data creation utility class
 * @details Creates Boundary for Multipart request, adds text/file fields, and final body creation.
 */
class VARCOSOUND_API FHttpMultipartFormData
{
public:
    /**
     * @brief Constructor
     * @details Creates a random Boundary string and initializes it.
     */
    FHttpMultipartFormData();

    /** @brief Destructor */
    ~FHttpMultipartFormData();

    /**
     * @brief Adds a text field to the form data.
     * @param Name form field name (Key)
     * @param Value form field value (Value)
     */
    void AddTextField(const FString& Name, const FString& Value);

    /**
     * @brief Adds file data to the form data.
     * @param Name form field name
     * @param FileData file binary data
     * @param FileName file name to send
     * @param ContentType file MIME type (default: application/octet-stream)
     */
    void AddFilePart(const FString& Name, const TArray<uint8>& FileData, const FString& FileName, const FString& ContentType = TEXT("application/octet-stream"));

    /**
     * @brief Returns the final HTTP request body (Body).
     * @details Adds closing Boundary(--Boundary--) if not present at call time.
     * @return Completed binary data array
     */
    const TArray<uint8>& GetContent();

    /**
     * @brief Returns the Content-Type string for HTTP request headers.
     * @return Example: "multipart/form-data; boundary=---------------------------12345"
     */
    FString GetContentType() const;

    /**
     * @brief Initializes the builder state. Boundary is also regenerated.
     */
    void Reset();

private:
    /** @brief Multipart boundary string (prefix '--' excluded) */
    FString Boundary;

    /** @brief Actual binary data buffer to be sent */
    TArray<uint8> FormData;

    /** @brief Whether the closing Boundary has been added */
    bool bIsFinished;

    /**
     * @brief Internal helper function to convert string to UTF-8 and add to FormData
     * @param InString String to add
     */
    void AppendString(const FString& InString);

    /**
     * @brief Adds closing Boundary to the end of the data.
     */
    void Finish();
};

