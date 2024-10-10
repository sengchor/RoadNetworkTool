#include "RoadActor.h"
#include "RoadHelper.h"
#include "Components/SplineComponent.h"
#include "KismetProceduralMeshLibrary.h"
#include "DrawDebugHelpers.h"
#include "Materials/MaterialInterface.h"
#include "FSplinePointUtilities.h"

using namespace SplineUtilities;

bool ARoadActor::bIsInRoadNetworkMode = false;
bool ARoadActor::EnableRoadDebugLine = false;
float ARoadActor::DebugWidth = 500.0f;
float ARoadActor::RoadThickness = 20.0f;

ARoadActor::ARoadActor()
{
	PrimaryActorTick.bCanEverTick = true;

	// Set the actor as a Road Actor using your helper method
	FRoadHelper::SetIsRoadActor(this, true);

	RootSceneComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));
	RootComponent = RootSceneComponent;
	RootComponent->SetMobility(EComponentMobility::Static);

	SplineRootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("SplineRootComponent"));
	SplineRootComponent->SetupAttachment(RootComponent);
	SplineRootComponent->SetMobility(EComponentMobility::Static);

	MeshRootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("MeshRootComponent"));
	MeshRootComponent->SetupAttachment(RootComponent);
	MeshRootComponent->SetMobility(EComponentMobility::Static);

	PathfindingComponent = CreateDefaultSubobject<URoadPathfindingComponent>(TEXT("PathfindingComponent"));
}

void ARoadActor::BeginPlay()
{
	Super::BeginPlay();

	InitializeQuadtree();
}

void ARoadActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Only draw debug road width if in the RoadNetwork mode and in the editor
#if WITH_EDITOR
	if (GEditor && !GetWorld()->IsGameWorld())
	{
		if (bIsInRoadNetworkMode && EnableRoadDebugLine)
		{
			DrawDebugRoadWidth(DebugWidth, RoadThickness, FColor::Green, 0.0f);
		}

		if (bDebugSelectedPoint)
		{
			DrawDebugSphere(GetWorld(), SelectedPoint, 100.0f, 12, FColor::Blue, false, 0.0f);
		}
	}
#endif
}

bool ARoadActor::ShouldTickIfViewportsOnly() const
{
	return true;
}


// ---------- Spline management functions ---------
void ARoadActor::AddSplineComponent(USplineComponent* SplineComponent)
{
	AddOrUpdateSplineComponent(SplineComponent, false);
}

void ARoadActor::UpdateSplineComponent(USplineComponent* SplineComponent)
{
	AddOrUpdateSplineComponent(SplineComponent, true);
}

void ARoadActor::AddOrUpdateSplineComponent(USplineComponent* SplineComponent, bool bIsUpdate = false)
{
	if (!SplineComponent) return;

	if (!bIsUpdate) { SplineComponents.AddUnique(SplineComponent); }
	UpdateComponentTransforms();

	FBox SplineBounds3D = SplineComponent->Bounds.GetBox();
	FBox2D SplineBounds(FVector2D(SplineBounds3D.Min.X, SplineBounds3D.Min.Y), FVector2D(SplineBounds3D.Max.X, SplineBounds3D.Max.Y));

	if (!SplineQuadtree.IsValid() || !SplineQuadtree->GetBounds().IsInside(SplineBounds))
	{
		// Reinitialize the quadtree with the updated bounds if the spline is outside the current quadtree bounds
		InitializeQuadtree();
		return;
	}

	if (bIsUpdate)
	{
		SplineQuadtree->UpdateSplineComponent(SplineComponent);
	}
	else
	{
		SplineQuadtree->InsertSplineComponent(SplineComponent);
	}
}

const TArray<USplineComponent*>& ARoadActor::GetSplineComponents() const
{
	return SplineComponents;
}


