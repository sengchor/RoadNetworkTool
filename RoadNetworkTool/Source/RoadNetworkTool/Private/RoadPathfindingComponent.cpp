#include "RoadPathfindingComponent.h"
#include "Containers/Queue.h"
#include "Algo/Reverse.h"
#include "RoadActor.h"

URoadPathfindingComponent::URoadPathfindingComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

TArray<TSharedPtr<FPathNode>> URoadPathfindingComponent::FindAllNodes(const TArray<USplineComponent*>& SplineComponents)
{
    TMap<FVector, TSharedPtr<FPathNode>> NodeMap; // Use smart pointers for NodeMap to manage neighbors correctly

    // Create nodes and link neighbors
    for (USplineComponent* Spline : SplineComponents)
    {
        FVector StartLocation = Spline->GetLocationAtSplinePoint(0, ESplineCoordinateSpace::World);
        FVector EndLocation = Spline->GetLocationAtSplinePoint(Spline->GetNumberOfSplinePoints() - 1, ESplineCoordinateSpace::World);

        // Check or add StartNode
        TSharedPtr<FPathNode>* StartNodePtr = NodeMap.Find(StartLocation);
        if (StartNodePtr == nullptr)
        {
            TSharedPtr<FPathNode> NewNode = MakeShared<FPathNode>(StartLocation);
            NodeMap.Add(StartLocation, NewNode);
            StartNodePtr = &NewNode;
        }

        // Check or add EndNode
        TSharedPtr<FPathNode>* EndNodePtr = NodeMap.Find(EndLocation);
        if (EndNodePtr == nullptr)
        {
            TSharedPtr<FPathNode> NewNode = MakeShared<FPathNode>(EndLocation);
            NodeMap.Add(EndLocation, NewNode);
            EndNodePtr = &NewNode;
        }

        // Link nodes as neighbors if they are different
        if (*StartNodePtr != *EndNodePtr)
        {
            if (!(*StartNodePtr)->Neighbors.Contains(*EndNodePtr))
            {
                (*StartNodePtr)->Neighbors.Add(*EndNodePtr);
            }

            if (!(*EndNodePtr)->Neighbors.Contains(*StartNodePtr))
            {
                (*EndNodePtr)->Neighbors.Add(*StartNodePtr);
            }
        }
    }

    // Convert NodeMap values to TArray
    TArray<TSharedPtr<FPathNode>> PathNodes;
    NodeMap.GenerateValueArray(PathNodes);

    return PathNodes;
}

