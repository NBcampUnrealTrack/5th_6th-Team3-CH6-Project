// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Framework/Commands/Commands.h"
#include "VarcoSoundStyle.h"

class FVarcoSoundCommands : public TCommands<FVarcoSoundCommands>
{
public:

	FVarcoSoundCommands()
		: TCommands<FVarcoSoundCommands>(TEXT("VarcoSound"), NSLOCTEXT("Contexts", "VarcoSound", "VarcoSound Plugin"), NAME_None, FVarcoSoundStyle::GetStyleSetName())
	{
	}

	// TCommands<> interface
	virtual void RegisterCommands() override;

public:
	TSharedPtr< FUICommandInfo > OpenPluginWindow;
};