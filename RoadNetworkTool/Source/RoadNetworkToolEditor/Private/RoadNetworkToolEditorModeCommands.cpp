// Copyright Epic Games, Inc. All Rights Reserved.

#include "RoadNetworkToolEditorModeCommands.h"
#include "RoadNetworkToolEditorMode.h"
#include "EditorStyleSet.h"

#define LOCTEXT_NAMESPACE "RoadNetworkToolEditorModeCommands"

FRoadNetworkToolEditorModeCommands::FRoadNetworkToolEditorModeCommands()
	: TCommands<FRoadNetworkToolEditorModeCommands>("RoadNetworkToolEditorMode",
		NSLOCTEXT("RoadNetworkToolEditorMode", "RoadNetworkToolEditorModeCommands", "RoadNetworkTool Editor Mode"),
		NAME_None,
		FEditorStyle::GetStyleSetName())
{
}

void FRoadNetworkToolEditorModeCommands::RegisterCommands()
{
	TArray <TSharedPtr<FUICommandInfo>>& ToolCommands = Commands.FindOrAdd(NAME_Default);

	UI_COMMAND(LineTool, "Line Creator", "Click to set the origin, then click to extend the line.", EUserInterfaceActionType::ToggleButton, FInputChord());
	ToolCommands.Add(LineTool);

	UI_COMMAND(CurveTool, "Curve Editor", "Click on the line to create a curve. Drag to adjust the curve point.", EUserInterfaceActionType::ToggleButton, FInputChord());
	ToolCommands.Add(CurveTool);
}

TMap<FName, TArray<TSharedPtr<FUICommandInfo>>> FRoadNetworkToolEditorModeCommands::GetCommands()
{
	return FRoadNetworkToolEditorModeCommands::Get().Commands;
}

#undef LOCTEXT_NAMESPACE