// ---------- Quadtree Functions ---------
FBox2D ARoadActor::CalculateSquareBounds(float PaddingPercentage)
{
	// Forces the actor's components to update their transforms
	UpdateComponentTransforms();

	// Get the bounding box of the entire RoadActor
	FBox WorldBounds3D = GetComponentsBoundingBox(true);

	FBox2D WorldBounds(FVector2D(WorldBounds3D.Min.X, WorldBounds3D.Min.Y), FVector2D(WorldBounds3D.Max.X, WorldBounds3D.Max.Y));

	// Expand the bounds by the calculated padding
	FVector2D BoundsSize = WorldBounds.GetSize();
	FVector2D Padding = BoundsSize * PaddingPercentage;
	WorldBounds = WorldBounds.ExpandBy(Padding);

	// Convert the bounds to a square by equalizing X and Y extents
	float MaxExtent = FMath::Max(WorldBounds.GetSize().X, WorldBounds.GetSize().Y);
	FVector2D Center = WorldBounds.GetCenter();

	// Create a new square bounds based on the maximum extent
	FBox2D SquareWorldBounds(
		FVector2D(Center.X - MaxExtent / 2, Center.Y - MaxExtent / 2),
		FVector2D(Center.X + MaxExtent / 2, Center.Y + MaxExtent / 2)
	);

	return SquareWorldBounds;
}


void ARoadActor::InitializeQuadtree()
{
	const float PaddingPercentage = 0.30f;
	FBox2D SquareWorldBounds = CalculateSquareBounds(PaddingPercentage);

	if (SplineQuadtree.IsValid())
	{
		SplineQuadtree->Clear();
	}

	// If quadtree doesn't exist or the bounds have changed, re-create it
	if (!SplineQuadtree.IsValid() || !SplineQuadtree->GetBounds().IsInside(SquareWorldBounds))
	{
		SplineQuadtree = MakeShared<FQuadtree>(SquareWorldBounds, MaxSplinesPerNode, MaxDepth);
	}

	SplineQuadtree->SetVisualizeQuadtree(VisualizeQuadtree);

	// Get the 2D bounds from the quadtree
	FBox2D QuadtreeBounds2D = SplineQuadtree->GetBounds();

	// Add all spline components to the quadtree
	for (USplineComponent* SplineComponent : SplineComponents)
	{
		if (SplineComponent)
		{
			SplineQuadtree->InsertSplineComponent(SplineComponent);
		}
	}
	if (VisulaizeSplineQuadtree)
	{
		DrawAllSplineDebugLines();
	}

	AllPathNodes = PathfindingComponent->FindAllNodes(SplineComponents);
}


// ---------- Pathfinding Functions ---------
TArray<FVector> ARoadActor::FindPathRoadNetwork(FVector StartLocation, FVector TargetLocation, bool bRightOffset)
{
	TArray<FVector> Path;

	// Find the closest nodes for the start and end
	TSharedPtr<FPathNode> StartNode = FindNearestNodeWithSpline(StartLocation);
	TSharedPtr<FPathNode> EndNode = FindNearestNodeWithSpline(TargetLocation);

	// Perform pathfinding
	TArray<TSharedPtr<FPathNode>> PathNodes = PathfindingComponent->AStarPathfinding(StartNode, EndNode, AllPathNodes);

	if (PathNodes.Num() == 0)
	{
		UE_LOG(LogTemp, Error, TEXT("No path found between the given start and target locations."));
		return Path;
	}

	// Create a deep copy of path nodes
	TArray<TSharedPtr<FPathNode>> CopiedPathNodes = CreateDeepCopyOfPathNodes(PathNodes);

	if (CopiedPathNodes[0]->Location != StartNode->Location)
	{
		CopiedPathNodes.Insert(StartNode, 0);
	}
	if (CopiedPathNodes.Last()->Location != EndNode->Location)
	{
		CopiedPathNodes.Add(EndNode);
	}

	// Adjust spline nodes if valid
	USplineComponent* StartSpline = nullptr;
	USplineComponent* EndSpline = nullptr;
	AdjustSplineNodes(CopiedPathNodes, StartLocation, TargetLocation, StartSpline, EndSpline);

	// Refine the path using spline points
	Path = RefinePathWithSplinePoints(CopiedPathNodes, StartSpline, EndSpline);
	if (bRightOffset)
	{
		ApplyRightOffsetToPathNodes(Path, RoadWidth);
	}

	// Add start and end points if path is not empty
	if (Path.Num() > 0)
	{
		Path = AddPathWithStartAndEndPoints(Path, StartLocation, TargetLocation);
	}

	return Path;
}


