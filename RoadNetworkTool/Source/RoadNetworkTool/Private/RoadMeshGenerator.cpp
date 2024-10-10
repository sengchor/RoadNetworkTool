#include "RoadMeshGenerator.h"
#include "ProceduralMeshComponent.h"
#include "Components/SplineComponent.h"
#include "DrawDebugHelpers.h"
#include "FSplinePointUtilities.h"

using namespace SplineUtilities;

FRoadMeshGenerator::FRoadMeshGenerator()
{

}

// Intersection and NonIntersection Detection
TArray<FIntersectionNode> FRoadMeshGenerator::FindSplineIntersectionNodes(const TArray<USplineComponent*>& SplineComponents) const
{
	TArray<FIntersectionNode> IntersectionNodes;
	const float IntersectionThreshold = 10.0f;

	for (int32 i = 0; i < SplineComponents.Num(); i++)
	{
		USplineComponent* SplineA = SplineComponents[i];
		if (!SplineA) continue;

		// Get the start and end points of SplineA
		FVector StartA = SplineA->GetLocationAtSplinePoint(0, ESplineCoordinateSpace::World);
		FVector EndA = SplineA->GetLocationAtSplinePoint(SplineA->GetNumberOfSplinePoints() - 1, ESplineCoordinateSpace::World);

		for (int32 j = 0; j < SplineComponents.Num(); j++)
		{
			USplineComponent* SplineB = SplineComponents[j];
			if (j == i || !SplineB) continue;

			// Get the start and end points of SplineB
			FVector StartB = SplineB->GetLocationAtSplinePoint(0, ESplineCoordinateSpace::World);
			FVector EndB = SplineB->GetLocationAtSplinePoint(SplineB->GetNumberOfSplinePoints() - 1, ESplineCoordinateSpace::World);

			// Check for intersection at the start or end points
			if (FVector::DistSquared(StartA, StartB) <= FMath::Square(IntersectionThreshold) ||
				FVector::DistSquared(StartA, EndB) <= FMath::Square(IntersectionThreshold) ||
				FVector::DistSquared(EndA, StartB) <= FMath::Square(IntersectionThreshold) ||
				FVector::DistSquared(EndA, EndB) <= FMath::Square(IntersectionThreshold))
			{
				FVector IntersectionPoint = (FVector::DistSquared(StartA, StartB) <= FMath::Square(IntersectionThreshold)) ? StartA :
					(FVector::DistSquared(StartA, EndB) <= FMath::Square(IntersectionThreshold)) ? StartA :
					(FVector::DistSquared(EndA, StartB) <= FMath::Square(IntersectionThreshold)) ? EndA : EndA;

				bool bIsNearExistingNode = false;
				for (FIntersectionNode& Node : IntersectionNodes)
				{
					if (FVector::DistSquared(IntersectionPoint, Node.IntersectionPoint) <= FMath::Square(IntersectionThreshold))
					{
						Node.IntersectingSplines.AddUnique(SplineA);
						Node.IntersectingSplines.AddUnique(SplineB);
						bIsNearExistingNode = true;
						break;
					}
				}

				if (!bIsNearExistingNode)
				{
					FIntersectionNode NewNode(IntersectionPoint);
					NewNode.IntersectingSplines.AddUnique(SplineA);
					NewNode.IntersectingSplines.AddUnique(SplineB);
					IntersectionNodes.Add(NewNode);
				}
			}
		}
	}

	return IntersectionNodes;
}

