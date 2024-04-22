// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Framework/Commands/Commands.h"
#include "C2M_v2Style.h"

class FC2M_v2Commands : public TCommands<FC2M_v2Commands>
{
public:

	FC2M_v2Commands()
		: TCommands<FC2M_v2Commands>(TEXT("C2M_v2"), NSLOCTEXT("Contexts", "C2M_v2", "C2M_v2 Plugin"), NAME_None, FC2M_v2Style::GetStyleSetName())
	{
	}

	// TCommands<> interface
	virtual void RegisterCommands() override;

public:
	TSharedPtr< FUICommandInfo > OpenPluginWindow;
};