TArray<TSharedPtr<FPathNode>> URoadPathfindingComponent::AStarPathfinding(TSharedPtr<FPathNode> StartNode, TSharedPtr<FPathNode> GoalNode, const TArray<TSharedPtr<FPathNode>>& AllNodes)
{
    if (!StartNode.IsValid() || !GoalNode.IsValid())
    {
        return TArray<TSharedPtr<FPathNode>>();
    }

    TMap<FVector, TSharedPtr<FPathNode>> NodeMap;
    for (TSharedPtr<FPathNode> Node : AllNodes)
    {
        NodeMap.Add(Node->Location, Node);
    }

    // Open set (nodes to be evaluated) and closed set (nodes already evaluated)
    TSet<TSharedPtr<FPathNode>> OpenSet;
    TSet<TSharedPtr<FPathNode>> ClosedSet;
    TMap<TSharedPtr<FPathNode>, TSharedPtr<FPathNode>> CameFrom; // For path reconstruction
    TMap<TSharedPtr<FPathNode>, float> GScore; // Cost from start node
    TMap<TSharedPtr<FPathNode>, float> FScore; // Total cost (GScore + heuristic)

    // Initialize scores
    OpenSet.Add(StartNode);
    GScore.Add(StartNode, 0.0f);
    FScore.Add(StartNode, FVector::Distance(StartNode->Location, GoalNode->Location));

   // Spline - based Heuristic function
   auto SplineHeuristic = [this](const TSharedPtr<FPathNode> NodeA, const TSharedPtr<FPathNode> NodeB) {
        // Calculate spline distance between points A and B
        USplineComponent* SplineComponent = FindSplineContainingNodes(NodeA, NodeB);

        if (!SplineComponent)
        {
            // Fallback to Euclidean distance if no spline is found
            return FVector::Distance(NodeA->Location, NodeB->Location);
        }

        // Calculate spline distance only between the two nodes
        float StartKey = SplineComponent->FindInputKeyClosestToWorldLocation(NodeA->Location);
        float EndKey = SplineComponent->FindInputKeyClosestToWorldLocation(NodeB->Location);
        double SplineDistance = FMath::Abs(SplineComponent->GetDistanceAlongSplineAtSplineInputKey(EndKey) - SplineComponent->GetDistanceAlongSplineAtSplineInputKey(StartKey));

        return SplineDistance;
        };

    while (OpenSet.Num() > 0)
    {
        // Get node with the lowest FScore
        TSharedPtr<FPathNode> CurrentNode = nullptr;
        float LowestScore = FLT_MAX;
        for (TSharedPtr<FPathNode> Node : OpenSet)
        {
            float Score = FScore[Node];
            if (Score < LowestScore)
            {
                LowestScore = Score;
                CurrentNode = Node;
            }
        }

        if (CurrentNode == GoalNode)
        {
            // Reconstruct path
            TArray<TSharedPtr<FPathNode>> Path;
            while (CameFrom.Contains(CurrentNode))
            {
                Path.Add(CurrentNode);
                CurrentNode = CameFrom[CurrentNode];
            }
            Path.Add(StartNode);
            Algo::Reverse(Path);
            return Path;
        }

        OpenSet.Remove(CurrentNode);
        ClosedSet.Add(CurrentNode);

        for (TSharedPtr<FPathNode> Neighbor : CurrentNode->Neighbors)
        {
            TSharedPtr<FPathNode>* ActualNeighborPtr = NodeMap.Find(Neighbor->Location);
            if (ActualNeighborPtr == nullptr || ClosedSet.Contains(*ActualNeighborPtr))
            {
                continue;
            }

            TSharedPtr<FPathNode> ActualNeighbor = *ActualNeighborPtr;

            float TentativeGScore = GScore[CurrentNode] + SplineHeuristic(CurrentNode, ActualNeighbor);

            if (!OpenSet.Contains(ActualNeighbor))
            {
                OpenSet.Add(ActualNeighbor);
            }
            else if (TentativeGScore >= GScore[ActualNeighbor])
            {
                continue;
            }

            CameFrom.Add(ActualNeighbor, CurrentNode);
            GScore.Add(ActualNeighbor, TentativeGScore);
            FScore.Add(ActualNeighbor, TentativeGScore + SplineHeuristic(ActualNeighbor, GoalNode));
        }
    }

    // No path found
    return TArray<TSharedPtr<FPathNode>>();
}

TSharedPtr<FPathNode> URoadPathfindingComponent::FindNearestNodeByLocation(const FVector& Location, const TArray<TSharedPtr<FPathNode>>& AllNodes)
{
    if (AllNodes.Num() == 0)
    {
        return nullptr;
    }

    TSharedPtr<FPathNode> NearestNode = nullptr;
    float MinDistance = FLT_MAX;

    for (TSharedPtr<FPathNode> Node : AllNodes)
    {
        float Distance = FVector::Dist(Location, Node->Location);
        if (Distance < MinDistance)
        {
            MinDistance = Distance;
            NearestNode = Node;
        }
    }

    return NearestNode;
}

TArray<FVector> URoadPathfindingComponent::GetLocationsFromPathNodes(const TArray<TSharedPtr<FPathNode>>& PathNodes)
{
    TArray<FVector> Locations;

    // Iterate through the path nodes and collect their locations
    for (TSharedPtr<FPathNode> Node : PathNodes)
    {
        if (Node.IsValid())
        {
            Locations.Add(Node->Location);
        }
    }

    return Locations;
}