TArray<FNonIntersectionNode> FRoadMeshGenerator::FindSplineNonIntersectionNodes(const TArray<USplineComponent*>& SplineComponents) const
{
	TArray<FNonIntersectionNode> NonIntersectionNodes;
	const float IntersectionThreshold = 1.0f;
	TArray<FIntersectionNode> IntersectionNodes = FindSplineIntersectionNodes(SplineComponents);

	for (USplineComponent* Spline : SplineComponents)
	{
		if (!Spline) continue;
		const int32 NumPoints = Spline->GetNumberOfSplinePoints();

		// Consider only the start (Index 0) and end (Index NumPoints - 1) points
		for (int32 PointIndex : {0, NumPoints - 1})
		{
			if (PointIndex >= NumPoints) continue; // Ensure the point index is within range
			FVector SplinePoint = Spline->GetLocationAtSplinePoint(PointIndex, ESplineCoordinateSpace::World);

			bool bIsIntersecting = false;
			for (const FIntersectionNode& Node : IntersectionNodes)
			{
				if (FVector::DistSquared(SplinePoint, Node.IntersectionPoint) <= FMath::Square(IntersectionThreshold))
				{
					bIsIntersecting = true;
					break;
				}
			}

			if (!bIsIntersecting)
			{
				FNonIntersectionNode NewNode(SplinePoint);
				NewNode.NonIntersectingSplines.Add(Spline);
				NonIntersectionNodes.Add(NewNode);
			}
		}
	}

	return NonIntersectionNodes;
}

// Road Mesh Generation
void FRoadMeshGenerator::GenerateRoadMesh(ARoadActor* RoadActor)
{
	RoadActor->RoadWidth = RoadActor->DebugWidth;
	RoadActor->DestroyProceduralMeshes();

	// Find intersection and non-intersection nodes
	TArray<FIntersectionNode> IntersectionNodes = this->FindSplineIntersectionNodes(RoadActor->SplineComponents);
	TArray<FNonIntersectionNode> NonIntersectionNodes = this->FindSplineNonIntersectionNodes(RoadActor->SplineComponents);

	// Map each SplineComponent to its corresponding SplineData
	TMap<USplineComponent*, FSplineData> SplineDataMap;

	// Process intersection nodes
	for (const FIntersectionNode& IntersectionNode : IntersectionNodes)
	{
		TArray<FSplineData> SplineDataArray;
		TArray<FVector> IntersectionLocations = FindIntersectionPointsFromNode(RoadActor, IntersectionNode, SplineDataArray);
		MergeSplineDataIntoMap(SplineDataMap, SplineDataArray);

		// Use IntersectionPoints to generate road intersections
		UProceduralMeshComponent* ProcMeshComponent = GenerateFanMeshFromPoints(RoadActor, IntersectionNode.IntersectionPoint, IntersectionLocations);

		SetRoadMeshMaterialAndCollision(ProcMeshComponent);
	}

	// Process non-intersection nodes
	for (const FNonIntersectionNode& NonIntersectionNode : NonIntersectionNodes)
	{
		MergeNonIntersectionPointsIntoMap(NonIntersectionNode, RoadActor->RoadWidth, SplineDataMap);
	}


	// Use the SplineDataMap to generate road segments
	for (USplineComponent* SplineComponent : RoadActor->SplineComponents)
	{
		if (FSplineData* SplineData = SplineDataMap.Find(SplineComponent))
		{
			TArray<FVector> LeftSegmentPoints, RightSegmentPoints;

			// Find segment points for this spline and generate the road mesh
			FindSegmentPointsFromSpline(RoadActor, *SplineData, LeftSegmentPoints, RightSegmentPoints);

			UProceduralMeshComponent* ProcMeshComponent = GenerateQuadMeshFromPoints(RoadActor, LeftSegmentPoints, RightSegmentPoints);

			SetRoadMeshMaterialAndCollision(ProcMeshComponent);
		}
	}
}

