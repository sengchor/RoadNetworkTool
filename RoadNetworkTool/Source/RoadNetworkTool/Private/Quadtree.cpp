#include "Quadtree.h"

// ---------- Constructor ---------
FQuadtree::FQuadtree(const FBox2D& InWorldBounds, int32 InMaxSplinesPerNode, int32 InMaxDepth)
    : MaxSplinesPerNode(InMaxSplinesPerNode), MaxDepth(InMaxDepth)
{
    RootNode = MakeShared<FQuadtreeNode>(InWorldBounds);
}

// ---------- Public Methods ---------
void FQuadtree::InsertSplineComponent(USplineComponent* SplineComponent)
{
    InsertSplineIntoNode(RootNode, SplineComponent, 0);
}

void FQuadtree::RemoveSplineComponent(USplineComponent* SplineComponent)
{
    RemoveSplineFromNode(RootNode, SplineComponent);
}

void FQuadtree::UpdateSplineComponent(USplineComponent* SplineComponent)
{
    if (SplineComponent)
    {
        RemoveSplineComponent(SplineComponent);
        InsertSplineComponent(SplineComponent);
    }
}

void FQuadtree::QuerySplinesInArea(const FBox2D& Area, TArray<USplineComponent*>& OutSplines) const
{
    QueryNodeSplinesInArea(RootNode, Area, OutSplines);
}

void FQuadtree::Clear()
{
    if (RootNode.IsValid())
    {
        ClearNode(RootNode);
    }
}

void FQuadtree::GetAllSplines(TArray<USplineComponent*>& OutSplines) const
{
    CollectSplines(RootNode, OutSplines);
}

void FQuadtree::SetVisualizeQuadtree(bool bValue)
{
    bVisualizeQuadtree = bValue;
}

FBox2D FQuadtree::GetBounds() const
{
    return RootNode ? RootNode->Bounds : FBox2D();
}

// ---------- Private Methods ---------
void FQuadtree::InsertSplineIntoNode(TSharedPtr<FQuadtreeNode> Node, USplineComponent* SplineComponent, int32 CurrentDepth)
{
    // Calculate the bounds of the spline in world space
    FBoxSphereBounds WorldBounds = SplineComponent->CalcBounds(SplineComponent->GetComponentTransform());
    FVector SplineMin = WorldBounds.GetBox().Min;
    FVector SplineMax = WorldBounds.GetBox().Max;

    // Convert the bounding box to TBox2<double>
    UE::Math::TBox2<double> SplineBounds(
        FVector2d(SplineMin.X, SplineMin.Y),
        FVector2d(SplineMax.X, SplineMax.Y)
    );

    // If the node is a leaf node and hasn't exceeded the max splines per node, add the spline here
    if (Node->IsLeafNode())
    {
        if (Node->SplineComponents.Num() < MaxSplinesPerNode)
        {
            Node->SplineComponents.Add(SplineComponent);
            return;
        }
        else if (CurrentDepth < MaxDepth)
        {
            SubdivideNode(Node, CurrentDepth);
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("Maximum depth reached for quadtree without subdividing further."));
            Node->SplineComponents.Add(SplineComponent);
            return;
        }
    }

    // Insert the spline into the appropriate child node(s)
    for (int32 i = 0; i < 4; i++)
    {
        if (Node->Children[i]->Bounds.Intersect(SplineBounds))
        {
            InsertSplineIntoNode(Node->Children[i], SplineComponent, CurrentDepth + 1);
        }
    }
}

void FQuadtree::RemoveSplineFromNode(TSharedPtr<FQuadtreeNode> Node, USplineComponent* SplineComponent)
{
    if (!Node.IsValid())
    {
        return;
    }

    // Check if the spline is in this node
    if (Node->SplineComponents.Remove(SplineComponent) > 0)
    {
        // Spline found and removed from this node
        return;
    }

    // If it's not a leaf node, check child nodes
    if (!Node->IsLeafNode())
    {
        for (int32 i = 0; i < 4; i++)
        {
            RemoveSplineFromNode(Node->Children[i], SplineComponent);
        }
    }
}

