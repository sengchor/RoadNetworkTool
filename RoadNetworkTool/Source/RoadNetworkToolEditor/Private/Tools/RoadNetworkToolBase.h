// RoadNetworkToolBase.h
#pragma once

#include "CoreMinimal.h"
#include "InteractiveTool.h"
#include "RoadActor.h"
#include "RoadNetworkToolBase.generated.h"

UCLASS(Abstract)
class ROADNETWORKTOOLEDITOR_API URoadNetworkToolBase : public UInteractiveTool
{
    GENERATED_BODY()

public:
    AActor* GetSelectedActor();
    ARoadActor* GetFirstRoadActor(UWorld* World);

    FInputRayHit FindRayHit(const FRay& WorldRay, FVector& HitPos);
    void DrawDebugSphereEditor(const FVector& Location, float LifeTime = 0.5f);
    bool GetNearSplinePoint(const FVector& Location, FVector& OutPointLocation, bool& bIsSplineNodeLocation, float Threshold);

protected:
    UWorld* TargetWorld = nullptr;
    ARoadActor* SplineActor = nullptr;
};