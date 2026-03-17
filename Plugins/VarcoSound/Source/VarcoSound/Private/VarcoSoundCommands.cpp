// Copyright Epic Games, Inc. All Rights Reserved.

#include "VarcoSoundCommands.h"

#define LOCTEXT_NAMESPACE "FVarcoSoundModule"

void FVarcoSoundCommands::RegisterCommands()
{
	UI_COMMAND(OpenPluginWindow, "VarcoSound", "Generate sound effects using AI", EUserInterfaceActionType::Button, FInputChord(EModifierKey::Control | EModifierKey::Alt, EKeys::P));
}

#undef LOCTEXT_NAMESPACE