USplineComponent* URoadPathfindingComponent::FindSplineContainingNodes(TSharedPtr<FPathNode> NodeA, TSharedPtr<FPathNode> NodeB) const
{
    if (!NodeA.IsValid() || !NodeB.IsValid())
    {
        return nullptr;
    }

    TArray<USplineComponent*> NearbySplines;
    FindSplinesInArea(NodeA->Location, DefaultSearchRadius, NearbySplines);

    TArray<USplineComponent*> NearbySplinesForNodeB;
    FindSplinesInArea(NodeB->Location, DefaultSearchRadius, NearbySplinesForNodeB);

    for (USplineComponent* Spline : NearbySplinesForNodeB)
    {
        if (!NearbySplines.Contains(Spline))
        {
            NearbySplines.Add(Spline);
        }
    }

    // Loop through all spline components
    for (USplineComponent* Spline : NearbySplines)
    {
        // Check if the spline contains both NodeA and NodeB
        bool bContainsNodeA = false;
        bool bContainsNodeB = false;

        // Check the first and last points on the spline to compare with NodeA and NodeB locations
        FVector SplineStartLocation = Spline->GetLocationAtSplinePoint(0, ESplineCoordinateSpace::World);
        FVector SplineEndLocation = Spline->GetLocationAtSplinePoint(Spline->GetNumberOfSplinePoints() - 1, ESplineCoordinateSpace::World);

        // Use distance-based checks for precision issues
        const float ToleranceDistance = 10.0f;
        bContainsNodeA = FVector::Dist(SplineStartLocation, NodeA->Location) < ToleranceDistance || FVector::Dist(SplineEndLocation, NodeA->Location) < ToleranceDistance;
        bContainsNodeB = FVector::Dist(SplineStartLocation, NodeB->Location) < ToleranceDistance || FVector::Dist(SplineEndLocation, NodeB->Location) < ToleranceDistance;

        // If both nodes are found at the start or end points of the spline, return the spline component
        if (bContainsNodeA && bContainsNodeB)
        {
            return Spline;
        }
    }

    // If no spline contains both nodes, return nullptr
    return nullptr;
}

void URoadPathfindingComponent::FindSplinesInArea(const FVector& Location, float SearchRadius, TArray<USplineComponent*>& OutSplines) const
{
    ARoadActor* RoadActor = Cast<ARoadActor>(GetOwner());
    if (!RoadActor || !RoadActor->SplineQuadtree.IsValid())
    {
        return;
    }

    // Define a search area as a 2D bounding box 
    FBox2D SearchArea(
        FVector2D(Location.X - SearchRadius, Location.Y - SearchRadius),
        FVector2D(Location.X + SearchRadius, Location.Y + SearchRadius)
    );

    RoadActor->SplineQuadtree->QuerySplinesInArea(SearchArea, OutSplines);

    bool DrawDebug = false;
    if (DrawDebug)
    {
        FVector BoxCenter = Location;
        FVector BoxExtent = FVector(SearchRadius, SearchRadius, 500.0f);

        DrawSplineAndBoxDebug(OutSplines, BoxCenter, BoxExtent);
    }
}

USplineComponent* URoadPathfindingComponent::FindNearestSplineComponent(const FVector& Location)
{
    TArray<USplineComponent*> NearbySplines;

    FindSplinesInArea(Location, DefaultSearchRadius, NearbySplines);

    USplineComponent* NearestSpline = nullptr;
    float MinDistance = FLT_MAX;

    // Loop through all spline components to find the nearest
    for (USplineComponent* Spline : NearbySplines)
    {
        if (Spline)
        {
            // Find the closest location on the spline to the input location
            FVector ClosestPointOnSpline = Spline->FindLocationClosestToWorldLocation(Location, ESplineCoordinateSpace::World);

            // Calculate the distance between the given location and the closest point on the spline
            float Distance = FVector::Dist(Location, ClosestPointOnSpline);

            // Track the nearest spline component
            if (Distance < MinDistance)
            {
                MinDistance = Distance;
                NearestSpline = Spline;
            }
        }
    }

    return NearestSpline;
}

