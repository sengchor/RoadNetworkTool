// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "InteractiveToolBuilder.h"
#include "BaseTools/ClickDragTool.h"
#include "RoadNetworkToolBase.h"
#include "RoadNetworkToolCurveTool.generated.h"


/**
 * Builder for URoadNetworkToolCurveTool
 */
UCLASS()
class ROADNETWORKTOOLEDITOR_API URoadNetworkToolCurveToolBuilder : public UInteractiveToolBuilder
{
	GENERATED_BODY()

public:
	virtual bool CanBuildTool(const FToolBuilderState& SceneState) const override { return true; }
	virtual UInteractiveTool* BuildTool(const FToolBuilderState& SceneState) const override;
};


/**
 * Property set for the URoadNetworkToolCurveTool
 */
UCLASS(Transient)
class ROADNETWORKTOOLEDITOR_API URoadNetworkToolCurveToolProperties : public UInteractiveToolPropertySet
{
	GENERATED_BODY()

public:
	URoadNetworkToolCurveToolProperties();

};



/**
 * URoadNetworkToolCurveTool is an example Tool that allows the user to measure the 
 * distance between two points. The first point is set by click-dragging the mouse, and
 * the second point is set by shift-click-dragging the mouse.
 */
UCLASS()
class ROADNETWORKTOOLEDITOR_API URoadNetworkToolCurveTool : public URoadNetworkToolBase, public IClickDragBehaviorTarget
{
	GENERATED_BODY()

public:
	virtual void SetWorld(UWorld* World);

	/** UInteractiveTool overrides */
	virtual void Setup() override;
	virtual void OnPropertyModified(UObject* PropertySet, FProperty* Property) override;

	/** IClickDragBehaviorTarget implementation */
	virtual FInputRayHit CanBeginClickDragSequence(const FInputDeviceRay& PressPos) override;
	virtual void OnClickPress(const FInputDeviceRay& PressPos) override;
	virtual void OnClickDrag(const FInputDeviceRay& DragPos) override;
	// these are not used in this Tool
	virtual void OnClickRelease(const FInputDeviceRay& ReleasePos) override;
	virtual void OnTerminateDragSequence() override {};

protected:
	/** Properties of the tool are stored here */
	UPROPERTY()
	TObjectPtr<URoadNetworkToolCurveToolProperties> Properties;

protected:
	int32 ClosestPointIndex;
	float DistanceThreshold;
	bool bHasNearSplinePoint;
	bool bIsSplineNodeLocation;

	void AddSplinePointAtLocation(USplineComponent* NearestSpline, const FVector& ClickLocation);
	void UpdateSplinePointPosition(const FVector& DragLocation);

public:
	USplineComponent* SelectedSpline = nullptr;
};
