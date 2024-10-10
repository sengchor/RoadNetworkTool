#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

/**
 * Spline Helper class
 */
class FRoadHelper
{
public:
    static inline FName GetRoadActorTag()
    {
        static FName NAME_RoadActorTag(TEXT("RoadActor"));
        return NAME_RoadActorTag;
    }

    static inline void SetIsRoadActor(AActor* InActor, bool bIsRoad = true)
    {
        if (InActor)
        {
            if (bIsRoad)
            {
                InActor->Tags.AddUnique(GetRoadActorTag());
            }
            else
            {
                InActor->Tags.Remove(GetRoadActorTag());
            }
        }
    }

    static inline bool IsRoadActor(const AActor* InActor)
    {
        return InActor && InActor->ActorHasTag(GetRoadActorTag());
    }
};