TArray<FVector> ARoadActor::RefinePathWithSplinePoints(TArray<TSharedPtr<FPathNode>>& PathNodes, USplineComponent* StartSpline, USplineComponent* EndSpline)
{
	TArray<FVector> PathLocations;

	if (PathNodes.Num() < 2)
	{
		return PathLocations;
	}

	// Iterate through each node in the PathNodes array, processing node pairs
	for (int32 i = 0; i < PathNodes.Num() - 1; ++i)
	{
		TSharedPtr<FPathNode> CurrentNode = PathNodes[i];
		TSharedPtr<FPathNode> NextNode = PathNodes[i + 1];

		// Determine the appropriate spline for the current segment
		USplineComponent* SplineComponent = nullptr;

		// Choose based on the node index (StartSpline, EndSpline, or dynamically found spline)
		if (i == 0)
		{
			SplineComponent = StartSpline;
		}
		else if (i == PathNodes.Num() - 2)
		{
			SplineComponent = EndSpline;
		}

		// If the predefined splines (StartSpline/EndSpline) are not available, find a spline containing the nodes
		if (!SplineComponent)
		{
			SplineComponent = PathfindingComponent->FindSplineContainingNodes(CurrentNode, NextNode);
		}

		// If no spline is found, use the nearest spline based on the midpoint of the segment
		if (!SplineComponent)
		{
			FVector MidPoint = (CurrentNode->Location + NextNode->Location) * 0.5f;
			SplineComponent = PathfindingComponent->FindNearestSplineComponent(MidPoint);
		}

		// If a valid spline is found, sample points along it and add to the path
		if (SplineComponent)
		{
			TArray<FVector> SplinePoints = GetSplinePointsBetweenLocations(SplineComponent, 400.0f, CurrentNode->Location, NextNode->Location);
			PathLocations.Append(SplinePoints);
		}
	}

	return PathLocations;
}


TArray<FVector> ARoadActor::AddPathWithStartAndEndPoints(TArray<FVector>& PathLocations, FVector StartLocation, FVector TargetLocation)
{
	TArray<FVector> RefinedPathLocations;

	TArray<FVector> StartToFirstPointArray = GetPointsBetweenLocations(StartLocation, PathLocations[0], 400.0f);
	TArray<FVector> LastPointToEndArray = GetPointsBetweenLocations(PathLocations[PathLocations.Num() - 1], TargetLocation, 400.0f);

	RefinedPathLocations.Append(StartToFirstPointArray);
	RefinedPathLocations.Append(PathLocations);
	RefinedPathLocations.Append(LastPointToEndArray);

	return RefinedPathLocations;
}