UProceduralMeshComponent* FRoadMeshGenerator::GenerateQuadMeshFromPoints(ARoadActor* RoadActor, const TArray<FVector>& LeftPoints, const TArray<FVector>& RightPoints)
{
	if (LeftPoints.Num() != RightPoints.Num() || LeftPoints.Num() < 2)
	{
		UE_LOG(LogTemp, Warning, TEXT("Invalid number of points for quad mesh generation."));
		return nullptr;
	}

	UProceduralMeshComponent* ProcMeshComponent = NewObject<UProceduralMeshComponent>(RoadActor);
	ProcMeshComponent->SetupAttachment(RoadActor->MeshRootComponent);
	ProcMeshComponent->RegisterComponentWithWorld(RoadActor->GetWorld());

	TArray<FVector> Vertices;
	TArray<int32> Triangles;
	TArray<FVector> Normals;
	TArray<FVector2D> UVs;
	TArray<FColor> VertexColors;
	TArray<FProcMeshTangent> Tangents;

	// Combine left and right points into a single vertex array (alternating order)
	for (int32 i = 0; i < LeftPoints.Num(); ++i)
	{
		Vertices.Add(LeftPoints[i]);   // Add left point
		Vertices.Add(RightPoints[i]);  // Add right point
	}

	// Generate triangles for quads (two triangles per quad)
	for (int32 i = 0; i < Vertices.Num() - 2; i += 2)
	{
		// First triangle of the quad (clockwise winding)
		if (!IsDegenerateTriangle(Vertices[i], Vertices[i + 2], Vertices[i + 1]))
		{
			Triangles.Add(i);
			Triangles.Add(i + 2);
			Triangles.Add(i + 1);
		}

		// Second triangle of the quad (clockwise winding)
		if (!IsDegenerateTriangle(Vertices[i + 1], Vertices[i + 2], Vertices[i + 3]))
		{
			Triangles.Add(i + 1);
			Triangles.Add(i + 2);
			Triangles.Add(i + 3);
		}
	}

	// Generate UVs based on vertex positions
	for (int32 i = 0; i < Vertices.Num(); ++i)
	{
		// Generate simple UVs - assuming road width is constant
		float U = (i / 2) * 0.1f;  // U maps to distance along the road
		float V = (i % 2 == 0) ? 0.0f : 1.0f;  // V maps across the width of the road (0 for left, 1 for right)
		UVs.Add(FVector2D(U, V));
	}

	// Set up normals (pointing up along the Z axis)
	for (int32 i = 0; i < Vertices.Num(); ++i)
	{
		Normals.Add(FVector(0.0f, 0.0f, 1.0f));  // Normal pointing up
	}

	// Set up tangents (pointing along the X axis)
	for (int32 i = 0; i < Vertices.Num(); ++i)
	{
		Tangents.Add(FProcMeshTangent(1.0f, 0.0f, 0.0f));  // Tangent pointing along X axis
	}

	// Create the mesh section using the attributes
	ProcMeshComponent->CreateMeshSection(0, Vertices, Triangles, Normals, UVs, VertexColors, Tangents, true);

	// Enable collision if needed
	ProcMeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);

	ProcMeshComponent->SetRelativeLocation(FVector(0.0f, 0.0f, 1.0f));

	RoadActor->ProceduralMeshes.Add(ProcMeshComponent);

	return ProcMeshComponent;
}


