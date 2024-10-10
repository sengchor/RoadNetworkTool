#include "FSplinePointUtilities.h"

namespace SplineUtilities
{
	TArray<FVector> GetSplinePointsWithDistance(USplineComponent* SplineComponent, float DistanceBetweenPoints)
	{
		TArray<FVector> SplinePoints;

		// Early return if SplineComponent is null or DistanceBetweenPoints is invalid.
		if (SplineComponent == nullptr || DistanceBetweenPoints <= 0.0f)
		{
			return SplinePoints;
		}

		// Get the number of points in the spline component.
		int32 NumPoints = SplineComponent->GetNumberOfSplinePoints();

		float SplineLength = SplineComponent->GetSplineLength();

		// Iterate along the spline length, sampling at each specified distance interval.
		for (float Distance = 0.0f; Distance <= SplineLength; Distance += DistanceBetweenPoints)
		{
			FVector PointLocation = SplineComponent->GetLocationAtDistanceAlongSpline(Distance, ESplineCoordinateSpace::World);
			SplinePoints.Add(PointLocation);
		}

		// Add the last point to ensure it is always included.
		FVector LastPointLocation = SplineComponent->GetLocationAtDistanceAlongSpline(SplineLength, ESplineCoordinateSpace::World);
		if (!SplinePoints.Contains(LastPointLocation))
		{
			SplinePoints.Add(LastPointLocation);
		}

		return SplinePoints;
	}


	void GetOffsetSplinePointByWidth(USplineComponent* SplineComponent, const FVector& SplinePoint, float Width, FVector& LeftOffsetPoint, FVector& RightOffsetPoint)
	{
		// Create a temporary array with the single SplinePoint
		TArray<FVector> SplinePoints = { SplinePoint };
		TArray<FVector> LeftOffsetPoints;
		TArray<FVector> RightOffsetPoints;

		// Call the function for handling multiple points
		GetOffsetSplinePointsByWidth(SplineComponent, SplinePoints, Width, LeftOffsetPoints, RightOffsetPoints);

		// Extract the first (and only) result
		if (LeftOffsetPoints.Num() > 0 && RightOffsetPoints.Num() > 0)
		{
			LeftOffsetPoint = LeftOffsetPoints[0];
			RightOffsetPoint = RightOffsetPoints[0];
		}
	}


	void GetOffsetSplinePointsByWidth(USplineComponent* SplineComponent, const TArray<FVector>& SplinePoints, float Width, TArray<FVector>& LeftOffsetPoints, TArray<FVector>& RightOffsetPoints)
	{
		if (SplineComponent == nullptr || SplinePoints.Num() == 0 || Width <= 0.0f)
		{
			return;
		}

		// Iterate through the spline points to calculate left and right offset points
		for (const FVector& SplinePoint : SplinePoints)
		{
			// Calculate the tangent at this point
			FVector Tangent = SplineComponent->FindTangentClosestToWorldLocation(SplinePoint, ESplineCoordinateSpace::World);

			// Calculate the right and left vectors from the tangent
			FVector RightVector = FVector::CrossProduct(Tangent, FVector::UpVector).GetSafeNormal();
			FVector LeftVector = -RightVector;

			// Calculate and add the left offset point
			FVector LeftOffsetPoint = SplinePoint + LeftVector * Width * 0.5f;
			LeftOffsetPoints.Add(LeftOffsetPoint);

			// Calculate and add the right offset point
			FVector RightOffsetPoint = SplinePoint + RightVector * Width * 0.5f;
			RightOffsetPoints.Add(RightOffsetPoint);
		}
	}

	TArray<FVector> GetSplinePointsWithinRange(USplineComponent* SplineComponent, const TArray<FVector>& SplinePoints, float StartKey, float EndKey, const FVector StartLocation, const FVector EndLocation)
	{
		TArray<FVector> OutSplinePoints;

		// Early return if no points are provided
		if (SplinePoints.Num() == 0 || SplineComponent == nullptr)
		{
			return OutSplinePoints;
		}

		// Ensure StartKey is less than or equal to EndKey.
		if (StartKey > EndKey)
		{
			Swap(StartKey, EndKey);
			Swap(StartLocation, EndLocation);
		}

		// Iterate through the provided spline points
		for (const FVector& SplinePoint : SplinePoints)
		{
			float CurrentKey = SplineComponent->FindInputKeyClosestToWorldLocation(SplinePoint);

			// If point is before or at StartKey, snap it to StartKey
			if (CurrentKey <= StartKey)
			{
				OutSplinePoints.Add(StartLocation);
			}
			// If point is within the range, preserve its original position
			else if (CurrentKey > StartKey && CurrentKey < EndKey)
			{
				OutSplinePoints.Add(SplinePoint);
			}
			// If point is at or beyond EndKey, snap it to EndKey and stop adding further points
			else if (CurrentKey >= EndKey)
			{
				OutSplinePoints.Add(EndLocation);
			}
		}

		return OutSplinePoints;
	}

