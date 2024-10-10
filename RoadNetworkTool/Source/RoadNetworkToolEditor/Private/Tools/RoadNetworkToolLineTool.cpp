// Copyright Epic Games, Inc. All Rights Reserved.

#include "RoadNetworkToolLineTool.h"
#include "DrawDebugHelpers.h"
#include "InteractiveToolManager.h"
#include "BaseBehaviors/ClickDragBehavior.h"
#include "Components/SplineComponent.h"
#include "GameFramework/Actor.h"
#include "RoadActor.h"
#include "RoadNetworkTool/Public/RoadActor.h"
#include "RoadNetworkToolLineToolCustomization.h"

// for raycast into World
#include "CollisionQueryParams.h"
#include "Engine/World.h"

#include "SceneManagement.h"

// localization namespace
#define LOCTEXT_NAMESPACE "URoadNetworkToolLineTool"

/*
 * ToolBuilder
 */

UInteractiveTool* URoadNetworkToolLineToolBuilder::BuildTool(const FToolBuilderState& SceneState) const
{
	URoadNetworkToolLineTool* NewTool = NewObject<URoadNetworkToolLineTool>(SceneState.ToolManager);
	NewTool->SetWorld(SceneState.World);
	return NewTool;
}

URoadNetworkToolLineToolProperties::URoadNetworkToolLineToolProperties()
{
	if (GConfig)
	{
		// Load values from the config file
		GConfig->GetFloat(TEXT("/Script/RoadNetworkTool.URoadNetworkToolLineToolProperties"), TEXT("Width"), Width, GEditorPerProjectIni);
		GConfig->GetFloat(TEXT("/Script/RoadNetworkTool.URoadNetworkToolLineToolProperties"), TEXT("Thickness"), Thickness, GEditorPerProjectIni);
		GConfig->GetBool(TEXT("/Script/RoadNetworkTool.URoadNetworkToolLineToolProperties"), TEXT("EnableDebugLine"), EnableDebugLine, GEditorPerProjectIni);
		GConfig->GetBool(TEXT("/Script/RoadNetworkTool.URoadNetworkToolLineToolProperties"), TEXT("CheckLineIntersection"), CheckLineIntersection, GEditorPerProjectIni);
		GConfig->GetFloat(TEXT("/Script/RoadNetworkTool.URoadNetworkToolLineToolProperties"), TEXT("SnapThreshold"), SnapThreshold, GEditorPerProjectIni);
	}
}

void URoadNetworkToolLineToolProperties::SaveConfiguration() const
{
	if (GConfig)
	{
		GConfig->SetFloat(TEXT("/Script/RoadNetworkTool.URoadNetworkToolLineToolProperties"), TEXT("Width"), Width, GEditorPerProjectIni);
		GConfig->SetFloat(TEXT("/Script/RoadNetworkTool.URoadNetworkToolLineToolProperties"), TEXT("Thickness"), Thickness, GEditorPerProjectIni);
		GConfig->SetBool(TEXT("/Script/RoadNetworkTool.URoadNetworkToolLineToolProperties"), TEXT("EnableDebugLine"), EnableDebugLine, GEditorPerProjectIni);
		GConfig->SetBool(TEXT("/Script/RoadNetworkTool.URoadNetworkToolLineToolProperties"), TEXT("CheckLineIntersection"), CheckLineIntersection, GEditorPerProjectIni);
		GConfig->SetFloat(TEXT("/Script/RoadNetworkTool.URoadNetworkToolLineToolProperties"), TEXT("SnapThreshold"), SnapThreshold, GEditorPerProjectIni);

		GConfig->Flush(false, GEditorPerProjectIni);
	}
}

void URoadNetworkToolLineTool::SetWorld(UWorld* World)
{
	check(World);
	this->TargetWorld = World;
}

void URoadNetworkToolLineTool::Setup()
{
	UInteractiveTool::Setup();

	// Setup click behavior
	UClickDragInputBehavior* ClickBehavior = NewObject<UClickDragInputBehavior>();
	ClickBehavior->Initialize(this);
	AddInputBehavior(ClickBehavior);

	// Create the property set and register it with the Tool
	Properties = NewObject<URoadNetworkToolLineToolProperties>(this);
	AddToolPropertySource(Properties);

	// Register the custom detail customization
	FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
	PropertyModule.RegisterCustomClassLayout(
		URoadNetworkToolLineToolProperties::StaticClass()->GetFName(),
		FOnGetDetailCustomizationInstance::CreateStatic(&FRoadNetworkToolLineToolCustomization::MakeInstance)
	);

	InitializeSplineActor();
}