void ARoadActor::AdjustSplineNodes(TArray<TSharedPtr<FPathNode>>& PathNodes, FVector StartLocation, FVector TargetLocation, USplineComponent*& OutStartSpline, USplineComponent*& OutEndSpline)
{
	// Early return if there are not enough nodes to adjust
	if (PathNodes.Num() < 1)
	{
		return;
	}

	// Variables to hold spline components
	USplineComponent* StartSpline = nullptr;
	USplineComponent* EndSpline = nullptr;

	if (PathNodes.Num() > 1)
	{
		StartSpline = PathfindingComponent->FindSplineContainingNodes(PathNodes[0], PathNodes[1]);
		EndSpline = PathfindingComponent->FindSplineContainingNodes(PathNodes[PathNodes.Num() - 2], PathNodes[PathNodes.Num() - 1]);
	}

	USplineComponent* StartNearSpline = PathfindingComponent->FindNearestSplineComponent(StartLocation);
	USplineComponent* EndNearSpline = PathfindingComponent->FindNearestSplineComponent(TargetLocation);

	// Check if both the start and end are near the same spline
	if (StartNearSpline == EndNearSpline && StartNearSpline != nullptr && EndNearSpline != nullptr)
	{
		FVector StartSplineLocation = StartNearSpline->FindLocationClosestToWorldLocation(StartLocation, ESplineCoordinateSpace::World);
		FVector TargetSplineLocation = EndNearSpline->FindLocationClosestToWorldLocation(TargetLocation, ESplineCoordinateSpace::World);

		if (PathNodes.Num() > 1)
		{
			// Adjust the start and end locations for path nodes
			PathNodes[0]->Location = StartSplineLocation;
			PathNodes[PathNodes.Num() - 1]->Location = TargetSplineLocation;
		}
		else
		{
			// Replace the single node with two nodes representing start and end points
			PathNodes.Empty();
			PathNodes.Add(MakeShared<FPathNode>(StartSplineLocation));
			PathNodes.Add(MakeShared<FPathNode>(TargetSplineLocation));
		}

		OutStartSpline = StartNearSpline;
		OutEndSpline = EndNearSpline;
	}
	else
	{
		// Handle the start node adjustment
		if (StartSpline == StartNearSpline && StartSpline != nullptr)
		{
			PathNodes[0]->Location = StartSpline->FindLocationClosestToWorldLocation(StartLocation, ESplineCoordinateSpace::World);

			OutStartSpline = StartSpline;
		}
		else if (StartNearSpline)
		{
			FVector StartSplineLocation = StartNearSpline->FindLocationClosestToWorldLocation(StartLocation, ESplineCoordinateSpace::World);
			PathNodes.Insert(MakeShared<FPathNode>(StartSplineLocation), 0);

			OutStartSpline = StartNearSpline;
		}

		// Handle the end node adjustment
		if ((EndSpline == EndNearSpline) && (EndSpline != nullptr) && (PathNodes.Num() > 1))
		{
			PathNodes[PathNodes.Num() - 1]->Location = EndSpline->FindLocationClosestToWorldLocation(TargetLocation, ESplineCoordinateSpace::World);

			OutEndSpline = EndSpline;
		}
		else if (EndNearSpline)
		{
			FVector TargetSplineLocation = EndNearSpline->FindLocationClosestToWorldLocation(TargetLocation, ESplineCoordinateSpace::World);
			PathNodes.Add(MakeShared<FPathNode>(TargetSplineLocation));

			OutEndSpline = EndNearSpline;
		}
	}
}


void ARoadActor::ApplyRightOffsetToPathNodes(TArray<FVector>& Path, float Width)
{
	for (int32 i = 0; i < Path.Num(); i++)
	{
		FVector& CurrentLocation = Path[i];
		FVector* NextLocation = (i + 1 < Path.Num()) ? &Path[i + 1] : nullptr;
		FVector* PreviousLocation = (i > 0) ? &Path[i - 1] : nullptr;

		FVector RightVector;

		// Adjust location if there is either a NextLocation or PreviousLocation
		if (NextLocation && PreviousLocation)
		{
			FVector ForwardVector1 = (*NextLocation - CurrentLocation).GetSafeNormal();
			FVector ForwardVector2 = (CurrentLocation - *PreviousLocation).GetSafeNormal();

			RightVector = FVector::CrossProduct(FVector::UpVector, (ForwardVector1 + ForwardVector2).GetSafeNormal()).GetSafeNormal();

			CurrentLocation += RightVector * Width / 4.0f;
		}
		else if (NextLocation || PreviousLocation)
		{
			FVector ForwardVector = NextLocation
				? (*NextLocation - CurrentLocation).GetSafeNormal()
				: (CurrentLocation - *PreviousLocation).GetSafeNormal();

			RightVector = FVector::CrossProduct(FVector::UpVector, ForwardVector).GetSafeNormal();

			// Apply the right offset to the current location
			CurrentLocation += RightVector * Width / 4.0f;
		}
	}
}


// ---------- Node management functions ---------
TArray<TSharedPtr<FPathNode>> ARoadActor::CreateDeepCopyOfPathNodes(const TArray<TSharedPtr<FPathNode>>& OriginalPathNodes)
{
	TArray<TSharedPtr<FPathNode>> DeepCopy;
	for (const TSharedPtr<FPathNode>& Node : OriginalPathNodes)
	{
		// Create a new shared pointer to a new FPathNode with copied location and properties
		TSharedPtr<FPathNode> NewNode = MakeShared<FPathNode>(*Node);
		DeepCopy.Add(NewNode);
	}
	return DeepCopy;
}


