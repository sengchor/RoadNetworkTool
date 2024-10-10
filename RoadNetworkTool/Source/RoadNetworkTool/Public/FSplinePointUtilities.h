#pragma once

#include "CoreMinimal.h"
#include "Components/SplineComponent.h"

namespace SplineUtilities
{
    // Function to get spline points with a specified distance between them
    TArray<FVector> GetSplinePointsWithDistance(USplineComponent* SplineComponent, float DistanceBetweenPoints = 100.0f);

    // Function to get offset points based on the width of the road
    void GetOffsetSplinePointByWidth(USplineComponent* SplineComponent, const FVector& SplinePoint,
        float Width, FVector& LeftOffsetPoint, FVector& RightOffsetPoint);

    // Function to get offset points based on the width of the road for multiple points
    void GetOffsetSplinePointsByWidth(USplineComponent* SplineComponent, const TArray<FVector>& SplinePoints,
        float Width, TArray<FVector>& LeftOffsetPoints, TArray<FVector>& RightOffsetPoints);

    // Function to get spline points within a specified range
    TArray<FVector> GetSplinePointsWithinRange(USplineComponent* SplineComponent, const TArray<FVector>& SplinePoints,
        float StartKey, float EndKey, const FVector StartLocation, const FVector EndLocation);

    // Function to find a point based on distance from a reference point
    FVector FindPointBasedOnDistance(const TArray<FVector>& Points, const FVector& ReferencePoint, bool bFindFurthest);

    FVector FindFurthestPoint(const TArray<FVector>& Points, const FVector& ReferencePoint);

    FVector FindNearestPoint(const TArray<FVector>& Points, const FVector& ReferencePoint);

    // Function to order points in a clockwise manner
    void OrderPointsClockwise(TArray<FVector>& Points);

    // Function to get all spline points between two specified locations along the spline at the specified interval
    TArray<FVector> GetSplinePointsBetweenLocations(USplineComponent* SplineComponent, float DistanceBetweenPoints, const FVector& StartLocation, const FVector& EndLocation);

    // Function to generate evenly spaced points along a straight line between the given start and end locations
    TArray<FVector> GetPointsBetweenLocations(const FVector& StartLocation, const FVector& EndLocation, float DistanceBetweenPoints);
};