void URoadPathfindingComponent::DrawSplineAndBoxDebug(const TArray<USplineComponent*>& SplineComponents, const FVector BoxCenter, const FVector BoxExtent) const
{
    ARoadActor* RoadActor = Cast<ARoadActor>(GetOwner());

    for (USplineComponent* SplineComponent : SplineComponents)
    {
        if (!SplineComponent) continue;

        FVector StartLocation = SplineComponent->GetLocationAtSplinePoint(0, ESplineCoordinateSpace::World) + FVector(0, 0, RoadActor->RoadThickness);
        FVector EndLocation = SplineComponent->GetLocationAtSplinePoint(SplineComponent->GetNumberOfSplinePoints() - 1,
            ESplineCoordinateSpace::World) + FVector(0, 0, RoadActor->RoadThickness);

        DrawDebugLine(GetWorld(), StartLocation, EndLocation, FColor::Red, false, 5.0f, 0, 50.0f);
    }

    DrawDebugBox(GetWorld(), BoxCenter, BoxExtent, FColor::Blue, false, 5.0f, 0, 50.0f);

    // Print the number of splines found on the screen
    if (GEngine)
    {
        FString DebugMessage = FString::Printf(TEXT("Number of Splines Found: %d"), SplineComponents.Num());
        GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Green, DebugMessage);
    }
}

void URoadPathfindingComponent::DebugDrawSplinesInArea(const FVector& Location, float SearchRadius)
{
    TArray<USplineComponent*> NearbySplines;

    FindSplinesInArea(Location, SearchRadius, NearbySplines);

    FVector BoxCenter = Location;
    FVector BoxExtent = FVector(SearchRadius, SearchRadius, 500.0f);

    DrawSplineAndBoxDebug(NearbySplines, BoxCenter, BoxExtent);
}

bool URoadPathfindingComponent::AreSplineConnected(USplineComponent* SplineA, USplineComponent* SplineB, float Tolerance)
{
    if (SplineA == SplineB)
    {
        return false;
    }

    if (!SplineA || !SplineB)
    {
        return false;
    }

    // Get the start and the end points of both splines
    FVector StartA = SplineA->GetLocationAtSplinePoint(0, ESplineCoordinateSpace::World);
    FVector EndA = SplineA->GetLocationAtSplinePoint(SplineA->GetNumberOfSplinePoints() - 1, ESplineCoordinateSpace::World);

    FVector StartB = SplineB->GetLocationAtSplinePoint(0, ESplineCoordinateSpace::World);
    FVector EndB = SplineB->GetLocationAtSplinePoint(SplineB->GetNumberOfSplinePoints() - 1, ESplineCoordinateSpace::World);

    // Check if any of the endpoints are with the specified tolerance
    if (StartA.Equals(StartB, Tolerance) || StartA.Equals(EndB, Tolerance) ||
        EndA.Equals(StartB, Tolerance) || EndA.Equals(EndB, Tolerance))
    {
        return true;
    }

    return false;
}

void URoadPathfindingComponent::FindSplinesInLineArea(const FVector& LineStart, const FVector& LineEnd, TArray<USplineComponent*>& OutSplines) const
{
    ARoadActor* RoadActor = Cast<ARoadActor>(GetOwner());
    if (!RoadActor || !RoadActor->SplineQuadtree.IsValid())
    {
        return;
    }

    // Define the search area as an axis-aligned bounding box (AABB) around the line
    FBox2D LineBoundingBox(
        FVector2D(FMath::Min(LineStart.X, LineEnd.X) - DefaultSearchRadius, FMath::Min(LineStart.Y, LineEnd.Y) - DefaultSearchRadius),
        FVector2D(FMath::Max(LineStart.X, LineEnd.X) + DefaultSearchRadius, FMath::Max(LineStart.Y, LineEnd.Y) + DefaultSearchRadius)
    );

    // Visualize the bounding box for debugging purposes
    FVector BoxCenter = FVector(
        (LineBoundingBox.Min.X + LineBoundingBox.Max.X) / 2.0f,
        (LineBoundingBox.Min.Y + LineBoundingBox.Max.Y) / 2.0f,
        0.0f
    );
    FVector BoxExtent = FVector(
        (LineBoundingBox.Max.X - LineBoundingBox.Min.X) / 2.0f,
        (LineBoundingBox.Max.Y - LineBoundingBox.Min.Y) / 2.0f,
        500.0f
    );

    RoadActor->SplineQuadtree->QuerySplinesInArea(LineBoundingBox, OutSplines);

    bool DrawDebug = false;
    if (DrawDebug)
    {
        DrawSplineAndBoxDebug(OutSplines, BoxCenter, BoxExtent);
    }
}