UProceduralMeshComponent* FRoadMeshGenerator::GenerateFanMeshFromPoints(ARoadActor* RoadActor, const FVector& CenterPoint, const TArray<FVector>& Points)
{
	if (Points.Num() < 2)
	{
		UE_LOG(LogTemp, Warning, TEXT("Not enough surrounding points to create a fan mesh."));
		return nullptr;
	}

	TArray<FVector> OrderedPoints = Points;
	OrderPointsClockwise(OrderedPoints); // Order the points around the center

	UProceduralMeshComponent* ProcMeshComponent = NewObject<UProceduralMeshComponent>(RoadActor);
	ProcMeshComponent->SetupAttachment(RoadActor->MeshRootComponent);
	ProcMeshComponent->RegisterComponentWithWorld(RoadActor->GetWorld());

	TArray<FVector> Vertices;
	TArray<int32> Triangles;
	TArray<FVector> Normals;
	TArray<FVector2D> UVs;
	TArray<FColor> VertexColors;
	TArray<FProcMeshTangent> Tangents;

	// Add center point as the first vertex
	Vertices.Add(CenterPoint);

	// Add surrounding points as vertices
	for (const FVector& Point : OrderedPoints)
	{
		Vertices.Add(Point);
	}

	// Generate triangles for the fan (first vertex is the center point)
	for (int32 i = 1; i < Vertices.Num(); ++i)
	{
		int32 CenterIndex = 0;                         // Index for the center point
		int32 CurrentIndex = i;                         // Index for the current point
		int32 NextIndex = (i % (Vertices.Num() - 1)) + 1; // Index for the next point (wraps around)

		// Check for degenerate triangles before adding them
		if (!IsDegenerateTriangle(Vertices[CenterIndex], Vertices[CurrentIndex], Vertices[NextIndex]))
		{
			// Create a valid triangle if it is not degenerate
			Triangles.Add(CenterIndex);
			Triangles.Add(CurrentIndex);
			Triangles.Add(NextIndex);
		}
	}

	// Adjust UV mapping based on the position relative to the center
	for (int32 i = 0; i < Vertices.Num(); ++i)
	{
		// Calculate UVs such that the center point has (0.5, 0.5) and other points are mapped around it
		FVector2D UV((Vertices[i].X - CenterPoint.X) * 0.01f + 0.5f, (Vertices[i].Y - CenterPoint.Y) * 0.01f + 0.5f);
		UVs.Add(UV);
	}

	// Set up normals pointing up
	for (int32 i = 0; i < Vertices.Num(); ++i)
	{
		Normals.Add(FVector(0.0f, 0.0f, 1.0f)); // Normals pointing up
	}

	// Set up tangents pointing along the X-axis
	for (int32 i = 0; i < Vertices.Num(); ++i)
	{
		Tangents.Add(FProcMeshTangent(1.0f, 0.0f, 0.0f)); // Default tangent along X-axis
	}

	// Create the mesh section using the attributes
	ProcMeshComponent->CreateMeshSection(0, Vertices, Triangles, Normals, UVs, VertexColors, Tangents, true);

	// Set collision and properties
	ProcMeshComponent->SetCollisionProfileName(TEXT("Custom"));
	ProcMeshComponent->SetCollisionResponseToChannel(ECC_Visibility, ECR_Ignore);

	ProcMeshComponent->SetRelativeLocation(FVector(0.0f, 0.0f, 1.0f));

	RoadActor->ProceduralMeshes.Add(ProcMeshComponent);

	return ProcMeshComponent;
}

// Intersection Point Handling
TArray<FVector> FRoadMeshGenerator::FindIntersectionPointsFromNode(ARoadActor* RoadActor, const FIntersectionNode IntersectionNode, TArray<FSplineData>& SplineDataArray)
{
	TArray<FVector> IntersectionPoints;

	// Initialize and retrieve spline data for each spline component in the intersection node
	for (USplineComponent* SplineComponent : IntersectionNode.IntersectingSplines)
	{
		FSplineData SplineData(SplineComponent);
		TArray<FVector> SplinePoints = GetSplinePointsWithDistance(SplineComponent);
		GetOffsetSplinePointsByWidth(SplineComponent, SplinePoints, RoadActor->RoadWidth, SplineData.LeftPoints, SplineData.RightPoints);
		SplineDataArray.Add(SplineData);
	}

	// Find intersections between splines
	for (int32 i = 0; i < SplineDataArray.Num(); i++)
	{
		FSplineData& SplineA = SplineDataArray[i];

		for (int32 j = 0; j < SplineDataArray.Num(); j++)
		{
			if (j == i) continue; // Skip comparing the same spline

			FSplineData& SplineB = SplineDataArray[j];
			FindAndAddSplineIntersections(SplineA, SplineB, IntersectionNode);
		}

		// Process and add final intersection points for the left and right sides
		ProcessSplineSidePoints(SplineA.LeftPoints, SplineA.LeftIntersections, SplineA.SplineComponent, SplineA.OutLeftIntersections, IntersectionNode, IntersectionPoints);
		ProcessSplineSidePoints(SplineA.RightPoints, SplineA.RightIntersections, SplineA.SplineComponent, SplineA.OutRightIntersections, IntersectionNode, IntersectionPoints);
	}

	return IntersectionPoints;
}