void URoadNetworkToolLineTool::InitializeSplineActor()
{
	SplineActor = Cast<ARoadActor>(GetSelectedActor());

	if (!SplineActor)
	{
		SplineActor = GetFirstRoadActor(TargetWorld);
	}

	if (SplineActor)
	{
		GEditor->SelectActor(SplineActor, true, true);
		SplineActor->SetDebugSelectedPoint(false);

		if (SplineActor->RoadWidth != 0)
		{
			Properties->Width = SplineActor->RoadWidth;
		}
		Properties->SnapThreshold = Properties->Width;

		ApplyProperties(SplineActor);

		CurrentSplineState = ESplineCreationState::Extending;

		SplineActor->InitializeQuadtree();
	}
}

FInputRayHit URoadNetworkToolLineTool::CanBeginClickDragSequence(const FInputDeviceRay& PressPos)
{
	// we only start drag if press-down is on top of something we can raycast
	FVector Temp;
	FInputRayHit Result = FindRayHit(PressPos.WorldRay, Temp);
	return Result;
}

void URoadNetworkToolLineTool::OnClickPress(const FInputDeviceRay& PressPos)
{
	FVector ClickLocation;
	FInputRayHit HitResult = FindRayHit(PressPos.WorldRay, ClickLocation);

	UpdateSelectedActor();
	
	if (CurrentSplineState == ESplineCreationState::SettingOrigin)
	{
		//UE_LOG(LogTemp, Warning, TEXT("SettingOrigin"));
		SetOriginPoint(ClickLocation);
	}
	else if (CurrentSplineState == ESplineCreationState::SettingEndPoint)
	{
		//UE_LOG(LogTemp, Warning, TEXT("SettingEndPoint"));
		SetEndPoint(ClickLocation);
	}
	else if (CurrentSplineState == ESplineCreationState::Extending)
	{
		//UE_LOG(LogTemp, Warning, TEXT("Extending"));
		SetExtendPoint(ClickLocation);
	}
}

// Helper function for handling actor selection changes
void URoadNetworkToolLineTool::UpdateSelectedActor()
{
	ARoadActor* NewSelectedActor = Cast<ARoadActor>(GetSelectedActor());

	// Check if no actor is selected, then get the first road actor from the world
	if (!NewSelectedActor)
	{
		NewSelectedActor = GetFirstRoadActor(TargetWorld);
	}

	// Only update if the new actor is valid and different from the current one
	if (NewSelectedActor && NewSelectedActor != SplineActor)
	{
		SplineActor = NewSelectedActor;
		CurrentSplineComponent = nullptr;
		CurrentSplineState = ESplineCreationState::Extending;

		if (GEditor)
		{
			// Deselect all actors and select the new spline actor
			GEditor->SelectNone(true, true, false);
			GEditor->SelectActor(SplineActor, true, true);
		}
	}
}

void URoadNetworkToolLineTool::SetOriginPoint(const FVector& ClickLocation)
{
	OriginPoint = ClickLocation;
	DrawDebugSphereEditor(OriginPoint);
	UpdateDebugOriginPoint(true, OriginPoint);
	CurrentSplineState = ESplineCreationState::SettingEndPoint;
}

void URoadNetworkToolLineTool::SetEndPoint(const FVector& ClickLocation)
{
	// Set the end point and create the initial spline
	FVector NearPointLocation;
	bool bIsSplineNodeLocation;

	if (GetNearSplinePoint(ClickLocation, NearPointLocation, bIsSplineNodeLocation, Properties->SnapThreshold))
	{
		EndPoint = NearPointLocation;

		if (!bIsSplineNodeLocation)
		{
			DrawDebugSphereEditor(EndPoint);
			UpdateDebugOriginPoint(false, OriginPoint);
			UE_LOG(LogTemp, Warning, TEXT("Clicked point is not a spline node. Spline not created."));
			CurrentSplineState = ESplineCreationState::Extending;
			return;
		}
	}
	else
	{
		EndPoint = ClickLocation;
	}

	DrawDebugSphereEditor(EndPoint);
	UpdateDebugOriginPoint(false, OriginPoint);

	if (!ValidateSplineConditions())
	{
		return;
	}

	CreateSpline();
	CurrentSplineState = ESplineCreationState::Extending;
}

