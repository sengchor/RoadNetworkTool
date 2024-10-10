// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Tools/UEdMode.h"
#include "RoadNetworkToolEditorMode.generated.h"

/**
 * This class provides an example of how to extend a UEdMode to add some simple tools
 * using the InteractiveTools framework. The various UEdMode input event handlers (see UEdMode.h)
 * forward events to a UEdModeInteractiveToolsContext instance, which
 * has all the logic for interacting with the InputRouter, ToolManager, etc.
 * The functions provided here are the minimum to get started inserting some custom behavior.
 * Take a look at the UEdMode markup for more extensibility options.
 */
UCLASS()
class URoadNetworkToolEditorMode : public UEdMode
{
	GENERATED_BODY()

public:
	const static FEditorModeID EM_RoadNetworkToolEditorModeId;

	static FString LineToolName;
	static FString CurveToolName;

	URoadNetworkToolEditorMode();
	virtual ~URoadNetworkToolEditorMode();

	/** UEdMode interface */
	virtual void Enter() override;
	virtual void Exit() override;
	virtual void ActorSelectionChangeNotify() override;
	virtual void CreateToolkit() override;
	virtual TMap<FName, TArray<TSharedPtr<FUICommandInfo>>> GetModeCommands() const override;

	/** Custom functions to manage selection behavior */
	virtual bool IsSelectionAllowed(AActor* InActor, bool bInSelected) const override;
};