void FRoadMeshGenerator::FindSegmentPointsFromSpline(ARoadActor* RoadActor, const FSplineData& SplineData, TArray<FVector>& LeftPoints, TArray<FVector>& RightPoints)
{
	//Ensure the intersection arrays have at least two points
	if (SplineData.OutLeftIntersections.Num() < 2 || SplineData.OutRightIntersections.Num() < 2)
	{
		UE_LOG(LogTemp, Warning, TEXT("Not enough intersection points to segment the spline."));
		return;
	}

	USplineComponent* SplineComponent = SplineData.SplineComponent;

	FVector FirstLeftIntersection = SplineData.OutLeftIntersections[0];
	FVector SecondLeftIntersection = SplineData.OutLeftIntersections[1];
	FVector FirstRightIntersection = SplineData.OutRightIntersections[0];
	FVector SecondRightIntersection = SplineData.OutRightIntersections[1];

	float FirstLeftKey = SplineComponent->FindInputKeyClosestToWorldLocation(FirstLeftIntersection);
	float SecondLeftKey = SplineComponent->FindInputKeyClosestToWorldLocation(SecondLeftIntersection);
	float FirstRightKey = SplineComponent->FindInputKeyClosestToWorldLocation(FirstRightIntersection);
	float SecondRightKey = SplineComponent->FindInputKeyClosestToWorldLocation(SecondRightIntersection);

	// Clear the output arrays
	LeftPoints.Empty();
	RightPoints.Empty();

	LeftPoints = GetSplinePointsWithinRange(SplineComponent, SplineData.LeftPoints, FirstLeftKey, SecondLeftKey, FirstLeftIntersection, SecondLeftIntersection);
	RightPoints = GetSplinePointsWithinRange(SplineComponent, SplineData.RightPoints, FirstRightKey, SecondRightKey, FirstRightIntersection, SecondRightIntersection);
}

void FRoadMeshGenerator::FindAndAddSplineIntersections(FSplineData& SplineA, FSplineData& SplineB, const FIntersectionNode& IntersectionNode)
{
	FVector NewIntersectionPoint;

	// Find left and right intersections between SplineA and SplineB
	auto CheckIntersection = [&](const TArray<FVector>& PointsA, const TArray<FVector>& PointsB, TArray<FVector>& IntersectionPoints)
		{
			if (FindIntersectionBetweenTwoSplines(PointsA, PointsB, IntersectionNode, NewIntersectionPoint))
			{
				IntersectionPoints.AddUnique(NewIntersectionPoint);
			}
		};

	CheckIntersection(SplineA.LeftPoints, SplineB.LeftPoints, SplineA.LeftIntersections);
	CheckIntersection(SplineA.LeftPoints, SplineB.RightPoints, SplineA.LeftIntersections);
	CheckIntersection(SplineA.RightPoints, SplineB.LeftPoints, SplineA.RightIntersections);
	CheckIntersection(SplineA.RightPoints, SplineB.RightPoints, SplineA.RightIntersections);
}

void FRoadMeshGenerator::ProcessSplineSidePoints(
	const TArray<FVector>& SplinePoints,
	const TArray<FVector>& SplineIntersections,
	USplineComponent* SplineComponent,
	TArray<FVector>& OutSplineIntersections,
	const FIntersectionNode& IntersectionNode,
	TArray<FVector>& OutIntersectionPoints)
{
	FVector IntersectionPoint, NonIntersectionPoint;
	TArray<FVector> NonIntersections = { SplinePoints[0], SplinePoints[SplinePoints.Num() - 1] };

	if (SplineIntersections.Num() > 0)
	{
		// Use the furthest intersection point
		IntersectionPoint = FindFurthestPoint(SplineIntersections, IntersectionNode.IntersectionPoint);
		OutIntersectionPoints.AddUnique(IntersectionPoint);

		OutSplineIntersections.Add(IntersectionPoint);
	}
	else
	{
		// Use the nearest non-intersection point
		NonIntersectionPoint = FindNearestPoint(NonIntersections, IntersectionNode.IntersectionPoint);
		
		OutIntersectionPoints.AddUnique(NonIntersectionPoint);

		OutSplineIntersections.Add(NonIntersectionPoint);
	}
}

