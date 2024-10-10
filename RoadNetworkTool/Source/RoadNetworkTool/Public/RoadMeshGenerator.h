#pragma once

#include "CoreMinimal.h"
#include "Components/SplineComponent.h"
#include "ProceduralMeshComponent.h"
#include "RoadActor.h"

struct FIntersectionNode
{
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Intersection")
	FVector IntersectionPoint;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Intersection")
	TArray<USplineComponent*> IntersectingSplines;

	FIntersectionNode()
		: IntersectionPoint(FVector::ZeroVector)
	{}

	FIntersectionNode(const FVector& InIntersectionPoint)
		: IntersectionPoint(InIntersectionPoint)
	{}
};

struct FNonIntersectionNode
{
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "NonIntersection")
	FVector NonIntersectionPoint;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "NonIntersection")
	TArray<USplineComponent*> NonIntersectingSplines;

	FNonIntersectionNode()
		: NonIntersectionPoint(FVector::ZeroVector)
	{}

	FNonIntersectionNode(const FVector& InNonIntersectionPoint)
		: NonIntersectionPoint(InNonIntersectionPoint)
	{}
};

struct FSplineData
{
	USplineComponent* SplineComponent;
	TArray<FVector> LeftPoints;
	TArray<FVector> RightPoints;
	TArray<FVector> LeftIntersections;
	TArray<FVector> RightIntersections;
	TArray<FVector> OutLeftIntersections;
	TArray<FVector> OutRightIntersections;

	FSplineData(USplineComponent* InSplineComponent)
		: SplineComponent(InSplineComponent) {}
};


class ROADNETWORKTOOL_API FRoadMeshGenerator
{
public:
	// Sets default values for this actor's properties
	FRoadMeshGenerator();

	// Intersection and NonIntersection Detection
	TArray<FIntersectionNode> FindSplineIntersectionNodes(const TArray<USplineComponent*>& SplineComponents) const;
	TArray<FNonIntersectionNode> FindSplineNonIntersectionNodes(const TArray<USplineComponent*>& SplineComponents) const;
	
	// Road Mesh Generation
	void GenerateRoadMesh(ARoadActor* RoadActor);
	UProceduralMeshComponent* GenerateQuadMeshFromPoints(ARoadActor* RoadActor, const TArray<FVector>& LeftPoints, const TArray<FVector>& RightPoints);
	UProceduralMeshComponent* GenerateFanMeshFromPoints(ARoadActor* RoadActor, const FVector& CenterPoint, const TArray<FVector>& Points);
	void SetRoadMeshMaterialAndCollision(UProceduralMeshComponent* ProcMeshComponent);

	// Intersection Point Handling
	TArray<FVector> FindIntersectionPointsFromNode(ARoadActor* RoadActor, const FIntersectionNode IntersectionNode, TArray<FSplineData>& SplineDataArray);
	void FindSegmentPointsFromSpline(ARoadActor* RoadActor, const FSplineData& SplineData, TArray<FVector>& LeftPoints, TArray<FVector>& RightPoints);
	void FindAndAddSplineIntersections(FSplineData& SplineA, FSplineData& SplineB, const FIntersectionNode& IntersectionNode);
	void ProcessSplineSidePoints(const TArray<FVector>& SplinePoints, const TArray<FVector>& SplineIntersections, USplineComponent* SplineComponent,
		TArray<FVector>& OutSplineIntersections, const FIntersectionNode& IntersectionNode, TArray<FVector>& OutIntersectionPoints);

	// Spline Data
	void MergeSplineDataIntoMap(TMap<USplineComponent*, FSplineData>& SplineDataMap, const TArray<FSplineData>& SplineDataArray);
	void MergeNonIntersectionPointsIntoMap(const FNonIntersectionNode& NonIntersectionNode, float RoadWidth, TMap<USplineComponent*, FSplineData>& SplineDataMap);

	// Spline Intersection
	bool FindIntersectionBetweenTwoSplines(const TArray<FVector>& SplinePointsA, const TArray<FVector>& SplinePointsB, const FIntersectionNode IntersectionNode, FVector& OutIntersectionPoint);
	bool LineSegmentIntersection2D(const FVector2D& A1, const FVector2D& A2, const FVector2D& B1, const FVector2D& B2, FVector& IntersectionPoint);

	// Point Operations
	FColor GetNextDebugColor();
	void DrawDebugCurvedRoad(ARoadActor* RoadActor);
	static bool IsDegenerateTriangle(const FVector& A, const FVector& B, const FVector& C);
};