void FQuadtree::SubdivideNode(TSharedPtr<FQuadtreeNode> Node, int32 CurrentDepth)
{
    FVector2D Min = Node->Bounds.Min;
    FVector2D Max = Node->Bounds.Max;
    FVector2D Center = (Min + Max) / 2;

    // Create child nodes by splitting the bounds into four quadrants
    Node->Children[0] = MakeShared<FQuadtreeNode>(FBox2D(Center, Max)); // Top-Right
    Node->Children[1] = MakeShared<FQuadtreeNode>(FBox2D(FVector2D(Min.X, Center.Y), FVector2D(Center.X, Max.Y))); // Top-Left
    Node->Children[2] = MakeShared<FQuadtreeNode>(FBox2D(Min, Center)); // Bottom-Left
    Node->Children[3] = MakeShared<FQuadtreeNode>(FBox2D(FVector2D(Center.X, Min.Y), FVector2D(Max.X, Center.Y))); // Bottom-Right

    // Move existing splines into the appropriate child nodes
    for (USplineComponent* SplineComponent : Node->SplineComponents)
    {
        InsertSplineIntoNode(Node, SplineComponent, CurrentDepth + 1);

        if (bVisualizeQuadtree)
        {
            for (int32 i = 0; i < 4; i++)
            {
                DrawDebugBoxForNode(Node->Children[i], SplineComponent->GetWorld());
            }
        }
    }

    Node->SplineComponents.Empty(); // Clear splines from the parent node
}

void FQuadtree::QueryNodeSplinesInArea(TSharedPtr<FQuadtreeNode> Node, const FBox2D& Area, TArray<USplineComponent*>& OutSplines) const
{
    // Check if the node's bounds intersect with the query area
    if (!Node->Bounds.Intersect(Area))
    {
        return; // No intersection, return early
    }

    // If the node is a leaf node, check each spline component within it
    if (Node->IsLeafNode())
    {
        for (USplineComponent* SplineComponent : Node->SplineComponents)
        {
            // Calculate the bounds of the spline in world space
            FBoxSphereBounds WorldBounds = SplineComponent->CalcBounds(SplineComponent->GetComponentTransform());
            FVector SplineMin = WorldBounds.GetBox().Min;
            FVector SplineMax = WorldBounds.GetBox().Max;

            // Convert the bounding box to TBox2<double>
            UE::Math::TBox2<double> SplineBounds(
                FVector2d(SplineMin.X, SplineMin.Y),
                FVector2d(SplineMax.X, SplineMax.Y)
            );

            // Check if the spline bounds intersect with the query area
            if (Area.Intersect(SplineBounds))
            {
                OutSplines.Add(SplineComponent);
            }
        }
    }
    else
    {
        // Recursively query the child nodes
        for (int32 i = 0; i < 4; i++)
        {
            QueryNodeSplinesInArea(Node->Children[i], Area, OutSplines);
        }
    }
}

void FQuadtree::ClearNode(TSharedPtr<FQuadtreeNode> Node)
{
    if (!Node.IsValid())
    {
        return;
    }

    // Clear any spline components in the current node
    Node->SplineComponents.Empty();

    // If the node has children (i.e., it's not a leaf node), clear the children as well
    if (!Node->IsLeafNode())
    {
        for (int32 i = 0; i < 4; i++)
        {
            if (Node->Children[i].IsValid())
            {
                ClearNode(Node->Children[i]);
                Node->Children[i].Reset(); // Reset the child node pointer to release the memory
            }
        }
    }
}

void FQuadtree::CollectSplines(const TSharedPtr<FQuadtreeNode>& Node, TArray<USplineComponent*>& OutSplines) const
{
    if (Node.IsValid())
    {
        if (Node->IsLeafNode())
        {
            OutSplines.Append(Node->SplineComponents);
        }
        else
        {
            for (int32 i = 0; i < 4; i++)
            {
                CollectSplines(Node->Children[i], OutSplines);
            }
        }
    }
}

void FQuadtree::DrawDebugBoxForNode(TSharedPtr<FQuadtreeNode> Node, UWorld* World)
{
    FVector2D Center2D = Node->Bounds.GetCenter();
    FVector2D Extent2D = Node->Bounds.GetExtent();

    // Create a 3D center and extent for the debug box (z = 0, but can be adjusted)
    FVector Center3D(Center2D.X, Center2D.Y, 500.0f);
    FVector Extent3D(Extent2D.X, Extent2D.Y, 0.0f);  // You can adjust the z value (height)

    // Draw the box
    DrawDebugBox(World, Center3D, Extent3D, FColor::Green, false, 50.0f, 0, 100.0f);  // Adjust the color, duration, and thickness as needed
}
