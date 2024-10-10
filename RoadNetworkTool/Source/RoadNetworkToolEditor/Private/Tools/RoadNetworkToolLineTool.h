// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "InteractiveToolBuilder.h"
#include "BaseTools/ClickDragTool.h"
#include "Components/SplineComponent.h"
#include "RoadNetworkTool/Public/RoadActor.h"
#include "RoadNetworkToolBase.h"
#include "RoadNetworkToolLineTool.generated.h"


/**
 * Builder for URoadNetworkToolInteractiveTool
 */
UCLASS()
class ROADNETWORKTOOLEDITOR_API URoadNetworkToolLineToolBuilder : public UInteractiveToolBuilder
{
	GENERATED_BODY()

public:
	virtual bool CanBuildTool(const FToolBuilderState& SceneState) const override { return true; }
	virtual UInteractiveTool* BuildTool(const FToolBuilderState& SceneState) const override;
};


/**
 * Property set for the URoadNetworkToolInteractiveTool
 */
UCLASS(Config = EditorPerProjectUserSettings)
class ROADNETWORKTOOLEDITOR_API URoadNetworkToolLineToolProperties : public UInteractiveToolPropertySet
{
	GENERATED_BODY()

public:
	URoadNetworkToolLineToolProperties();
	void SaveConfiguration() const;

	UPROPERTY(EditAnywhere, Category = "New Road Network")
	bool EnableDebugLine = true;

	UPROPERTY(EditAnywhere, Category = "New Road Network", meta = (ClampMin = "0.0"))
	float Width = 300.0f;

	UPROPERTY(EditAnywhere, Category = "New Road Network", meta = (ClampMin = "0.0"))
	float Thickness = 10.0f;

	UPROPERTY(EditAnywhere, Category = "New Road Network")
	bool CheckLineIntersection = true;

	UPROPERTY(EditAnywhere, Category = "New Road Network", meta = (ClampMin = "0.0"))
	float SnapThreshold = 300.f;
};


/**
 * URoadNetworkToolInteractiveTool is an example Tool that allows the user to measure the 
 * distance between two points. The first point is set by click-dragging the mouse, and
 * the second point is set by shift-click-dragging the mouse.
 */
UCLASS()
class ROADNETWORKTOOLEDITOR_API URoadNetworkToolLineTool : public URoadNetworkToolBase, public IClickDragBehaviorTarget
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
	virtual void OnClickDrag(const FInputDeviceRay& DragPos) override {}
	// these are not used in this Tool
	virtual void OnClickRelease(const FInputDeviceRay& ReleasePos) override {}
	virtual void OnTerminateDragSequence() override {}

protected:
	/** Properties of the tool are stored here */
	UPROPERTY()
	TObjectPtr<URoadNetworkToolLineToolProperties> Properties;

protected:
	enum class ESplineCreationState
	{
		SettingOrigin,
		SettingEndPoint,
		Extending
	};

	/*UWorld* TargetWorld = nullptr;
	ARoadActor* SplineActor = nullptr;*/
	USplineComponent* CurrentSplineComponent = nullptr;

	/** Start and end points for the spline */
	FVector OriginPoint;
	FVector EndPoint;

	ESplineCreationState CurrentSplineState = ESplineCreationState::SettingOrigin;

	void UpdateDebugOriginPoint(bool bEnable, const FVector& NewOriginPoint);
	void InitializeSplineActor();
	
	void CreateSpline();
	void UpdateSelectedActor();
	void SetOriginPoint(const FVector& ClickLocation);
	void SetEndPoint(const FVector& ClickLocation);
	void SetExtendPoint(const FVector& ClickLocation);
	bool ValidateSplineConditions();

	bool ArePointsOnSameSpline(const FVector& Point1, const FVector& Point2);
	bool IsLineIntersectingSpline(const FVector& LineStart, const FVector& LineEnd);
	bool DoLinesIntersect(const FVector& A1, const FVector& A2, const FVector& B1, const FVector& B2);
	void ApplyProperties(ARoadActor* RoadActor);
};