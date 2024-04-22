// Copyright Epic Games, Inc. All Rights Reserved.

#include "C2M_v2Commands.h"

#define LOCTEXT_NAMESPACE "FC2M_v2Module"

void FC2M_v2Commands::RegisterCommands()
{
	UI_COMMAND(OpenPluginWindow, "C2M_v2", "Bring up C2M_v2 window", EUserInterfaceActionType::Button, FInputChord());
}

#undef LOCTEXT_NAMESPACE