// Spline Data
void FRoadMeshGenerator::MergeSplineDataIntoMap(TMap<USplineComponent*, FSplineData>& SplineDataMap, const TArray<FSplineData>& SplineDataArray)
{
	for (const FSplineData& SplineData : SplineDataArray)
	{
		// Check if the SplineComponent already exists in the map
		if (FSplineData* ExistingSplineData = SplineDataMap.Find(SplineData.SplineComponent))
		{
			// Append the properties to the existing data
			ExistingSplineData->OutLeftIntersections.Append(SplineData.OutLeftIntersections);
			ExistingSplineData->OutRightIntersections.Append(SplineData.OutRightIntersections);
		}
		else
		{
			// Add a new entry to the map
			SplineDataMap.Add(SplineData.SplineComponent, SplineData);
		}
	}
}

void FRoadMeshGenerator::MergeNonIntersectionPointsIntoMap(const FNonIntersectionNode& NonIntersectionNode, float RoadWidth, TMap<USplineComponent*, FSplineData>& SplineDataMap)
{
	for (USplineComponent* SplineComponent : NonIntersectionNode.NonIntersectingSplines)
	{
		FVector LeftNonIntersectionPoint, RightNonIntersectionPoint;

		// Calculate the offset points for this spline component
		GetOffsetSplinePointByWidth(SplineComponent, NonIntersectionNode.NonIntersectionPoint, RoadWidth, LeftNonIntersectionPoint, RightNonIntersectionPoint);

		// If the spline component exists in the map, update its data
		if (FSplineData* ExistingSplineData = SplineDataMap.Find(SplineComponent))
		{
			ExistingSplineData->OutLeftIntersections.Add(LeftNonIntersectionPoint);
			ExistingSplineData->OutRightIntersections.Add(RightNonIntersectionPoint);
		}
	}
}

// Spline Intersection
bool FRoadMeshGenerator::FindIntersectionBetweenTwoSplines(const TArray<FVector>& SplinePointsA, const TArray<FVector>& SplinePointsB, const FIntersectionNode IntersectionNode, FVector& OutIntersectionPoint)
{
	TArray<FVector> Intersections;
	FVector IntersectionNodePoint = IntersectionNode.IntersectionPoint;

	// Iterate through all segments of SplinePointsA
	for (int32 i = 0; i < SplinePointsA.Num() - 1; i++)
	{
		FVector A1 = SplinePointsA[i];
		FVector A2 = SplinePointsA[i + 1];

		// Iterate through all segments of SplinePointsB
		for (int32 j = 0; j < SplinePointsB.Num() - 1; j++)
		{
			FVector B1 = SplinePointsB[j];
			FVector B2 = SplinePointsB[j + 1];

			// Calculate intersection point (assuming 2D in the XY plane)
			FVector IntersectionLocation;
			if (LineSegmentIntersection2D(FVector2D(A1.X, A1.Y), FVector2D(A2.X, A2.Y), FVector2D(B1.X, B1.Y), FVector2D(B2.X, B2.Y), IntersectionLocation))
			{
				// Convert 2D intersection point back to 3D, using the average Z value
				IntersectionLocation.Z = (A1.Z + B1.Z) / 2.0f;

				// Create and add a new intersection point
				Intersections.Add(IntersectionLocation);
			}
		}
	}

	// If no intersections found, return false
	if (Intersections.Num() == 0)
	{
		OutIntersectionPoint = FVector::ZeroVector;
		return false;
	}

	// Find the point that is furthest from IntersectionNodePoint
	FVector FurthestPoint = FindFurthestPoint(Intersections, IntersectionNodePoint);

	// Set the furthest intersection point and return true
	OutIntersectionPoint = FurthestPoint;
	return true;
}

