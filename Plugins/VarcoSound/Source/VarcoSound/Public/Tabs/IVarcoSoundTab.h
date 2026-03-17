// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * Common interface for all VarcoSound tab widgets
 * Ensures consistent behavior across different tab types
 */
class IVarcoSoundTab
{
public:
	virtual ~IVarcoSoundTab() = default;

	/**
	 * Pause all audio playback in this tab
	 * Called when switching between tabs to prevent multiple sounds playing simultaneously
	 */
	virtual void PauseAllPlayback() = 0;

	/**
	 * Set the API key for this tab's API client
	 * @param InApiKey The API key string
	 */
	virtual void SetApiKey(const FString& InApiKey) = 0;
};