void URoadNetworkToolLineTool::SetExtendPoint(const FVector& ClickLocation)
{
	FVector NearPointLocation;
	bool bIsSplineNodeLocation;
	
	// Check if the click was near an existing spline point
	if (GetNearSplinePoint(ClickLocation, NearPointLocation, bIsSplineNodeLocation, Properties->SnapThreshold))
	{
		// Reselect the origin point and update the CurrentSplineState to SettingEndPoint
		OriginPoint = NearPointLocation;
		UpdateDebugOriginPoint(true, OriginPoint);
		CurrentSplineState = ESplineCreationState::SettingEndPoint;

		if (!bIsSplineNodeLocation)
		{
			DrawDebugSphereEditor(OriginPoint);
			UpdateDebugOriginPoint(false, OriginPoint);
			UE_LOG(LogTemp, Warning, TEXT("Clicked point is not a spline node. Continuing in Extending state."));
			CurrentSplineState = ESplineCreationState::Extending;
			return;
		}
	}
}

bool URoadNetworkToolLineTool::ValidateSplineConditions()
{

	if (ArePointsOnSameSpline(OriginPoint, EndPoint))
	{
		UE_LOG(LogTemp, Warning, TEXT("Origin and End Points are on the same spline. Spline not created."));
		OriginPoint = EndPoint;
		CurrentSplineState = ESplineCreationState::Extending;
		return false;
	}

	if (OriginPoint.Equals(EndPoint, 1.0f))
	{
		UE_LOG(LogTemp, Warning, TEXT("Origin and End Point are the same. Spline not created."));
		OriginPoint = EndPoint;
		CurrentSplineState = ESplineCreationState::Extending;
		return false;
	}

	// Check for intersection
	if (Properties->CheckLineIntersection && IsLineIntersectingSpline(OriginPoint, EndPoint))
	{
		UE_LOG(LogTemp, Warning, TEXT("Line intersects with an existing spline. Spline not created."));
		OriginPoint = EndPoint;
		CurrentSplineState = ESplineCreationState::Extending;
		return false;
	}

	return true;
}

void URoadNetworkToolLineTool::UpdateDebugOriginPoint(bool bEnable, const FVector& NewOriginPoint)
{
	if (SplineActor)
	{
		SplineActor->SetDebugSelectedPoint(bEnable, NewOriginPoint);
	}
}

void URoadNetworkToolLineTool::CreateSpline()
{
	if (!TargetWorld) return;

	if (!SplineActor)
	{
		SplineActor = TargetWorld->SpawnActor<ARoadActor>();
		// Generate a unique name for the new RoadActor
		FName UniqueName = MakeUniqueObjectName(TargetWorld, ARoadActor::StaticClass(), FName(TEXT("RoadNetwork")));
		SplineActor->SetActorLabel(UniqueName.ToString());

		InitializeSplineActor();
	}

	USplineComponent* SplineComponent = NewObject<USplineComponent>(SplineActor);
	SplineComponent->SetupAttachment(SplineActor->SplineRootComponent);
	SplineComponent->RegisterComponentWithWorld(TargetWorld);

	SplineComponent->ClearSplinePoints();

	// Add the points to the spline
	SplineComponent->AddSplinePoint(OriginPoint, ESplineCoordinateSpace::World);
	SplineComponent->AddSplinePoint(EndPoint, ESplineCoordinateSpace::World);

	SplineComponent->UpdateSpline();

	if (GEditor)
	{
		GEditor->SelectNone(true, true, false);
		GEditor->SelectActor(SplineActor, true, true);
	}

	OriginPoint = FVector::ZeroVector;
	CurrentSplineComponent = SplineComponent;
	SplineActor->AddSplineComponent(CurrentSplineComponent);
	ApplyProperties(SplineActor);
}

bool URoadNetworkToolLineTool::ArePointsOnSameSpline(const FVector& Point1, const FVector& Point2)
{
	if (!SplineActor)
	{
		return false;
	}

	TArray<USplineComponent*> NearbySplines;
	SplineActor->PathfindingComponent->FindSplinesInArea(Point1, SplineActor->PathfindingComponent->DefaultSearchRadius, NearbySplines);

	for (USplineComponent* SplineComponent : NearbySplines)
	{
		int32 Point1Index = INDEX_NONE;
		int32 Point2Index = INDEX_NONE;

		// Iterate through all spline points in the component
		for (int32 i = 0; i < SplineComponent->GetNumberOfSplinePoints(); ++i)
		{
			FVector SplinePoint = SplineComponent->GetLocationAtSplinePoint(i, ESplineCoordinateSpace::World);
			if (Point1Index == INDEX_NONE && FVector::Dist(Point1, SplinePoint) < Properties->SnapThreshold)
			{
				Point1Index = i;
			}
			if (Point2Index == INDEX_NONE && FVector::Dist(Point2, SplinePoint) < Properties->SnapThreshold)
			{
				Point2Index = i;
			}

			// If both points are found and belong to this spline component, return true
			if (Point1Index != INDEX_NONE && Point2Index != INDEX_NONE)
			{
				return true;
			}
		}
	}

	// If no spline component contains both points, return false
	return false;
}