bool FRoadMeshGenerator::LineSegmentIntersection2D(
	const FVector2D& A1, const FVector2D& A2,
	const FVector2D& B1, const FVector2D& B2,
	FVector& IntersectionPoint)
{
	FVector2D S1 = A2 - A1;
	FVector2D S2 = B2 - B1;

	float Denominator = (-S2.X * S1.Y + S1.X * S2.Y);

	if (FMath::IsNearlyZero(Denominator))
	{
		return false; // Parallel or collinear segments
	}

	float s = (-S1.Y * (A1.X - B1.X) + S1.X * (A1.Y - B1.Y)) / Denominator;
	float t = (S2.X * (A1.Y - B1.Y) - S2.Y * (A1.X - B1.X)) / Denominator;

	if (s >= 0 && s <= 1 && t >= 0 && t <= 1)
	{
		// Intersection detected
		IntersectionPoint = FVector(A1.X + (t * S1.X), A1.Y + (t * S1.Y), 0.0f);
		return true;
	}

	return false; // No intersection
}

// Point Operations

// Function to cycle through a predefined array of colors and return the next one
FColor FRoadMeshGenerator::GetNextDebugColor()
{
	// Array of colors to cycle through
	static TArray<FColor> DebugColors = { FColor::Yellow, FColor::Red, FColor::Green, FColor::Blue, FColor::Cyan, FColor::Magenta, FColor::Orange, FColor::Purple };

	// Static variable to track the current color index
	static int32 ColorIndex = 0;

	// Select the next color in the array
	FColor CurrentColor = DebugColors[ColorIndex];

	// Update ColorIndex to the next value, wrapping around if necessary
	ColorIndex = (ColorIndex + 1) % DebugColors.Num();

	return CurrentColor;
}

void FRoadMeshGenerator::DrawDebugCurvedRoad(ARoadActor* RoadActor)
{
	RoadActor->RoadWidth = RoadActor->DebugWidth;

	for (USplineComponent* SplineComponent : RoadActor->SplineComponents)
	{
		TArray<FVector> SplinePoints = GetSplinePointsWithDistance(SplineComponent);

		// Declare separate arrays for left and right points
		TArray<FVector> LeftSplinePoints, RightSplinePoints;

		// Get the left and right offset points
		GetOffsetSplinePointsByWidth(SplineComponent, SplinePoints, RoadActor->RoadWidth, LeftSplinePoints, RightSplinePoints);

		// Visualize the points
		for (const FVector& Point : LeftSplinePoints)
		{
			DrawDebugSphere(RoadActor->GetWorld(), Point, 25.0f, 12, FColor::Orange, false, 1.0f);
		}
		for (const FVector& Point : RightSplinePoints)
		{
			DrawDebugSphere(RoadActor->GetWorld(), Point, 25.0f, 12, FColor::Orange, false, 1.0f);
		}
	}
}

bool FRoadMeshGenerator::IsDegenerateTriangle(const FVector& A, const FVector& B, const FVector& C)
{
	// Calculate cross product between vectors AB and AC
	FVector AB = B - A;
	FVector AC = C - A;
	FVector CrossProduct = FVector::CrossProduct(AB, AC);

	// Check if the magnitude of the cross product is zero (or close to zero)
	bool bIsDegenerate = CrossProduct.SizeSquared() < KINDA_SMALL_NUMBER;

	return bIsDegenerate;
}

void FRoadMeshGenerator::SetRoadMeshMaterialAndCollision(UProceduralMeshComponent* ProcMeshComponent)
{
	if (!ProcMeshComponent)
	{
		return;
	}

	// Load and set the material
	UMaterialInterface* Material = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Traffic/Material/M_Road_Inst.M_Road_Inst"));
	if (Material)
	{
		ProcMeshComponent->SetMaterial(0, Material);
	}

	// Set Collision
	ProcMeshComponent->SetCollisionProfileName(TEXT("Custom"));
	ProcMeshComponent->SetCollisionResponseToChannel(ECC_Visibility, ECR_Ignore);
}