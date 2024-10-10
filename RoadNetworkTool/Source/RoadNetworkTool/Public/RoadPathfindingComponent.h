#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Components/SplineComponent.h"
#include "RoadPathfindingComponent.generated.h"


USTRUCT()
struct FPathNode {
    GENERATED_BODY()

    FVector Location;
    TArray<TSharedPtr<FPathNode>> Neighbors;

    FPathNode() : Location(FVector::ZeroVector) {}
    FPathNode(FVector InLocation) : Location(InLocation) {}

    bool operator==(const FPathNode& Other) const
    {
        return Location.Equals(Other.Location);
    }

    bool operator!=(const FPathNode& Other) const
    {
        return !(*this == Other);
    }
};


UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class ROADNETWORKTOOL_API URoadPathfindingComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    // Constructor
    URoadPathfindingComponent();

    // Properties
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pathfinding")
    float DefaultSearchRadius = 2500.0f;

    // Public Methods
    TArray<TSharedPtr<FPathNode>> FindAllNodes(const TArray<USplineComponent*>& SplineComponents);

    TArray<TSharedPtr<FPathNode>> AStarPathfinding(TSharedPtr<FPathNode> StartNode, TSharedPtr<FPathNode> GoalNode, const TArray<TSharedPtr<FPathNode>>& AllNodes);

    TSharedPtr<FPathNode> FindNearestNodeByLocation(const FVector& Location, const TArray<TSharedPtr<FPathNode>>& AllNodes);

    TArray<FVector> GetLocationsFromPathNodes(const TArray<TSharedPtr<FPathNode>>& PathNodes);

    USplineComponent* FindSplineContainingNodes(TSharedPtr<FPathNode> NodeA, TSharedPtr<FPathNode> NodeB) const;

    void FindSplinesInArea(const FVector& Location, float SearchRadius, TArray<USplineComponent*>& OutSplines) const;

    USplineComponent* FindNearestSplineComponent(const FVector& Location);

    void DrawSplineAndBoxDebug(const TArray<USplineComponent*>& SplineComponents, const FVector BoxCenter, const FVector BoxExtent) const;

    UFUNCTION(BlueprintCallable, Category = "Pathfinding")
    void DebugDrawSplinesInArea(const FVector& Location, float SearchRadius = 5000.0f);

    bool AreSplineConnected(USplineComponent* SplineA, USplineComponent* SplineB, float Tolerance = KINDA_SMALL_NUMBER);

    void FindSplinesInLineArea(const FVector& LineStart, const FVector& LineEnd, TArray<USplineComponent*>& OutSplines) const;
};