TSharedPtr<FPathNode> ARoadActor::FindNearestNodeWithSpline(const FVector& Location)
{
	// Find the nearest spline to the TargetLocation
	USplineComponent* NearSpline = PathfindingComponent->FindNearestSplineComponent(Location);

	// Check if a spline was found
	if (NearSpline)
	{
		// Calculate start and end points of the nearest spline
		FVector SplineStartLocation = NearSpline->GetLocationAtSplinePoint(0, ESplineCoordinateSpace::World);
		FVector SplineEndLocation = NearSpline->GetLocationAtSplinePoint(NearSpline->GetNumberOfSplinePoints() - 1, ESplineCoordinateSpace::World);

		// Determine which endpoint is closest to the TargetLocation
		FVector NearestLocation = (FVector::Dist(Location, SplineStartLocation) < FVector::Dist(Location, SplineEndLocation))
			? SplineStartLocation
			: SplineEndLocation;

		// Find and return the nearest node in AllPathNodes to the closest spline endpoint
		return PathfindingComponent->FindNearestNodeByLocation(NearestLocation, AllPathNodes);
	}
	else
	{
		// No spline found, use the nearest node to the TargetLocation
		return PathfindingComponent->FindNearestNodeByLocation(Location, AllPathNodes);
	}
}


// ---------- Debug functions ---------
void ARoadActor::DrawAllSplineDebugLines()
{
	if (SplineQuadtree.IsValid())
	{
		TArray<USplineComponent*> AllSplines;
		SplineQuadtree->GetAllSplines(AllSplines);

		// Draw debug lines for each spline
		for (USplineComponent* SplineComponent : AllSplines)
		{
			if (SplineComponent)
			{
				int32 NumPoints = SplineComponent->GetNumberOfSplinePoints();
				for (int32 PointIndex = 0; PointIndex < NumPoints - 1; PointIndex++)
				{
					FVector Start = SplineComponent->GetLocationAtSplinePoint(PointIndex, ESplineCoordinateSpace::World);
					FVector End = SplineComponent->GetLocationAtSplinePoint(PointIndex + 1, ESplineCoordinateSpace::World);
					DrawDebugLine(GetWorld(), Start, End, FColor::Red, false, 50.0f, 0, 50.0f);
				}
			}
		}
	}
}


void ARoadActor::DrawDebugRoadWidth(float Width, float Thickness, FColor Color, float Duration)
{
	for (USplineComponent* SplineComponent : SplineComponents)
	{
		if (SplineComponent)
		{
			const int32 NumPoints = SplineComponent->GetNumberOfSplinePoints();

			// Loop through each segment between spline points
			for (int32 i = 0; i < NumPoints - 1; ++i)
			{
				// Get the location and tangent at the current spline point
				const FVector PointLocation = SplineComponent->GetLocationAtSplinePoint(i, ESplineCoordinateSpace::World);
				const FVector NextPointLocation = SplineComponent->GetLocationAtSplinePoint(i + 1, ESplineCoordinateSpace::World);

				// Calculate the midpoint between this point and the next point
				FVector MidPoint = (PointLocation + NextPointLocation) / 2.0f;

				const FVector Direction = (NextPointLocation - PointLocation).GetSafeNormal();
				FRotator Rotation = Direction.Rotation();

				// Calculate the box dimensions
				const FVector BoxExtent = FVector((NextPointLocation - PointLocation).Size() / 2.0f, Width * 0.5f, Thickness * 0.5f);

				// Adjust midpoint upward by half the thickness to keep it aligned with the spline
				MidPoint += FVector::UpVector * Thickness * 0.5f;

				// Draw the debug box for the current road segment
				DrawDebugBox(GetWorld(), MidPoint, BoxExtent, Rotation.Quaternion(), Color, false, Duration);
			}
		}
	}
}


void ARoadActor::SetDebugSelectedPoint(bool bEnable, const FVector& NewSelectedPoint)
{
	bDebugSelectedPoint = bEnable;
	SelectedPoint = NewSelectedPoint;
}


FVector ARoadActor::GetSelectedPoint()
{
	return SelectedPoint;
}


void ARoadActor::DestroyProceduralMeshes()
{
	for (UProceduralMeshComponent* ProcMeshComponent : ProceduralMeshes)
	{
		if (ProcMeshComponent)
		{
			ProcMeshComponent->DestroyComponent();
		}
	}

	ProceduralMeshes.Empty();
}