	FVector FindPointBasedOnDistance(const TArray<FVector>& Points, const FVector& ReferencePoint, bool bFindFurthest)
	{
		if (Points.Num() == 0)
		{
			return FVector(); // Return a default FPoint if there are no points
		}

		// Initialize with the first point in the array
		FVector ResultPoint = Points[0];
		float BestDistanceSquared = FVector::DistSquared(Points[0], ReferencePoint);

		// Iterate through each point to find the target (furthest or nearest) one
		for (const FVector& Point : Points)
		{
			float DistanceSquared = FVector::DistSquared(Point, ReferencePoint);

			// Use a ternary comparison based on whether we're finding the furthest or nearest point
			if ((bFindFurthest && DistanceSquared > BestDistanceSquared) ||
				(!bFindFurthest && DistanceSquared < BestDistanceSquared))
			{
				BestDistanceSquared = DistanceSquared;
				ResultPoint = Point;
			}
		}

		return ResultPoint;
	}

	FVector FindFurthestPoint(const TArray<FVector>& Intersections, const FVector& IntersectionNodePoint)
	{
		return FindPointBasedOnDistance(Intersections, IntersectionNodePoint, true);
	}

	FVector FindNearestPoint(const TArray<FVector>& Intersections, const FVector& IntersectionNodePoint)
	{
		return FindPointBasedOnDistance(Intersections, IntersectionNodePoint, false);
	}

	void OrderPointsClockwise(TArray<FVector>& Points)
	{
		if (Points.Num() < 3)
		{
			UE_LOG(LogTemp, Warning, TEXT("Not enough points to order."));
			return;
		}

		// Calculate the centroid of the points
		FVector Centroid = FVector(0, 0, 0);
		for (const FVector& Point : Points)
		{
			Centroid += Point;
		}
		Centroid /= Points.Num();

		// Sort points around the centroid in clockwise order
		Points.Sort([Centroid](const FVector& A, const FVector& B)
			{
				float AngleA = FMath::Atan2(A.Y - Centroid.Y, A.X - Centroid.X);
				float AngleB = FMath::Atan2(B.Y - Centroid.Y, B.X - Centroid.X);

				return AngleA > AngleB;
			});
	}


	TArray<FVector> GetSplinePointsBetweenLocations(USplineComponent* SplineComponent, float DistanceBetweenPoints, const FVector& StartLocation, const FVector& EndLocation)
	{
		TArray<FVector> SplinePoints;

		// Early return if SplineComponent is null or DistanceBetweenPoints is invalid.
		if (SplineComponent == nullptr || DistanceBetweenPoints <= 0.0f)
		{
			return SplinePoints;
		}

		float SplineLength = SplineComponent->GetSplineLength();

		// Find the closest spline input keys to the start and end locations
		float StartKey = SplineComponent->FindInputKeyClosestToWorldLocation(StartLocation);
		float EndKey = SplineComponent->FindInputKeyClosestToWorldLocation(EndLocation);

		// Ensure start key is always less than end key for proper sampling
		bool bReverseOrder = StartKey > EndKey;
		if (bReverseOrder)
		{
			Swap(StartKey, EndKey);
		}

		// Get the spline distance for the start and end keys
		float StartDistance = SplineComponent->GetDistanceAlongSplineAtSplineInputKey(StartKey);
		float EndDistance = SplineComponent->GetDistanceAlongSplineAtSplineInputKey(EndKey);

		// Iterate along the spline between StartDistance and EndDistance, sampling points at each specified interval
		for (float Distance = StartDistance; Distance <= EndDistance; Distance += DistanceBetweenPoints)
		{
			FVector PointLocation = SplineComponent->GetLocationAtDistanceAlongSpline(Distance, ESplineCoordinateSpace::World);
			SplinePoints.Add(PointLocation);
		}

		// Add the end point to ensure it is always included
		FVector EndPointLocation = SplineComponent->GetLocationAtDistanceAlongSpline(EndDistance, ESplineCoordinateSpace::World);
		if (!SplinePoints.Contains(EndPointLocation))
		{
			SplinePoints.Add(EndPointLocation);
		}

		// Reverse the points if the order was swapped
		if (bReverseOrder)
		{
			Algo::Reverse(SplinePoints);
		}

		return SplinePoints;
	}


	TArray<FVector> GetPointsBetweenLocations(const FVector& StartLocation, const FVector& EndLocation, float DistanceBetweenPoints)
	{
		TArray<FVector> StraightPoints;

		// Calculate the total distance between StartLocation and EndLocation
		float TotalDistance = FVector::Dist(StartLocation, EndLocation);

		// Add points from StartLocation to EndLocation at intervals of DistanceBetweenPoints
		for (float Distance = 0.0f; Distance <= TotalDistance; Distance += DistanceBetweenPoints)
		{
			FVector PointLocation = FMath::Lerp(StartLocation, EndLocation, Distance / TotalDistance);
			StraightPoints.Add(PointLocation);
		}

		// Ensure the EndLocation is added as the final point
		if (!StraightPoints.Contains(EndLocation))
		{
			StraightPoints.Add(EndLocation);
		}

		return StraightPoints;
	}
}