bool URoadNetworkToolLineTool::IsLineIntersectingSpline(const FVector& LineStart, const FVector& LineEnd)
{
	if (!SplineActor)
	{
		return false;
	}

	TArray<USplineComponent*> NearbySplines;

	SplineActor->PathfindingComponent->FindSplinesInLineArea(LineStart, LineEnd, NearbySplines);

	for (USplineComponent* SplineComponent : NearbySplines)
	{
		int32 NumSplinePoints = SplineComponent->GetNumberOfSplinePoints();
		if (NumSplinePoints < 2)
		{
			continue; // Skip if the spline doesn't have at least two points
		}

		// Check intersection for each segment of the spline
		for (int32 PointIndex = 0; PointIndex < NumSplinePoints - 1; ++PointIndex)
		{
			// Get the start and end points of the current spline segment
			FVector SplineStart = SplineComponent->GetLocationAtSplinePoint(PointIndex, ESplineCoordinateSpace::World);
			FVector SplineEnd = SplineComponent->GetLocationAtSplinePoint(PointIndex + 1, ESplineCoordinateSpace::World);

			// Check for intersection between the lines in the XY plane
			if (DoLinesIntersect(LineStart, LineEnd, SplineStart, SplineEnd))
			{
				// Ensure the intersection is not at the endpoints of the line
				if (!LineStart.Equals(SplineStart, 1.0f) && !LineStart.Equals(SplineEnd, 1.0f) &&
					!LineEnd.Equals(SplineStart, 1.0f) && !LineEnd.Equals(SplineEnd, 1.0f))
				{
					return true;
				}
			}
		}
	}

	return false;
}

bool URoadNetworkToolLineTool::DoLinesIntersect(const FVector& A1, const FVector& A2, const FVector& B1, const FVector& B2)
{
	// Project the 3D points onto the XY plane (ignore Z value)
	FVector2D A1_2D(A1.X, A1.Y);
	FVector2D A2_2D(A2.X, A2.Y);
	FVector2D B1_2D(B1.X, B1.Y);
	FVector2D B2_2D(B2.X, B2.Y);

	// Direction vectors in 2D
	FVector2D DirA = A2_2D - A1_2D;
	FVector2D DirB = B2_2D - B1_2D;

	// Perpendicular vector
	auto PerpDot = [](const FVector2D& V1, const FVector2D& V2)
		{
			return V1.X * V2.Y - V1.Y * V2.X;
		};

	float Denominator = PerpDot(DirA, DirB);
	if (FMath::IsNearlyZero(Denominator))
	{
		// Lines are parallel or collinear in the XY plane
		return false;
	}

	FVector2D Diff = B1_2D - A1_2D;
	float t = PerpDot(Diff, DirB) / Denominator;
	float u = PerpDot(Diff, DirA) / Denominator;

	// Check if the intersection point lies on both segments
	return (t >= 0.0f && t <= 1.0f && u >= 0.0f && u <= 1.0f);
}

void URoadNetworkToolLineTool::OnPropertyModified(UObject* PropertySet, FProperty* Property)
{
	// if the user updated any of the property fields, update the property
	if (SplineActor)
	{
		ApplyProperties(SplineActor);
	}
	else
	{
		ARoadActor* FoundSplineActor = GetFirstRoadActor(TargetWorld);
		if (FoundSplineActor)
		{
			ApplyProperties(FoundSplineActor);
		}
	}
	Properties->SaveConfiguration();
}

void URoadNetworkToolLineTool::ApplyProperties(ARoadActor* RoadActor)
{
	if (RoadActor)
	{
		RoadActor->EnableRoadDebugLine = Properties->EnableDebugLine;
		RoadActor->DebugWidth = Properties->Width;
		RoadActor->RoadThickness = Properties->Thickness;
		Properties->SnapThreshold = RoadActor->DebugWidth;
	}
	Properties->SaveConfiguration();
}


#undef LOCTEXT_NAMESPACE
