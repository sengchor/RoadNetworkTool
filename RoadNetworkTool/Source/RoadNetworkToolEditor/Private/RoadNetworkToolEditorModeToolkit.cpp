// Copyright Epic Games, Inc. All Rights Reserved.

#include "RoadNetworkToolEditorModeToolkit.h"
#include "RoadNetworkToolEditorMode.h"
#include "Engine/Selection.h"

#include "Modules/ModuleManager.h"
#include "PropertyEditorModule.h"
#include "IDetailsView.h"
#include "EditorModeManager.h"

#define LOCTEXT_NAMESPACE "RoadNetworkToolEditorModeToolkit"

FRoadNetworkToolEditorModeToolkit::FRoadNetworkToolEditorModeToolkit()
{
}

void FRoadNetworkToolEditorModeToolkit::Init(const TSharedPtr<IToolkitHost>& InitToolkitHost, TWeakObjectPtr<UEdMode> InOwningMode)
{
	FModeToolkit::Init(InitToolkitHost, InOwningMode);
}

void FRoadNetworkToolEditorModeToolkit::GetToolPaletteNames(TArray<FName>& PaletteNames) const
{
	PaletteNames.Add(NAME_Default);
}


FName FRoadNetworkToolEditorModeToolkit::GetToolkitFName() const
{
	return FName("RoadNetworkToolEditorMode");
}

FText FRoadNetworkToolEditorModeToolkit::GetBaseToolkitName() const
{
	return LOCTEXT("DisplayName", "RoadNetworkToolEditorMode Toolkit");
}

#undef LOCTEXT_NAMESPACE
