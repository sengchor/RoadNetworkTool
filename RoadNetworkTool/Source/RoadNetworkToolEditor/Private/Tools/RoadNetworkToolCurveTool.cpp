// Copyright Epic Games, Inc. All Rights Reserved.

#include "RoadNetworkToolCurveTool.h"
#include "InteractiveToolManager.h"
#include "ToolBuilderUtil.h"
#include "BaseBehaviors/ClickDragBehavior.h"
#include "RoadActor.h"
#include "RoadNetworkToolCurveToolCustomization.h"

// for raycast into World
#include "CollisionQueryParams.h"
#include "Engine/World.h"

#include "SceneManagement.h"

// localization namespace
#define LOCTEXT_NAMESPACE "URoadNetworkToolCurveTool"

/*
 * ToolBuilder
 */

UInteractiveTool* URoadNetworkToolCurveToolBuilder::BuildTool(const FToolBuilderState & SceneState) const
{
	URoadNetworkToolCurveTool* NewTool = NewObject<URoadNetworkToolCurveTool>(SceneState.ToolManager);
	NewTool->SetWorld(SceneState.World);
	return NewTool;
}


// JAH TODO: update comments
/*
 * Tool
 */

URoadNetworkToolCurveToolProperties::URoadNetworkToolCurveToolProperties()
{

}


void URoadNetworkToolCurveTool::SetWorld(UWorld* World)
{
	check(World);
	this->TargetWorld = World;
}


void URoadNetworkToolCurveTool::Setup()
{
	UInteractiveTool::Setup();

	// Add default mouse input behavior
	UClickDragInputBehavior* MouseBehavior = NewObject<UClickDragInputBehavior>();
	// We will use the shift key to indicate that we should move the second point. 
	// This call tells the Behavior to call our OnUpdateModifierState() function on mouse-down and mouse-move
	MouseBehavior->Initialize(this);
	AddInputBehavior(MouseBehavior);

	// Register the custom detail customization
	FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
	PropertyModule.RegisterCustomClassLayout(
		URoadNetworkToolCurveToolProperties::StaticClass()->GetFName(),
		FOnGetDetailCustomizationInstance::CreateStatic(&FRoadNetworkToolCurveToolCustomization::MakeInstance)
	);

	// Create the property set and register it with the Tool
	Properties = NewObject<URoadNetworkToolCurveToolProperties>(this, "Measurement");
	AddToolPropertySource(Properties);
	
	SplineActor = Cast<ARoadActor>(GetSelectedActor());

	if (!SplineActor)
	{
		SplineActor = GetFirstRoadActor(TargetWorld);
	}

	if (SplineActor)
	{
		GEditor->SelectActor(SplineActor, true, true);
		SplineActor->SetDebugSelectedPoint(false);
	}
}


FInputRayHit URoadNetworkToolCurveTool::CanBeginClickDragSequence(const FInputDeviceRay& PressPos)
{
	// we only start drag if press-down is on top of something we can raycast
	FVector Temp;
	FInputRayHit Result = FindRayHit(PressPos.WorldRay, Temp);
	return Result;
}


void URoadNetworkToolCurveTool::OnClickPress(const FInputDeviceRay& PressPos)
{
	FVector ClickLocation;
	FInputRayHit HitResult = FindRayHit(PressPos.WorldRay, ClickLocation);
	DistanceThreshold = SplineActor->DebugWidth / 2.0f;

	if (!SplineActor) { return; }

	FVector NearPointLocation;
	bHasNearSplinePoint = GetNearSplinePoint(ClickLocation, NearPointLocation, bIsSplineNodeLocation, DistanceThreshold);

	USplineComponent* NearestSpline = SplineActor->PathfindingComponent->FindNearestSplineComponent(ClickLocation);
	if (!NearestSpline)
	{
		UE_LOG(LogTemp, Warning, TEXT("No spline found at click location."));
		return;
	}

	if (!bHasNearSplinePoint)
	{
		AddSplinePointAtLocation(NearestSpline, ClickLocation);
	}
	else
	{
		// If a nearby spline point is found but is not a node, select it
		if (!bIsSplineNodeLocation)
		{
			SplineActor->SetDebugSelectedPoint(true, NearPointLocation);
			SelectedSpline = NearestSpline;
			ClosestPointIndex = SelectedSpline->FindInputKeyClosestToWorldLocation(NearPointLocation);
		}
	}
}


void URoadNetworkToolCurveTool::OnClickDrag(const FInputDeviceRay& DragPos)
{
	FVector DragLocation;
	FInputRayHit HitResult = FindRayHit(DragPos.WorldRay, DragLocation);

	if (!SelectedSpline) { return; }

	// Set DragLocation's Z value
	DragLocation.Z = SelectedSpline->GetLocationAtSplineInputKey(0, ESplineCoordinateSpace::World).Z;

	if (SplineActor)
	{
		UpdateSplinePointPosition(DragLocation);
	}
}

void URoadNetworkToolCurveTool::OnClickRelease(const FInputDeviceRay& ReleasePos)
{
	bHasNearSplinePoint = false;
	bIsSplineNodeLocation = false;

	if (SplineActor && SelectedSpline)
	{
		SplineActor->UpdateSplineComponent(SelectedSpline);
	}
}

void URoadNetworkToolCurveTool::OnPropertyModified(UObject* PropertySet, FProperty* Property)
{
	// if the user updated any of the property fields, update the distance
}

void URoadNetworkToolCurveTool::AddSplinePointAtLocation(USplineComponent* NearestSpline, const FVector& ClickLocation)
{
	FVector ClosestSplineLocation = NearestSpline->FindLocationClosestToWorldLocation(ClickLocation, ESplineCoordinateSpace::World);
	float Distance = FVector::Dist(ClickLocation, ClosestSplineLocation);

	if (Distance <= DistanceThreshold)
	{
		float InputKey = NearestSpline->FindInputKeyClosestToWorldLocation(ClosestSplineLocation);
		int32 StartIndex = 0;
		int32 EndIndex = NearestSpline->GetNumberOfSplinePoints() - 1;

		// Ensure that the input key is not at the start or end of the spline
		if (InputKey > StartIndex && InputKey < EndIndex)
		{
			int32 NextPointIndex = FMath::CeilToInt(InputKey);
			NearestSpline->AddSplinePointAtIndex(ClosestSplineLocation, NextPointIndex, ESplineCoordinateSpace::World, true);
			NearestSpline->UpdateSpline();

			// Update new point on spline
			SplineActor->SetDebugSelectedPoint(true, ClosestSplineLocation);
			SelectedSpline = NearestSpline;
			ClosestPointIndex = NextPointIndex;

			bHasNearSplinePoint = true;
		}
	}
	else
	{
		SplineActor->SetDebugSelectedPoint(false, ClosestSplineLocation);
	}
}

void URoadNetworkToolCurveTool::UpdateSplinePointPosition(const FVector& DragLocation)
{
	if (!SelectedSpline) { return; }

	if (bHasNearSplinePoint && !bIsSplineNodeLocation)
	{
		SplineActor->SetDebugSelectedPoint(true, DragLocation);
		SelectedSpline->SetLocationAtSplinePoint(ClosestPointIndex, DragLocation, ESplineCoordinateSpace::World, true);
	}
	else
	{
		FVector ClosestSplineLocation = SelectedSpline->FindLocationClosestToWorldLocation(DragLocation, ESplineCoordinateSpace::World);
		SplineActor->SetDebugSelectedPoint(false, ClosestSplineLocation);
	}
}