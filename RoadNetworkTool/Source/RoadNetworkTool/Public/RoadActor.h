#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/SplineComponent.h"
#include "ProceduralMeshComponent.h"
#include "RoadPathfindingComponent.h"
#include "Quadtree.h"
#include "RoadActor.generated.h"

UCLASS()
class ROADNETWORKTOOL_API ARoadActor : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	ARoadActor();

	// Static variables for global settings
	static bool bIsInRoadNetworkMode;
	static bool EnableRoadDebugLine;
	static float DebugWidth;
	static float RoadThickness;

	// Components
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quadtree")
	bool VisualizeQuadtree;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quadtree")
	bool VisulaizeSplineQuadtree;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quadtree")
	int32 MaxSplinesPerNode = 5;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quadtree")
	int32 MaxDepth = 5;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Road Properties")
	float RoadWidth;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USceneComponent* RootSceneComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USceneComponent* SplineRootComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USceneComponent* MeshRootComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Pathfinding")
	URoadPathfindingComponent* PathfindingComponent;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Splines")
	TArray<USplineComponent*> SplineComponents;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProceduralMesh")
	TArray<UProceduralMeshComponent*> ProceduralMeshes;

	// Spline management functions
	void AddSplineComponent(USplineComponent* SplineComponent);
	void UpdateSplineComponent(USplineComponent* SplineComponent);
	void AddOrUpdateSplineComponent(USplineComponent* SplineComponent, bool bIsUpdate);
	const TArray<USplineComponent*>& GetSplineComponents() const;

	// Quadtree management
	FBox2D CalculateSquareBounds(float PaddingPercentage);
	void InitializeQuadtree();

	// Pathfinding-related functions
	UFUNCTION(BlueprintCallable, Category = "Pathfinding")
	TArray<FVector> FindPathRoadNetwork(FVector StartLocation, FVector TargetLocation, bool bRightOffset);

	TArray<FVector> RefinePathWithSplinePoints(TArray<TSharedPtr<FPathNode>>& PathNodes, USplineComponent* StartSpline, USplineComponent* EndSpline);
	TArray<FVector> AddPathWithStartAndEndPoints(TArray<FVector>& PathLocations, FVector StartLocation, FVector TargetLocation);
	void AdjustSplineNodes(TArray<TSharedPtr<FPathNode>>& PathNodes, FVector StartLocation, FVector TargetLocation, USplineComponent*& OutStartSpline, USplineComponent*& OutEndSpline);
	void ApplyRightOffsetToPathNodes(TArray<FVector>& Path, float Width);

	// Node management functions
	TArray<TSharedPtr<FPathNode>> CreateDeepCopyOfPathNodes(const TArray<TSharedPtr<FPathNode>>& OriginalPathNodes);
	TSharedPtr<FPathNode> FindNearestNodeWithSpline(const FVector& Location);

	// Debug functions
	UFUNCTION(BlueprintCallable, Category = "Pathfinding")
	void DrawAllSplineDebugLines();
	void DrawDebugRoadWidth(float Width, float Thickness, FColor Color, float Duration);

	void SetDebugSelectedPoint(bool bEnable, const FVector& NewSelectedPoint = FVector::ZeroVector);
	FVector GetSelectedPoint();

	// Utility functions
	virtual bool ShouldTickIfViewportsOnly() const override;
	void DestroyProceduralMeshes();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Spline quadtree
	TSharedPtr<FQuadtree> SplineQuadtree;

private:
	// Path nodes data
	TArray<TSharedPtr<FPathNode>> AllPathNodes;

	// Debug-related variables
	bool bDebugSelectedPoint;
	FVector SelectedPoint;
};
