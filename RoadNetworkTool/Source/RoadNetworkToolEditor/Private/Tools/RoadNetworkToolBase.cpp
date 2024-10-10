// RoadNetworkToolBase.cpp
#include "RoadNetworkToolBase.h"
#include "Editor/UnrealEd/Public/Selection.h"
#include "Kismet/GameplayStatics.h"

AActor* URoadNetworkToolBase::GetSelectedActor()
{
	if (GEditor)
	{
		USelection* SelectedActors = GEditor->GetSelectedActors();

		// Check if there are any selected actors
		if (SelectedActors->Num() > 0)
		{
			return Cast<AActor>(SelectedActors->GetSelectedObject(0));
		}
	}
	return nullptr;
}

ARoadActor* URoadNetworkToolBase::GetFirstRoadActor(UWorld* World)
{
	if (!World)
	{
		return nullptr;
	}

	TArray<AActor*> FoundActors;
	UGameplayStatics::GetAllActorsOfClass(World, ARoadActor::StaticClass(), FoundActors);

	if (FoundActors.Num() > 0)
	{
		return Cast<ARoadActor>(FoundActors[0]);
	}

	return nullptr;
}

FInputRayHit URoadNetworkToolBase::FindRayHit(const FRay& WorldRay, FVector& HitPos)
{
	// trace a ray into the World
	FCollisionObjectQueryParams QueryParams(FCollisionObjectQueryParams::AllObjects);
	FHitResult Result;
	bool bHitWorld = TargetWorld->LineTraceSingleByObjectType(Result, WorldRay.Origin, WorldRay.PointAt(999999), QueryParams);
	if (bHitWorld)
	{
		HitPos = Result.ImpactPoint;
		return FInputRayHit(Result.Distance);
	}
	return FInputRayHit();
}

void URoadNetworkToolBase::DrawDebugSphereEditor(const FVector& Location, float LifeTime)
{
	if (TargetWorld)
	{
		// Draw a debug sphere
		DrawDebugSphere(TargetWorld, Location, 100.0f, 12, FColor::Red, false, LifeTime);
	}
}

bool URoadNetworkToolBase::GetNearSplinePoint(const FVector& Location, FVector& OutPointLocation, bool& bIsSplineNodeLocation, float Threshold)
{
	if (!SplineActor)
	{
		return false;
	}

	float MinDistance = Threshold;
	bool bFoundNearPoint = false;
	int32 NearestPointIndex = -1;
	FVector NearestSplinePoint;
	bIsSplineNodeLocation = false;

	// Find the nearest spline component
	USplineComponent* NearSplineComponent = SplineActor->PathfindingComponent->FindNearestSplineComponent(Location);

	// Check if a valid spline component was found
	if (!NearSplineComponent || NearSplineComponent->GetNumberOfSplinePoints() < 2)
	{
		return false;
	}

	int32 StartSplineIndex = 0;
	int32 EndSplineIndex = NearSplineComponent->GetNumberOfSplinePoints() - 1;

	for (int32 i = 0; i < NearSplineComponent->GetNumberOfSplinePoints(); ++i)
	{
		FVector SplinePoint = NearSplineComponent->GetLocationAtSplinePoint(i, ESplineCoordinateSpace::World);
		float Distance = FVector::Dist(Location, SplinePoint);

		// Check if this point is closer than the current closest point within the threshold
		if (Distance < MinDistance)
		{
			NearestPointIndex = i;
			NearestSplinePoint = SplinePoint;
			MinDistance = Distance;
			bFoundNearPoint = true;
		}
	}

	if (bFoundNearPoint)
	{
		OutPointLocation = NearestSplinePoint;

		if (NearestPointIndex == StartSplineIndex || NearestPointIndex == EndSplineIndex)
		{
			bIsSplineNodeLocation = true;
		}
	}

	return bFoundNearPoint;
}